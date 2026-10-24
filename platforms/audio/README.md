# platforms/audio — the FMOD backend

`fmod_miniaudio.cpp` is the engine's sound. It implements the 29 FMOD 5 low-level entry points
`src/libsource/exosound`'s `FModAudioSystem` calls, against the engine's **own** model of that API
(`src/port/fmod/FModAPI.h`), on [miniaudio](../third_party/miniaudio) — so the engine links it
exactly where it linked Aspyr's `libfmod.so`, and a drift in the API model is a compile error here
rather than a crash. The plan is `docs/MIDDLEWARE_REPLACEMENT.md` §1.

```sh
cmake --preset k2-host-pc                              # -DK2P_AUDIO=miniaudio is the default
cmake --preset k2-host-pc -DK2P_AUDIO=null             # the silent backend, as the A/B control
K2_AUDIO_TRACE=1 ./build_run_macos.sh ...         # log every sound, stream and voice event
K2_AUDIO_WAV=/tmp/mix.wav ./build_run_macos.sh ...# render the mix to a WAV instead of a device
platforms/out/k2-host-pc/audio/fmodcheck -v <gamedir>/streammusic
platforms/out/k2-host-pc/audio/fmodcheck --bif <gamedir>/data/sounds.bif
```

| path | what |
|---|---|
| `fmod_miniaudio.cpp` | the 29 entry points. A fixed voice pool with stable `FMOD::Channel*` slots, one `ma_decoder` + `ma_sound` per playing voice, END callbacks deferred to `System::update()` on the game thread, 3D/pan/pitch/occlusion/cursor mapped onto `ma_sound`. |
| `k2wavsniff.{h,c}` | the payload sniffer: what is REALLY inside a KOTOR "wav", and where it starts. |
| `tools/fmodcheck.c` | the headless sweep: sniff + full decode of every file in a directory, or every type-4 resource in a BIF, reporting failures, silence and total duration. |

## The container lies (which is the whole reason `k2wavsniff` exists)

| shape | where | what a naive decoder does |
|---|---|---|
| honest RIFF/PCM | 2,265 `wav` resources in `data/*.bif` | fine |
| RIFF/WAVE whose `fmt ` claims 8-bit 22,050 Hz PCM, whose `data` size is **0**, and whose payload is MP3 | all 160 `streammusic/*.wav` | decodes nothing |
| 470 (0x1d6) bytes of decoy MP3 frames in front of a real RIFF/PCM | all 504 `streamsounds/*.wav` | decodes the decoy, or noise |

⚠ **A lone MPEG sync word is not evidence.** 16-bit PCM is full of `ff` bytes, and `ff ff 02 00`
— two samples of -1 and 2 — passes every single-header test there is. The first cut of the sniffer
called 19 of the 504 streamsounds MP3 on exactly that basis and 12 of them failed outright (the
other 7 would have played garbage). `k2wav_is_mpeg` therefore walks a **chain**: a header is
believed only when the header its own computed frame length points at is also a header, with the
same version, layer and sample rate, three frames deep. With that, the whole install classifies
and decodes clean:

```
streammusic   160 checked: 0 riff, 160 mp3, 0 unknown | 0 FAILED, 0 silent | 105.7 min
streamsounds  504 checked: 504 riff, 0 mp3, 0 unknown | 0 FAILED, 0 silent |  30.9 min
sounds.bif   2265 checked: 2265 riff, 0 mp3, 0 unknown | 0 FAILED, 0 silent |  96.5 min
```

## What the shim has to copy from FMOD, and why

* **The END callback is the engine's clock.** `FModAudioSystemChannelInfo::m_bDone` is set in one
  place only — `HandleChannelEnd`, off the channel END callback — so a conversation waiting on a
  VO line never advances without it. A sound that plays out reports END from `System::update()`,
  on the game thread; miniaudio's own end callback runs on the audio thread, so it only sets a
  flag that `update()` drains. ⚠ A `stop()`ped channel reports **nothing**: `StopChannel` resets
  the record right after `stop()` and the next `PlaySoundA` takes it, so a late END marked the
  next sound done (it was reaped at once: grenades cut off).
* **A handle dies with its channel.** After `stop()` or a played-out END every call through the
  handle fails with `FMOD_ERR_INVALID_HANDLE`, as FMOD's does: a resume that "succeeded" on a
  finished voice left a silent channel the engine took for playing forever.
* **The engine's file callbacks cannot seek.** `SystemSeekCallback` wants FMOD to hand the
  `StreamInfo` back as per-file userdata; FMOD passes 0, and the guard swallows the `fseek`. A
  forward-only reader is all the engine can service, so each stream is read whole, once, at
  `createSound` time (the `FILE*` is already positioned at the stream's offset) and seeking
  happens in our copy. The largest shipped stream is 14.5 MB.
* **A stream's "file name" is its id**, in decimal — `SystemOpenCallback` `strtoul`s it back.
* **`isPlaying`'s result code matters where its bool does not**: `GetIsChannelPlaying` reads
  `FMOD_ERR_CHANNEL_STOLEN` / `FMOD_ERR_INVALID_HANDLE` as "not playing" and ignores the bool.
* **The engine does its own voice management**, so with a pool of `System::init`'s `maxchannels`
  (133 in practice) stealing never actually happens; the stealing path is a backstop.
* **3D panning is the shim's, not miniaudio's.** miniaudio's spatializer gives a voice dead ahead
  0.5 per channel (each speaker is scaled by `(dot + 1) / 2`), 6 dB under the 2D path every music,
  UI and ambient sound takes; FMOD has no such dip, so barks and VO came out quiet. A 3D voice keeps
  miniaudio's distance rolloff with the directional term off, and `update()` pans it from its
  listener-space azimuth (unity at the centre, far ear floored at 0.2). The engine's `setPan` is
  ignored on a 3D voice, as FMOD's default 3D level of 1 ignores 2D panning.
* **`FMOD_System_Create` can run twice.** `CExoSoundInternal::Initialize` on a live system tears
  the backend down and returns false; the next one calls `InitSystem` again. Real FMOD answers
  with a fresh `System` and leaks the old, so a second Create here recycles.

## Deliberate gaps

* `set3DOcclusion` applies the direct term as **gain only**. The one-pole lowpass that belongs
  with it, and HRTF, are the Steam Audio stage (`docs/MIDDLEWARE_REPLACEMENT.md` §2).
* The reverb occlusion term is accepted and ignored: there is no reverb bus.
* Movies still open their **own** SDL audio device (`platforms/video/player/k2movie.c`) instead of
  mixing into this `ma_engine`, as §3.3 of the plan wants. Two output devices coexist on CoreAudio,
  so it works; it is not what the plan asks for.
