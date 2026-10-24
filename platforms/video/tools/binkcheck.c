/*
 * binkcheck -- headless k2bink verifier (host-only dev tool; never shipped).
 *
 *   binkcheck [options] FILE.bik...
 *     --no-oracle     decode only (timing + a per-movie hash); do not spawn ffmpeg
 *     --frames N      stop after N frames
 *     --dump DIR      write every --every'th frame as DIR/<movie>_<frame>.ppm (BT.601 -> RGB)
 *     --every N       (default 30)
 *     --pcm DIR       write each audio track as DIR/<movie>_t<N>.f32 (interleaved float32 LE)
 *     --quiet         one line per movie
 *
 * The oracle is FFmpeg's own Bink decoder, run as a separate process through a pipe
 * (`ffmpeg -f rawvideo` / `-f f32le`). It is a dev-time reference only: nothing links or ships it.
 * Pass criterion: every visible Y/U/V(/A) byte identical, audio within 1 LSB at 16 bits.
 */
#include "k2bink.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct opts { int oracle, quiet, every; unsigned max_frames; const char* dump; const char* pcm; } opts;

static FILE* spawn(const char* fmt, const char* path, int track)
{
    char cmd[4096];
    if (track < 0)
        snprintf(cmd, sizeof cmd, "ffmpeg -nostdin -v error -i '%s' -map 0:v:0 -fps_mode passthrough -f rawvideo -pix_fmt %s -", path, fmt);
    else
        snprintf(cmd, sizeof cmd, "ffmpeg -nostdin -v error -i '%s' -map 0:a:%d -f f32le -", path, track);
    return popen(cmd, "r");
}

static uint64_t fnv(uint64_t h, const uint8_t* p, size_t n)
{
    while (n--) { h ^= *p++; h *= 1099511628211ull; }
    return h;
}

static uint8_t clamp8(int v) { return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v); }

static void dump_ppm(const char* dir, const char* path, const k2bink_planes* f)
{
    const char* base = strrchr(path, '/'); base = base ? base + 1 : path;
    char out[4096];
    snprintf(out, sizeof out, "%s/%.*s_%05u.ppm", dir, (int)(strcspn(base, ".")), base, f->frame_index);
    FILE* fp = fopen(out, "wb");
    if (!fp) { perror(out); return; }
    fprintf(fp, "P6\n%u %u\n255\n", f->width, f->height);
    for (unsigned y = 0; y < f->height; ++y)
        for (unsigned x = 0; x < f->width; ++x) {
            int Y = f->y[y * f->y_stride + x] - 16;
            int U = f->u ? f->u[(y / 2) * f->uv_stride + x / 2] - 128 : 0;
            int V = f->v ? f->v[(y / 2) * f->uv_stride + x / 2] - 128 : 0;
            uint8_t rgb[3] = { clamp8((298 * Y + 409 * V + 128) >> 8),
                               clamp8((298 * Y - 100 * U - 208 * V + 128) >> 8),
                               clamp8((298 * Y + 516 * U + 128) >> 8) };
            fwrite(rgb, 1, 3, fp);
        }
    fclose(fp);
}

/* compares one visible plane against the next w*h bytes of the oracle; returns the differing count */
static long cmp_plane(FILE* ora, const uint8_t* p, int stride, unsigned w, unsigned h, uint8_t* row,
                      const char* name, unsigned frame, int* reported)
{
    long bad = 0;
    for (unsigned y = 0; y < h; ++y) {
        if (fread(row, 1, w, ora) != w) return -1;
        if (memcmp(row, p + (size_t)y * stride, w) == 0) continue;
        for (unsigned x = 0; x < w; ++x)
            if (row[x] != p[(size_t)y * stride + x]) {
                if (!*reported) {
                    fprintf(stderr, "  first video mismatch: frame %u plane %s at (%u,%u): ours %u oracle %u\n",
                            frame, name, x, y, p[(size_t)y * stride + x], row[x]);
                    *reported = 1;
                }
                ++bad;
            }
    }
    return bad;
}

static int check(const char* path, const opts* o)
{
    int err = 0;
    k2bink* b = k2bink_open_file(path, &err);
    if (!b) { printf("%s: open failed: %s\n", path, k2bink_strerror(err)); return 1; }
    k2bink_info_t in;
    k2bink_info(b, &in);
    if (!o->quiet)
        printf("%s: BIK%c %ux%u %u/%u fps, %u frames, alpha=%d gray=%d, %u audio track(s)\n", path,
               in.revision, in.width, in.height, in.fps_num, in.fps_den, in.frames, in.has_alpha,
               in.grayscale, in.audio_tracks);

    FILE* vora = NULL;
    FILE* aora[K2BINK_MAX_TRACKS] = { 0 };
    if (o->oracle) {
        vora = spawn(in.has_alpha ? "yuva420p" : in.grayscale ? "gray" : "yuv420p", path, -1);
        for (unsigned t = 0; t < in.audio_tracks; ++t) aora[t] = spawn(NULL, path, (int)t);
    }

    FILE* pcm[K2BINK_MAX_TRACKS] = { 0 };
    if (o->pcm) {
        const char* base = strrchr(path, '/'); base = base ? base + 1 : path;
        for (unsigned t = 0; t < in.audio_tracks; ++t) {
            char out[4096];
            snprintf(out, sizeof out, "%s/%.*s_t%u.f32", o->pcm, (int)strcspn(base, "."), base, t);
            if (!(pcm[t] = fopen(out, "wb"))) perror(out);
        }
    }
    uint8_t* row = malloc(in.width + 16);
    enum { ACAP = 16384 };
    float* abuf = malloc(sizeof(float) * ACAP * 2);
    float* obuf = malloc(sizeof(float) * ACAP * 2);
    long vbad_frames = 0, vbad_bytes = 0, abad = 0, asamples[K2BINK_MAX_TRACKS] = { 0 };
    double amax = 0, apeak = 0;
    int vrep = 0, arep = 0, rc = 0, oracle_short = 0;
    uint64_t hash = 1469598103934665603ull;
    unsigned n = 0;
    clock_t t0 = clock();

    k2bink_planes f;
    int r;
    while ((o->max_frames == 0 || n < o->max_frames) && (r = k2bink_decode_video(b, &f)) == K2BINK_OK) {
        unsigned cw = (f.width + 1) / 2, ch = (f.height + 1) / 2;
        for (unsigned y = 0; y < f.height; ++y) hash = fnv(hash, f.y + (size_t)y * f.y_stride, f.width);
        if (f.u)
            for (unsigned y = 0; y < ch; ++y) {
                hash = fnv(hash, f.u + (size_t)y * f.uv_stride, cw);
                hash = fnv(hash, f.v + (size_t)y * f.uv_stride, cw);
            }
        if (vora && !oracle_short) {
            long d = cmp_plane(vora, f.y, f.y_stride, f.width, f.height, row, "Y", n, &vrep), e = 0;
            if (d >= 0 && f.u && !in.grayscale) {
                long du = cmp_plane(vora, f.u, f.uv_stride, cw, ch, row, "U", n, &vrep);
                long dv = du < 0 ? -1 : cmp_plane(vora, f.v, f.uv_stride, cw, ch, row, "V", n, &vrep);
                e = du < 0 || dv < 0 ? -1 : du + dv;
            }
            if (e >= 0 && d >= 0 && f.a) {
                long da = cmp_plane(vora, f.a, f.a_stride, f.width, f.height, row, "A", n, &vrep);
                e = da < 0 ? -1 : e + da;
            }
            if (d < 0 || e < 0) { fprintf(stderr, "  oracle video ended early at frame %u\n", n); oracle_short = 1; rc = 1; }
            else if (d + e) {
                if (!o->quiet && vbad_frames < 20) fprintf(stderr, "  frame %u: %ld bytes differ\n", n, d + e);
                ++vbad_frames; vbad_bytes += d + e;
            }
        }
        if (o->dump && n % o->every == 0) dump_ppm(o->dump, path, &f);

        for (unsigned t = 0; t < in.audio_tracks; ++t) {
            unsigned c = in.track[t].channels;
            int got;
            while ((got = k2bink_decode_audio(b, (int)t, abuf, ACAP)) > 0) {
                size_t ns = (size_t)got * c;
                asamples[t] += got;
                if (pcm[t]) fwrite(abuf, sizeof(float), ns, pcm[t]);
                for (size_t i = 0; i < ns; ++i) if (fabs(abuf[i]) > apeak) apeak = fabs(abuf[i]);
                if (!aora[t]) continue;
                size_t have = fread(obuf, sizeof(float), ns, aora[t]);
                for (size_t i = 0; i < have; ++i) {
                    double dd = fabs((double)abuf[i] - obuf[i]);
                    if (dd > amax) amax = dd;
                    if (dd > 1.0 / 32768) {
                        if (!arep) { fprintf(stderr, "  first audio mismatch: track %u sample %ld: ours %.6f oracle %.6f\n",
                                             t, (long)(asamples[t] - got) * (long)c + (long)i, abuf[i], obuf[i]); arep = 1; }
                        ++abad;
                    }
                }
                if (have < ns) { fprintf(stderr, "  oracle audio track %u ended early\n", t); pclose(aora[t]); aora[t] = NULL; rc = 1; }
            }
            if (got < 0) { fprintf(stderr, "  audio decode error on track %u: %s\n", t, k2bink_strerror(got)); rc = 1; }
        }
        ++n;
    }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    if (o->max_frames == 0 && r != K2BINK_EOF) { fprintf(stderr, "  video decode error at frame %u: %s\n", n, k2bink_strerror(r)); rc = 1; }
    if (o->max_frames == 0 && n != in.frames) { fprintf(stderr, "  decoded %u of %u frames\n", n, in.frames); rc = 1; }
    if (o->max_frames == 0) {
        if (vora && !oracle_short && fread(row, 1, 1, vora) == 1) { fprintf(stderr, "  oracle has more video frames than we decoded\n"); rc = 1; }
        for (unsigned t = 0; t < in.audio_tracks; ++t)
            if (aora[t] && fread(obuf, sizeof(float), 1, aora[t]) == 1) {
                /* a trailing remainder is reported but not fatal: container-level padding differs */
                long extra = 1; while (fread(obuf, sizeof(float), 1, aora[t]) == 1) ++extra;
                fprintf(stderr, "  oracle track %u has %ld more samples than we decoded\n", t, extra);
            }
    }
    if (vbad_frames || abad) rc = 1;

    printf("%s: %s  %u frames in %.2fs (%.0f fps)  hash %016llx", path, rc ? "FAIL" : "ok", n, secs,
           secs > 0 ? n / secs : 0.0, (unsigned long long)hash);
    long atotal = 0;
    for (unsigned t = 0; t < in.audio_tracks; ++t) atotal += asamples[t];
    printf("  audio %ld frames, peak %.3f", atotal, apeak);
    if (o->oracle) printf("  video-bad-frames %ld (%ld bytes)  audio-bad %ld  audio-maxdiff %.3g", vbad_frames, vbad_bytes, abad, amax);
    printf("\n");

    if (vora) pclose(vora);
    for (unsigned t = 0; t < in.audio_tracks; ++t) if (aora[t]) pclose(aora[t]);
    for (unsigned t = 0; t < in.audio_tracks; ++t) if (pcm[t]) fclose(pcm[t]);
    free(row); free(abuf); free(obuf);
    k2bink_close(b);
    return rc;
}

int main(int argc, char** argv)
{
    opts o = { 1, 0, 30, 0, NULL, NULL };
    int files = 0, fails = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--no-oracle")) o.oracle = 0;
        else if (!strcmp(argv[i], "--quiet")) o.quiet = 1;
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) o.max_frames = (unsigned)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--every") && i + 1 < argc) o.every = atoi(argv[++i]) > 0 ? atoi(argv[i]) : 1;
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) o.dump = argv[++i];
        else if (!strcmp(argv[i], "--pcm") && i + 1 < argc) o.pcm = argv[++i];
        else if (argv[i][0] == '-') { fprintf(stderr, "unknown option %s\n", argv[i]); return 2; }
        else { ++files; fails += check(argv[i], &o); }
    }
    if (!files) { fprintf(stderr, "usage: binkcheck [--no-oracle] [--frames N] [--dump DIR [--every N]] [--pcm DIR] [--quiet] FILE.bik...\n"); return 2; }
    if (files > 1) printf("%d/%d movies passed\n", files - fails, files);
    return fails ? 1 : 0;
}
