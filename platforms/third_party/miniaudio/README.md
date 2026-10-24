# miniaudio (vendored)

[miniaudio](https://github.com/mackron/miniaudio) **v0.11.25**, commit
`9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` (tag `0.11.25`, 2026-03-04), unmodified.

Dual-licensed **Unlicense OR MIT-0** — see `LICENSE`. Both are permissive with no attribution
requirement, which is what `docs/MIDDLEWARE_REPLACEMENT.md` requires of every replacement.

It backs `platforms/audio/fmod_miniaudio.cpp`, the real FMOD shim (`K2P_AUDIO=miniaudio`).

Only `miniaudio.h` is vendored. Upstream's `miniaudio.c` is a two-line implementation stub; ours is
`k2ma.c`, which exists so the `MA_NO_*` switches live in exactly ONE place (`k2ma.h`) and the
implementation TU and `fmod_miniaudio.cpp` cannot disagree about the struct layouts those switches
change.

To update: drop in a new `miniaudio.h`, refresh the commit line above, rebuild and re-run
`platforms/out/macos/audio/fmodcheck` over `streammusic/`, `streamsounds/` and `data/sounds.bif`.
