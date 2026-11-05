#!/bin/bash
# Build the CFI-instrumented host (libuv + host.c in one ThinLTO unit) with an ignorelist.
# usage: rt/build.sh <variant-name> <ignorelist-file>
# env:   LIBUV_DIR (default <prototype>/.libuv), OUT_DIR (default <prototype>/out/rt)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
PROTO=$(cd "$HERE/.." && pwd)
UV=${LIBUV_DIR:-$PROTO/.libuv}
OUT_ROOT=${OUT_DIR:-$PROTO/out/rt}
V=$1; IGN=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
OUT=$OUT_ROOT/$V; mkdir -p "$OUT"
SRCS=$(python3 -I -c "import sys; sys.path.insert(0,'$PROTO/tools'); import analyze; print(' '.join(analyze.SOURCES))")
CF="-O2 -g -flto=thin -fsanitize=cfi-icall -fsanitize-trap=cfi-icall -fvisibility=hidden -fno-sanitize-ignorelist -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -I$UV/include -I$UV/src -std=gnu11 -fsanitize-ignorelist=$IGN"
objs=""
for s in $SRCS; do o=$OUT/$(echo $s | tr / _).o; clang $CF -w -c "$UV/$s" -o "$o" -MD -MF "$o.d" & objs="$objs $o"; done; wait
clang $CF -c "$HERE/host.c" -o "$OUT/host.o"
clang -flto=thin -fsanitize=cfi-icall -fsanitize-trap=cfi-icall -fno-sanitize-ignorelist -fuse-ld=lld -rdynamic \
  "$OUT/host.o" $objs -o "$OUT/host" -ldl -lpthread
# The uninstrumented "native addon": no CFI flags at all.
[ -f "$OUT_ROOT/addon.so" ] || clang -O2 -fPIC -shared -I"$UV/include" "$HERE/addon.c" -o "$OUT_ROOT/addon.so" -Wl,--allow-shlib-undefined
