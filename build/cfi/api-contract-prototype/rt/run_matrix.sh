#!/bin/bash
# Build the host with several ignorelists and run every scenario against each.
# usage: rt/run_matrix.sh <generated-ignorelist> [<electron checkout with pr/54561>]
# Variants: gen (generated), gen-minus-timers (one exemption removed: must trap),
#           none (no exemptions: every addon scenario must trap),
#           pr (the PR's fun:uv* entries, if an electron checkout is given).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
PROTO=$(cd "$HERE/.." && pwd)
OUT_ROOT=${OUT_DIR:-$PROTO/out/rt}; mkdir -p "$OUT_ROOT"
cp "$1" "$OUT_ROOT/ign.gen.txt"
grep -v '^fun:uv__run_timers$' "$1" > "$OUT_ROOT/ign.gen-minus-timers.txt"
printf '#!special-case-list-v2\n[cfi-icall]\n' > "$OUT_ROOT/ign.none.txt"
VARIANTS="gen gen-minus-timers none"
if [ -n "$2" ]; then
  # Node's patch renames fs-poll's poll_cb to uv__fs_poll_cb; map it back for upstream libuv.
  ( echo '#!special-case-list-v2'; echo '[cfi-icall]'
    git -C "$2" show HEAD:build/cfi/ignorelist.txt | grep '^fun:uv' | sed 's/uv__fs_poll_cb/poll_cb/' ) > "$OUT_ROOT/ign.pr.txt"
  VARIANTS="$VARIANTS pr"
fi
for v in $VARIANTS; do OUT_DIR=$OUT_ROOT "$HERE/build.sh" $v "$OUT_ROOT/ign.$v.txt"; done
set +e
for s in work timer async watchers pipe tcp udp fs os preadv mkstemp getsockname alloc_ok alloc_bad allocator; do
  for v in $VARIANTS; do
    timeout 20 "$OUT_ROOT/$v/host" "$OUT_ROOT/addon.so" $s > "$OUT_ROOT/$v.$s.log" 2>&1; rc=$?
    sig=""; [ $rc -ge 128 ] && sig="SIG$(kill -l $((rc-128)))"
    printf "%-17s %-12s rc=%-3s %-7s callbacks=%s\n" $v $s $rc "$sig" "$(grep -c 'addon cb\|host:' "$OUT_ROOT/$v.$s.log")"
  done
done
