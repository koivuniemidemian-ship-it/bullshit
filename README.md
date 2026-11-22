# SHIZUKA 🤫

A one-knob **spectral gate** for macOS (VST3 + AU, universal), run by an anime librarian who has had enough.

## The knob

**SHHH-O-METER** sets the "quiet line", from off up to −10 dB. Every FFT bin below the line gets pushed down, and the push gets deeper the further you turn it. Low settings clean up hiss, hum and room tone. High settings leave only the loudest peaks, which sounds glitchy and underwater.

The spectrum behind her shows what's happening. Bars under the dashed QUIET LINE are greyed out because they're being silenced.

## Her moods

| Knob | Mood |
|---|---|
| off | dozing, zzz |
| low | polite "shh~" with a cat smile |
| mid | unimpressed half-lidded stare and a sweat drop |
| high | anger vein, glinting glasses, speed lines, shouting |
| max | **SILENCE.** Opaque glowing glasses, rage aura, fangs, screen shake |

Her mouth gets bigger the more sound she's removing.

## Tech

- 2048-point STFT, 75% overlap, sqrt-Hann windows. Reports 2048 samples of latency, so your DAW compensates automatically.
- Soft 6 dB knee, with gains smoothed across neighbouring bins and over time to reduce "musical noise" chirps.

## Getting the .pkg

Every push to `main` builds the installer on GitHub Actions. Open **Actions**, click the latest run, and download it from **Artifacts**. Tag a version to attach it to a GitHub Release:

```bash
git tag v0.2.0 && git push origin v0.2.0
```

The installer is unsigned, so macOS blocks it the first time. Go to **System Settings → Privacy & Security** and click **Open Anyway**.

## Building locally on a Mac

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j
./packaging/build_pkg.sh 0.2.0
```

## Signing and notarizing (optional)

Add these repo secrets and the workflow signs and notarizes automatically: `MACOS_CERT_P12`, `MACOS_CERT_PASSWORD`, `APP_SIGN_ID`, `INSTALLER_SIGN_ID`, `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD`.

## License note

Built on [JUCE](https://juce.com) 8, which is dual-licensed (AGPLv3 or commercial). Check JUCE's terms before selling it.
