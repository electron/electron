#!/bin/bash
# usage: emit_ir.sh <libuv root> <ignorelist> <outdir>
set -e
UV=$(cd "$1" && pwd); IGN=$2; OUT=$3; mkdir -p $OUT
SRCS=$(python3 -I -c "import sys; sys.path.insert(0,'$(dirname $0)'); import analyze; print(' '.join(analyze.SOURCES))")
for s in $SRCS; do [ -f $UV/$s ] || continue
  clang -O0 -S -emit-llvm -flto=thin -fsanitize=cfi-icall -fvisibility=hidden -fno-sanitize-ignorelist \
    -fsanitize-ignorelist=$IGN -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
    -I$UV/include -I$UV/src -std=gnu11 $UV/$s -o $OUT/$(echo $s | tr / _).ll & done; wait
