# Le Goo — by ballhogaudio

A one-knob distortion plugin (VST3 + Audio Unit for macOS) starring Bradygoo.
Turn **Le Goo** up and the sound goes warm → fuzz → full goo, while Bradygoo turns into a
ballhog aura edit and bounces on every beat of your session tempo.

## Get the installer

**Easiest — let GitHub build it**
1. Open the **Actions** tab → latest **Build Le Goo installer** run.
2. Download the **Le-Goo-Installer** artifact (a zip containing `Le Goo Installer.pkg`).
3. Double-click the `.pkg` and click through.

To publish a release with the `.pkg` attached, push a tag:
```
git tag le-goo-v1.0.0 && git push origin le-goo-v1.0.0
```

**Or build it on your own Mac**
1. In the `le-goo` folder, double-click `Build Le Goo Installer.command`
   (if macOS blocks it: right-click → Open → Open).
2. If it asks for Apple's Command Line Tools, install them and run it again.
3. `Le Goo Installer.pkg` appears in the `le-goo` folder after a few minutes.

The installer puts:

| Format | Where | For |
|---|---|---|
| VST3 | `/Library/Audio/Plug-Ins/VST3/Le Goo.vst3` | Ableton, FL Studio, Bitwig, Reaper, Studio One, Cubase |
| AU | `/Library/Audio/Plug-Ins/Components/Le Goo.component` | Logic Pro, GarageBand |

Rescan plugins in your DAW; Le Goo is listed under **ballhogaudio** as a distortion effect.

## Using it
- **Le Goo** is the only control and a real plugin parameter, so it can be automated.
- Bradygoo bounces on the beat while the DAW is playing; **BOUNCE ½× / 1× / 2×** sets how often.
- The window resizes from the corner.

| Le Goo | Stage |
|---|---|
| 0–12% | Clean |
| 12–35% | Warm (tanh soft clip) |
| 35–55% | Fuzz |
| 55–75% | Aura |
| 75–92% | Ballhog |
| 92–100% | Max: wavefold + bitcrush, extra low end |

## How it's built
| Path | What |
|---|---|
| `Source/PluginProcessor.*` | DSP: 35 Hz high-pass → 4× oversampled waveshaper → tone low-pass → low shelf, dry/wet + output trim; reads host tempo and position |
| `Source/PluginEditor.*` | Hosts the interface in a native web view; sends knob changes to the host, streams levels, scope, spectrum and transport to the page |
| `Resources/index.html` | The interface. Open it in a browser to preview it at 130 BPM |
| `Build Le Goo Installer.command` | Fetches JUCE 8 (and CMake if missing), builds a universal binary, ad-hoc signs, makes the `.pkg` |
| `../.github/workflows/le-goo.yml` | Runs the same script on GitHub's macOS runners whenever `le-goo/` changes |

Built with [JUCE](https://juce.com) 8, which the build script downloads. Check JUCE's licence terms before distributing builds publicly.

The plugins and installer are ad-hoc signed, not notarised. On another Mac, macOS may ask
you to allow the installer in **System Settings → Privacy & Security**.
