// fmodcheck -- does our FMOD shim's decode path actually handle the game's audio?
//
// It runs the two halves the shim runs: k2wav_sniff() to find the real payload, then miniaudio to
// decode it end to end. Headless, so a whole install can be swept in seconds without launching
// the game. Anything it reports as `FAIL` or `silent` is a file the shim would play as nothing.
//
//   fmodcheck <dir|file>...            # every *.wav underneath
//   fmodcheck --bif data/sounds.bif    # every type-4 (wav) resource inside a BIF
//   fmodcheck -v ...                   # one line per file, not just the failures
#include <stdint.h>
#include "k2ma.h"
#include "k2wavsniff.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int g_verbose;
static int g_total, g_failed, g_silent;
static double g_seconds;
static int g_kinds[3];

static const char* KindName(k2wav_kind k)
{
    return k == K2WAV_MP3 ? "mp3" : k == K2WAV_RIFF ? "riff" : "unknown";
}

// One buffer in, one verdict out -- the same sniff + decoder-config the shim does.
static void Check(const char* label, const unsigned char* data, size_t size)
{
    k2wav_payload     payload;
    ma_decoder        dec;
    ma_decoder_config cfg;
    ma_uint64         frames = 0, decoded = 0;
    float             buf[4096];
    float             peak = 0.0f;
    double            seconds;

    g_total++;
    k2wav_sniff(data, size, &payload);
    g_kinds[payload.kind < 3 ? payload.kind : 0]++;

    cfg = ma_decoder_config_init(ma_format_f32, 0, 0);
    if (payload.kind == K2WAV_MP3)  cfg.encodingFormat = ma_encoding_format_mp3;
    if (payload.kind == K2WAV_RIFF) cfg.encodingFormat = ma_encoding_format_wav;

    if (ma_decoder_init_memory(data + payload.offset, payload.length, &cfg, &dec) != MA_SUCCESS)
    {
        printf("FAIL  %-44s %zu B, %s payload at +%zu: decoder init\n",
               label, size, KindName(payload.kind), payload.offset);
        g_failed++;
        return;
    }
    ma_decoder_get_length_in_pcm_frames(&dec, &frames);

    for (;;)
    {
        ma_uint64 got = 0;
        ma_uint64 i;
        if (ma_decoder_read_pcm_frames(&dec, buf, sizeof(buf) / sizeof(buf[0]) / dec.outputChannels,
                                       &got) != MA_SUCCESS || got == 0)
            break;
        for (i = 0; i < got * dec.outputChannels; i++)
        {
            float a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > peak) peak = a;
        }
        decoded += got;
    }

    seconds = dec.outputSampleRate ? (double)decoded / dec.outputSampleRate : 0.0;
    g_seconds += seconds;

    if (decoded == 0)
    {
        printf("FAIL  %-44s %zu B, %s at +%zu: decoded 0 frames\n",
               label, size, KindName(payload.kind), payload.offset);
        g_failed++;
    }
    else if (peak < 1.0e-4f)
    {
        printf("silent %-43s %zu B, %s, %.2fs @ %u Hz x%u\n",
               label, size, KindName(payload.kind), seconds, dec.outputSampleRate, dec.outputChannels);
        g_silent++;
    }
    else if (g_verbose)
    {
        printf("ok    %-44s %zu B, %-7s +%-6zu %7.2fs @ %5u Hz x%u  peak %.3f%s\n",
               label, size, KindName(payload.kind), payload.offset, seconds,
               dec.outputSampleRate, dec.outputChannels, peak,
               (frames && decoded != frames) ? "  (length mismatch)" : "");
    }
    ma_decoder_uninit(&dec);
}

static unsigned char* Slurp(const char* path, size_t* size)
{
    FILE* fh = fopen(path, "rb");
    unsigned char* d;
    int64_t n;
    if (!fh) return 0;
    fseek(fh, 0, SEEK_END);
    n = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    d = (unsigned char*)malloc(n > 0 ? (size_t)n : 1);
    *size = d ? fread(d, 1, (size_t)n, fh) : 0;
    fclose(fh);
    return d;
}

static void CheckFile(const char* path)
{
    size_t size = 0;
    unsigned char* d = Slurp(path, &size);
    const char* base = strrchr(path, '/');
    if (!d) { printf("FAIL  %-44s cannot open\n", path); g_failed++; g_total++; return; }
    Check(base ? base + 1 : path, d, size);
    free(d);
}

static unsigned Le32(const unsigned char* p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

// A BIFF's variable resource table: 16 bytes per entry (id, offset, size, type); type 4 is `wav`.
static void CheckBif(const char* path)
{
    size_t size = 0;
    unsigned char* d = Slurp(path, &size);
    unsigned nvar, voff, i;
    if (!d || size < 20 || memcmp(d, "BIFF", 4) != 0)
    {
        fprintf(stderr, "fmodcheck: %s is not a BIFF\n", path);
        free(d);
        return;
    }
    nvar = Le32(d + 8);
    voff = Le32(d + 16);
    for (i = 0; i < nvar; i++)
    {
        const unsigned char* e = d + voff + 16 * (size_t)i;
        unsigned off, len, type;
        char label[64];
        if ((size_t)(voff + 16 * (size_t)(i + 1)) > size) break;
        off  = Le32(e + 4);
        len  = Le32(e + 8);
        type = Le32(e + 12);
        if (type != 4 || (size_t)off + len > size) continue;
        snprintf(label, sizeof(label), "%s#%u", strrchr(path, '/') ? strrchr(path, '/') + 1 : path, i);
        Check(label, d + off, len);
    }
    free(d);
}

static void CheckPath(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) { fprintf(stderr, "fmodcheck: %s: no such path\n", path); return; }
    if (S_ISDIR(st.st_mode))
    {
        DIR* dir = opendir(path);
        struct dirent* ent;
        if (!dir) return;
        while ((ent = readdir(dir)) != 0)
        {
            char child[4096];
            const char* dot = strrchr(ent->d_name, '.');
            if (ent->d_name[0] == '.') continue;
            if (!dot || strcasecmp(dot, ".wav") != 0) continue;
            snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
            CheckFile(child);
        }
        closedir(dir);
        return;
    }
    CheckFile(path);
}

int main(int argc, char** argv)
{
    int i, any = 0;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0) { g_verbose = 1; continue; }
        if (strcmp(argv[i], "--bif") == 0 && i + 1 < argc) { CheckBif(argv[++i]); any = 1; continue; }
        CheckPath(argv[i]);
        any = 1;
    }
    if (!any)
    {
        fprintf(stderr, "usage: fmodcheck [-v] [--bif <file.bif>] <dir|file>...\n");
        return 2;
    }
    printf("\n%d checked: %d riff, %d mp3, %d unknown | %d FAILED, %d silent | %.1f min of audio\n",
           g_total, g_kinds[K2WAV_RIFF], g_kinds[K2WAV_MP3], g_kinds[K2WAV_UNKNOWN],
           g_failed, g_silent, g_seconds / 60.0);
    return g_failed ? 1 : 0;
}
