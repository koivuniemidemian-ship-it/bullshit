# MOIST BOI 💦

A disgusting distortion plug-in (VST3 + AU, macOS universal) hosted by a sweaty, googly-eyed blob who gets visibly more distressed the harder you drive him.

| Knob | What it does |
|---|---|
| **MOISTURE** | Input drive, 0–48 dB, into a 4× oversampled tanh saturator. The blob sweats and then sprays sweat. |
| **STANK** | Asymmetric bias plus a sine wavefolder. More stank = more wobble. |
| **CRUST** | Bit reduction (16→3 bit) and sample-and-hold decimation. Crusty patches show up on the blob. |
| **GUNK** | Darkening lowpass, 20 kHz down to 400 Hz. |
| **OOZE** | Dry/wet mix, latency-compensated. |
| **VOLUME** | Output, −24 to +12 dB. |
| **SQUELCH** | Rectifier fuzz with octave-up grime. Spinning eyes, tongue out. |

His mouth moves with the output level, so screen recordings of him "singing" along with your track are the intended content strategy.

## Getting the .pkg (no Mac dev setup needed)

1. Create an empty repo on GitHub, then push this folder:
   ```bash
   cd moist-boi
   git remote add origin https://github.com/<you>/moist-boi.git
   git push -u origin main
   ```
   Or, with the GitHub CLI: `gh repo create moist-boi --public --source . --push`
2. Open the **Actions** tab. The `build-macos-pkg` workflow runs on a macOS runner, which takes about 10–15 minutes the first time.
3. Download `MoistBoi-…-macOS` from the run's **Artifacts** section. It's a zip containing the `.pkg`.
4. To publish a release with the `.pkg` attached:
   ```bash
   git tag v0.1.0 && git push origin v0.1.0
   ```

## Installing an unsigned build

Without an Apple Developer ID, macOS will block the installer the first time. Try to open it once, then go to **System Settings → Privacy & Security** and click **Open Anyway**. On macOS 15 and later, right-click → Open no longer skips this.

The plug-ins themselves are ad-hoc signed, so Apple Silicon hosts will load them.

## Signing and notarizing (optional, $99/yr Apple Developer account)

Add these repo secrets and the workflow signs and notarizes automatically:

- `MACOS_CERT_P12`: base64 of a .p12 export containing your *Developer ID Application* and *Developer ID Installer* certs (`base64 -i certs.p12 | pbcopy`)
- `MACOS_CERT_PASSWORD`
- `APP_SIGN_ID`: e.g. `Developer ID Application: Your Name (TEAMID)`
- `INSTALLER_SIGN_ID`: e.g. `Developer ID Installer: Your Name (TEAMID)`
- `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD` (app-specific password)

## Building locally on a Mac

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j
./packaging/build_pkg.sh 0.1.0      # → dist/MoistBoi-0.1.0.pkg
```

A Standalone `.app` is also built in `build/MoistBoi_artefacts/Release/Standalone/` for quick testing.

## Rebranding

Edit the block at the top of `CMakeLists.txt` (company, bundle ID, 4-char codes) and the `ID_BASE` / pkg IDs in `packaging/`. Change the plugin code if you fork it into a different plugin.

## License note

Built on [JUCE](https://juce.com) 8, which is dual-licensed (AGPLv3 or a JUCE commercial/starter license). If you plan to sell this or ship it closed-source, check JUCE's current license terms first.
