/*
 * binkplay -- minimal SDL2 player for k2bink (host-only sample; the engine's player is
 * ASL::PlayBinkMovieGL).
 *
 *   binkplay [--track N] [--volume V] [--loop] FILE.bik
 *
 * Video is an SDL IYUV streaming texture. Audio track N (default 0) is queued to one SDL audio
 * device as interleaved float32, and presentation is slaved to the device's consumed-sample clock,
 * as the engine's player is. Without audio, it paces to the movie's fps with the wall clock.
 * Esc / Space / click skips (the engine's BinkEventFilter semantics). Runs headless with
 * SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy.
 */
#include "k2bink.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    const char* path = NULL;
    int track = 0, loop = 0;
    float volume = 1.0f;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--track") && i + 1 < argc) track = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--volume") && i + 1 < argc) volume = (float)atof(argv[++i]);
        else if (!strcmp(argv[i], "--loop")) loop = 1;
        else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: binkplay [--track N] [--volume V] [--loop] FILE.bik\n"); return 2; }

    int err;
    k2bink* b = k2bink_open_file(path, &err);
    if (!b) { fprintf(stderr, "%s: %s\n", path, k2bink_strerror(err)); return 1; }
    k2bink_info_t in;
    k2bink_info(b, &in);
    printf("%s: BIK%c %ux%u @ %.3f fps, %u frames, %u audio track(s)\n", path, in.revision, in.width,
           in.height, (double)in.fps_num / in.fps_den, in.frames, in.audio_tracks);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow(path, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       in.width > 1280 ? 1280 : (int)in.width,
                                       in.width > 1280 ? (int)(in.height * 1280 / in.width) : (int)in.height,
                                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (!ren && win) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "SDL window/renderer: %s\n", SDL_GetError()); return 1; }
    SDL_RenderSetLogicalSize(ren, (int)in.width, (int)in.height);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                         (int)in.width, (int)in.height);

    SDL_AudioDeviceID dev = 0;
    unsigned ach = 0, arate = 0;
    if (track >= 0 && (unsigned)track < in.audio_tracks) {
        SDL_AudioSpec want = { 0 }, have;
        arate = in.track[track].sample_rate;
        ach = in.track[track].channels;
        want.freq = (int)arate; want.format = AUDIO_F32SYS; want.channels = (Uint8)ach; want.samples = 1024;
        dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (!dev) fprintf(stderr, "no audio: %s\n", SDL_GetError());
        else SDL_PauseAudioDevice(dev, 0);
    }

    enum { ACAP = 8192 };
    float* abuf = malloc(sizeof(float) * ACAP * 2);
    Uint64 queued = 0;              /* sample frames ever queued */
    Uint64 t0 = SDL_GetPerformanceCounter();
    double freq = (double)SDL_GetPerformanceFrequency();
    int quit = 0, shown = 0;
    k2bink_planes f;

    while (!quit) {
        int r = k2bink_decode_video(b, &f);
        if (r == K2BINK_EOF && loop) { k2bink_seek_keyframe(b, 0); t0 = SDL_GetPerformanceCounter(); continue; }
        if (r != K2BINK_OK) {
            if (r != K2BINK_EOF) fprintf(stderr, "decode error at frame %d: %s\n", shown, k2bink_strerror(r));
            break;
        }
        for (unsigned t = 0; t < in.audio_tracks; ++t) {
            int n;
            while ((n = k2bink_decode_audio(b, (int)t, abuf, ACAP)) > 0)
                if (dev && (int)t == track) {
                    for (int i = 0; i < n * (int)ach; ++i) abuf[i] *= volume;
                    SDL_QueueAudio(dev, abuf, (Uint32)(n * ach * sizeof(float)));
                    queued += (Uint64)n;
                }
        }
        SDL_UpdateYUVTexture(tex, NULL, f.y, f.y_stride, f.u, f.uv_stride, f.v, f.uv_stride);

        /* wait until this frame's presentation time on the master clock */
        double due = (double)f.frame_index * in.fps_den / in.fps_num;
        for (;;) {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                if (e.type == SDL_QUIT || e.type == SDL_MOUSEBUTTONDOWN ||
                    (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_SPACE)))
                    quit = 1;
            double now;
            if (dev && queued) {
                Uint64 pending = SDL_GetQueuedAudioSize(dev) / (ach * sizeof(float));
                now = (double)(queued - pending) / arate;
                /* underrun: the device clock stalls; don't let video stall with it forever */
                if (pending == 0 && now < due) now = due;
            } else {
                now = (double)(SDL_GetPerformanceCounter() - t0) / freq;
            }
            if (quit || now >= due) break;
            double wait = due - now;
            SDL_Delay(wait > 0.004 ? (Uint32)(wait * 1000 * 0.5) : 1);
        }
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        ++shown;
    }
    while (!quit && dev && SDL_GetQueuedAudioSize(dev) > 0) SDL_Delay(10);   /* drain the tail */

    printf("shown %d frame(s)%s\n", shown, quit ? " (skipped)" : "");
    free(abuf);
    if (dev) SDL_CloseAudioDevice(dev);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    k2bink_close(b);
    return 0;
}
