#!/bin/bash
# Measures what a first launch of a freshly copied and signed Electron.app costs
# on this machine, the way api-autoupdater-darwin-spec.ts makes its fixture
# apps. Gatekeeper scans a bundle the first time code in it runs, and
# syspolicyd does those scans one at a time.
#
# Usage: autoupdater-stress-probe.sh <Electron.app> <signing identity> [ramdisk dir]

set -uo pipefail

APP=$1
IDENTITY=$2
RAMDISK=${3:-}

now_ms() { python3 -c 'import time; print(int(time.time() * 1000))'; }

WORK=$(mktemp -d)
mkdir -p "$WORK/app"
printf '{"name":"probe","main":"main.js"}' > "$WORK/app/package.json"
echo 'process.exit(0);' > "$WORK/app/main.js"

launch() {
  local start
  start=$(now_ms)
  "$1/Contents/MacOS/Electron" "$WORK/app" > /dev/null 2>&1
  echo "  $2: $(($(now_ms) - start))ms"
}

echo "== machine"
sysctl -n machdep.cpu.brand_string hw.ncpu hw.memsize
sw_vers | tr '\n' ' '
echo
spctl --status 2>&1 || true
df -h / | tail -1

echo "== template (copy, strip -x, deep sign)"
mkdir -p "$WORK/template"
start=$(now_ms)
cp -cR "$APP" "$WORK/template/"
TEMPLATE="$WORK/template/Electron.app"
FRAMEWORK="$TEMPLATE/Contents/Frameworks/Electron Framework.framework/Versions/A/Electron Framework"
echo "  copy: $(($(now_ms) - start))ms, framework $(stat -f%z "$FRAMEWORK") bytes"
start=$(now_ms)
strip -x "$FRAMEWORK"
echo "  strip -x: $(($(now_ms) - start))ms, framework $(stat -f%z "$FRAMEWORK") bytes"
start=$(now_ms)
codesign -s "$IDENTITY" --deep --force "$TEMPLATE" 2>/dev/null
echo "  deep sign: $(($(now_ms) - start))ms"
launch "$TEMPLATE" "template first launch"
launch "$TEMPLATE" "template second launch"

clone() {
  mkdir -p "$1"
  cp -cR "$TEMPLATE" "$1/"
  /usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier com.github.Electron.probe$RANDOM" "$1/Electron.app/Contents/Info.plist"
  codesign -s "$IDENTITY" --force "$1/Electron.app" 2>/dev/null
}

echo "== fresh clones, one at a time"
for i in 1 2 3 4; do
  clone "$WORK/seq$i"
  launch "$WORK/seq$i/Electron.app" "clone $i first launch"
done

echo "== fresh clones, four at once"
for i in 1 2 3 4; do clone "$WORK/par$i"; done
start=$(now_ms)
for i in 1 2 3 4; do
  (
    "$WORK/par$i/Electron.app/Contents/MacOS/Electron" "$WORK/app" > /dev/null 2>&1
    echo "  parallel launch $i finished at +$(($(now_ms) - start))ms"
  ) &
done
wait

echo "== zip -0 of one clone"
start=$(now_ms)
(cd "$WORK/seq1" && zip -q -0 -r --symlinks "$WORK/seq1.zip" ./)
echo "  zip: $(($(now_ms) - start))ms, $(stat -f%z "$WORK/seq1.zip") bytes"

if [ -n "$RAMDISK" ]; then
  echo "== ramdisk at $RAMDISK"
  mkdir -p "$RAMDISK/probe-template"
  start=$(now_ms)
  cp -R "$TEMPLATE" "$RAMDISK/probe-template/"
  echo "  copy template to ramdisk: $(($(now_ms) - start))ms"
  RAM_TEMPLATE="$RAMDISK/probe-template/Electron.app"
  for i in 1 2; do
    mkdir -p "$RAMDISK/probe$i"
    start=$(now_ms)
    cp -cR "$RAM_TEMPLATE" "$RAMDISK/probe$i/"
    /usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier com.github.Electron.ramprobe$RANDOM" "$RAMDISK/probe$i/Electron.app/Contents/Info.plist"
    codesign -s "$IDENTITY" --force "$RAMDISK/probe$i/Electron.app" 2>/dev/null
    echo "  ramdisk clone $i prepare: $(($(now_ms) - start))ms"
    launch "$RAMDISK/probe$i/Electron.app" "ramdisk clone $i first launch"
  done
  start=$(now_ms)
  (cd "$RAMDISK/probe1" && zip -q -0 -r --symlinks "$RAMDISK/probe1.zip" ./)
  echo "  ramdisk zip: $(($(now_ms) - start))ms"
  rm -rf "$RAMDISK"/probe*
fi

echo "== syspolicyd scans in the last 10 minutes"
/usr/bin/log show --last 10m --style compact --predicate 'process == "syspolicyd"' 2>/dev/null |
  grep -c "GK performScan" || true

rm -rf "$WORK"
