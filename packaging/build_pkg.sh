#!/usr/bin/env bash
# Builds dist/Shizuka-<version>.pkg from the compiled VST3 + AU bundles.
#
# Optional env vars (all may be empty -> unsigned / ad-hoc signed build):
#   APP_SIGN_ID        "Developer ID Application: Your Name (TEAMID)"
#   INSTALLER_SIGN_ID  "Developer ID Installer: Your Name (TEAMID)"
#   ARTEFACTS_DIR      defaults to build/Shizuka_artefacts/Release
set -euo pipefail

VERSION="${1:-0.1.0}"
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
ART="${ARTEFACTS_DIR:-$ROOT/build/Shizuka_artefacts/Release}"
WORK="$ROOT/build/pkg"
OUT="$ROOT/dist"
ID_BASE="com.dampaudio.shizuka"

rm -rf "$WORK"
mkdir -p "$WORK/vst3root" "$WORK/auroot" "$OUT"

VST3="$(find "$ART" -maxdepth 2 -name '*.vst3' -type d | head -n1)"
AU="$(find "$ART" -maxdepth 2 -name '*.component' -type d | head -n1)"
[[ -d "$VST3" ]] || { echo "VST3 bundle not found in $ART"; exit 1; }
[[ -d "$AU"   ]] || { echo "AU bundle not found in $ART"; exit 1; }

sign_bundle() {
  if [[ -n "${APP_SIGN_ID:-}" ]]; then
    codesign --force --options runtime --timestamp --sign "$APP_SIGN_ID" "$1"
  else
    codesign --force --sign - "$1"   # ad-hoc, required for Apple Silicon to load it
  fi
}
sign_bundle "$VST3"
sign_bundle "$AU"

ditto "$VST3" "$WORK/vst3root/$(basename "$VST3")"
ditto "$AU"   "$WORK/auroot/$(basename "$AU")"

# Component plists with relocation OFF, otherwise Installer may "relocate"
# the plugin into some old build folder instead of /Library/Audio/Plug-Ins.
make_plist() {
  pkgbuild --analyze --root "$1" "$2" >/dev/null
  plutil -replace 0.BundleIsRelocatable -bool NO "$2"
}
make_plist "$WORK/vst3root" "$WORK/vst3.plist"
make_plist "$WORK/auroot"   "$WORK/au.plist"

chmod +x "$HERE/scripts/postinstall"

pkgbuild --root "$WORK/vst3root" --component-plist "$WORK/vst3.plist" \
  --identifier "$ID_BASE.vst3" --version "$VERSION" \
  --install-location "/Library/Audio/Plug-Ins/VST3" \
  "$WORK/Shizuka-VST3.pkg"

pkgbuild --root "$WORK/auroot" --component-plist "$WORK/au.plist" \
  --identifier "$ID_BASE.au" --version "$VERSION" \
  --install-location "/Library/Audio/Plug-Ins/Components" \
  --scripts "$HERE/scripts" \
  "$WORK/Shizuka-AU.pkg"

sed "s/__VERSION__/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"

SIGN_ARGS=()
[[ -n "${INSTALLER_SIGN_ID:-}" ]] && SIGN_ARGS=(--sign "$INSTALLER_SIGN_ID")

productbuild --distribution "$WORK/distribution.xml" \
  --package-path "$WORK" \
  --resources "$HERE/resources" \
  "${SIGN_ARGS[@]+"${SIGN_ARGS[@]}"}" \
  "$OUT/Shizuka-$VERSION.pkg"

echo "Built $OUT/Shizuka-$VERSION.pkg"
