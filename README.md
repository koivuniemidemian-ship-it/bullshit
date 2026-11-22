# HUSH-CHAN 🤫

A one-knob **spectral gate** for macOS (VST3 + AU, universal), run by an anime girl who starts out sweet and icy-blue and turns into a blood-soaked yandere as you turn the knob.

## The knob

**ANGER** sets the "quiet line", from off up to −10 dB. Every FFT bin below the line gets pushed down, and the push gets deeper the further you turn it. Low settings clean up hiss, hum and room tone. High settings leave only the loudest peaks, which sounds glitchy and underwater.

The spectrum behind her shows what's happening. Bars under the dashed QUIET LINE are greyed out because they're being silenced.

## Her moods

| Knob | What happens |
|---|---|
| off | eyes closed, peaceful; blue sky, drifting clouds, snowy mountains, birds, flowers |
| low | sparkly eyes, happy smile, "hehe, shh~" |
| mid | pout and squint; a blood moon starts rising behind the mountains |
| high | yandere shadow over her eyes, empty pupils, blood on her face, dress and hair; the sky turns red, bats replace birds, blood drips from the top of the screen |
| max | **SILENCE.** Slit pupils, glowing eyes, blood tears, crazy grin. The moon bleeds, blood rains, splatters everywhere, a dripping blood frame, screen shake |

## Custom character art

The character is drawn in code. For illustration-quality art, put `girl_calm.png` (and optionally `girl_rage.png`) in `art/` and push. See `art/README.md`. Only use art you own or commissioned.

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
