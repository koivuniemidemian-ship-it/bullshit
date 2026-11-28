#!/bin/bash
# Builds Le Goo (VST3 + Audio Unit) on this Mac and wraps it in "Le Goo Installer.pkg".
# Double-click this file, or run:  bash "Build Le Goo Installer.command"

set -eo pipefail
cd "$(dirname "$0")"
HERE="$PWD"
LOG="$HERE/build.log"
exec > >(tee "$LOG") 2>&1

fail() {
  echo
  echo "✖ Build stopped: $1"
  echo "  The full log is in: $LOG"
  echo "  Send build.log back to Claude and it can fix the problem."
  read -r -p "Press Return to close." _ || true
  exit 1
}
trap 'fail "a step failed (see the lines above)."' ERR

echo "== Le Goo by ballhogaudio — building the installer =="
echo

# 1) Apple's command line tools (compiler, git, codesign, pkgbuild)
if ! xcode-select -p >/dev/null 2>&1; then
  echo "Apple's Command Line Tools are needed. A window will ask to install them."
  xcode-select --install || true
  echo "When that install finishes, double-click this file again."
  read -r -p "Press Return to close." _ || true
  exit 0
fi
echo "✓ Command line tools: $(xcode-select -p)"

# 2) CMake (uses yours if installed, otherwise downloads a private copy into ./tools)
CMAKE="$(command -v cmake || true)"
if [ -n "$CMAKE" ]; then
  ver="$("$CMAKE" --version | head -1 | awk '{print $3}')"
  major="${ver%%.*}"; rest="${ver#*.}"; minor="${rest%%.*}"
  if [ "$major" -lt 3 ] || { [ "$major" -eq 3 ] && [ "$minor" -lt 22 ]; }; then CMAKE=""; fi
fi
if [ -z "$CMAKE" ]; then
  CMAKE="$HERE/tools/cmake/CMake.app/Contents/bin/cmake"
  if [ ! -x "$CMAKE" ]; then
    echo "Downloading CMake 3.30.5…"
    mkdir -p tools
    curl -L --fail --progress-bar -o tools/cmake.tar.gz \
      "https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-macos-universal.tar.gz"
    tar -xzf tools/cmake.tar.gz -C tools
    mv tools/cmake-3.30.5-macos-universal tools/cmake
    rm -f tools/cmake.tar.gz
  fi
fi
echo "✓ CMake: $("$CMAKE" --version | head -1)"

# 3) JUCE (the plugin framework)
if [ ! -f JUCE/CMakeLists.txt ]; then
  echo "Downloading JUCE 8…"
  rm -rf JUCE
  git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git JUCE \
    || git clone --depth 1 https://github.com/juce-framework/JUCE.git JUCE
fi
echo "✓ JUCE ready"

# 4) Compile (universal: Apple Silicon + Intel)
echo
echo "Compiling Le Goo — this takes a few minutes the first time…"
"$CMAKE" -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
"$CMAKE" --build build --config Release -j "$(sysctl -n hw.ncpu)"

VST3="$(find build -type d -name 'Le Goo.vst3' -path '*VST3*' | head -1)"
AU="$(find build -type d -name 'Le Goo.component' -path '*AU*' | head -1)"
[ -d "$VST3" ] || fail "the VST3 was not produced."
[ -d "$AU" ]   || fail "the Audio Unit was not produced."
echo "✓ Built: $VST3"
echo "✓ Built: $AU"

# 5) Sign locally (ad-hoc) so macOS will load the plugins
codesign --force --deep --sign - "$VST3"
codesign --force --deep --sign - "$AU"
echo "✓ Signed (ad-hoc)"

# 6) Make the installer
PK="$HERE/build/pkg"
rm -rf "$PK"; mkdir -p "$PK/vst3root" "$PK/auroot" "$PK/scripts-au" "$PK/parts"
cp -R "$VST3" "$PK/vst3root/"
cp -R "$AU" "$PK/auroot/"

cat > "$PK/scripts-au/postinstall" <<'EOS'
#!/bin/bash
# make Logic / GarageBand notice the new Audio Unit
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
exit 0
EOS
chmod +x "$PK/scripts-au/postinstall"

# keep the installer from "relocating" the plugins to some other copy on the disk
for kind in vst3 au; do
  pkgbuild --analyze --root "$PK/${kind}root" "$PK/$kind.plist" >/dev/null
  if /usr/libexec/PlistBuddy -c "Print :0" "$PK/$kind.plist" >/dev/null 2>&1; then
    plutil -replace 0.BundleIsRelocatable -bool NO "$PK/$kind.plist"
  else
    rm -f "$PK/$kind.plist"
  fi
done
compplist() { [ -f "$PK/$1.plist" ] && printf -- '--component-plist\n%s\n' "$PK/$1.plist"; true; }
VST3_CP=(); while IFS= read -r l; do VST3_CP+=("$l"); done < <(compplist vst3)
AU_CP=();   while IFS= read -r l; do AU_CP+=("$l");   done < <(compplist au)

pkgbuild --root "$PK/vst3root" "${VST3_CP[@]}" \
  --identifier com.ballhogaudio.legoo.vst3 --version 1.0.0 \
  --install-location "/Library/Audio/Plug-Ins/VST3" "$PK/parts/LeGoo-VST3.pkg"
pkgbuild --root "$PK/auroot" "${AU_CP[@]}" --scripts "$PK/scripts-au" \
  --identifier com.ballhogaudio.legoo.au --version 1.0.0 \
  --install-location "/Library/Audio/Plug-Ins/Components" "$PK/parts/LeGoo-AU.pkg"

cat > "$PK/distribution.xml" <<'EOX'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Le Goo by ballhogaudio</title>
  <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline>
    <line choice="vst3"/>
    <line choice="au"/>
  </choices-outline>
  <choice id="vst3" title="Le Goo VST3" description="For Ableton Live, FL Studio, Bitwig, Reaper, Studio One, Cubase. Installs to /Library/Audio/Plug-Ins/VST3.">
    <pkg-ref id="com.ballhogaudio.legoo.vst3"/>
  </choice>
  <choice id="au" title="Le Goo Audio Unit" description="For Logic Pro and GarageBand. Installs to /Library/Audio/Plug-Ins/Components.">
    <pkg-ref id="com.ballhogaudio.legoo.au"/>
  </choice>
  <pkg-ref id="com.ballhogaudio.legoo.vst3" version="1.0.0">LeGoo-VST3.pkg</pkg-ref>
  <pkg-ref id="com.ballhogaudio.legoo.au" version="1.0.0">LeGoo-AU.pkg</pkg-ref>
</installer-gui-script>
EOX

OUT="$HERE/Le Goo Installer.pkg"
rm -f "$OUT"
productbuild --distribution "$PK/distribution.xml" --package-path "$PK/parts" "$OUT"

trap - ERR
echo
echo "✔ Done:  $OUT"
echo "  Double-click it to install Le Goo (VST3 + AU), then rescan plugins in your DAW."
open -R "$OUT" || true
read -r -p "Press Return to close." _ || true
