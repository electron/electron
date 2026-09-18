#!/bin/bash
# Build a dist-like directory (stripped electron + runtime files) from an out dir.
set -e
OUT=${1:?out dir}
DIST=${2:-$(dirname $0)/dist}
MANIFEST=$(dirname $0)/../zip_manifests/dist_zip.linux.x64.manifest
rm -rf "$DIST"; mkdir -p "$DIST/locales" "$DIST/resources"
while read -r f; do
  case "$f" in
    electron) ;; LICENSE|LICENSES.chromium.html|version) [ -f "$OUT/$f" ] && cp "$OUT/$f" "$DIST/$f" || true ;;
    *) if [ -f "$OUT/$f" ]; then cp "$OUT/$f" "$DIST/$f"; else echo "missing: $f"; fi ;;
  esac
done < "$MANIFEST"
strip -o "$DIST/electron" "$OUT/electron"
mkdir -p "$DIST/resources/app" && cp "$(dirname $0)/app/main.js" "$(dirname $0)/app/package.json" "$DIST/resources/app/"
ls -la "$DIST/electron" "$OUT/electron" | awk '{print $5, $9}'
du -sh "$DIST"
