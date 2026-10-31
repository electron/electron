#!/bin/bash
# Stands in for devtools-frontend's esbuild binary on macOS CI, to diagnose the
# "Failed to run esbuild: Error: The service was stopped" build flake. The
# build-electron action moves the real binary to esbuild.real and installs this
# script as esbuild. The esbuild npm package runs it in service mode and talks
# to it over stdin/stdout, so those are passed through untouched; stderr is
# forwarded and also kept. A failed run (non-zero exit or killed by a signal)
# leaves a record with the exit status and the end of stderr in
# $ESBUILD_EXIT_LOG_DIR (default $RUNNER_TEMP/esbuild-exit-logs), which the
# action prints when the build fails. A successful run leaves nothing.
#
# Runs under macOS's bash 3.2.

real="$0.real"
log_dir="${ESBUILD_EXIT_LOG_DIR:-${RUNNER_TEMP:-${TMPDIR:-/tmp}}/esbuild-exit-logs}"
log_dir="${log_dir%/}"

# If the logging cannot be set up, behave exactly like the real binary.
mkdir -p "$log_dir" 2>/dev/null || exec "$real" "$@"
stderr_file="$log_dir/stderr.$$"
fifo="$log_dir/stderr.$$.fifo"
rm -f "$fifo"
mkfifo "$fifo" 2>/dev/null || exec "$real" "$@"

# Pass on the signals a parent would send to stop esbuild, and note them so a
# record shows whether the kill came through this process. A background job
# of a non-interactive shell starts with SIGINT ignored (and bash 3.2 cannot
# undo that), so SIGINT is passed on as SIGTERM, which ends esbuild the same
# way.
child=""
forwarded=""
# shellcheck disable=SC2329 # invoked by the traps below
forward() {
  forwarded="$forwarded $1"
  [ -n "$child" ] && kill -s "$2" "$child" 2>/dev/null
}
trap 'forward TERM TERM' TERM
trap 'forward INT TERM' INT
trap 'forward HUP HUP' HUP

started_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)
SECONDS=0

tee "$stderr_file" < "$fifo" >&2 &
tee_pid=$!

# The explicit <&0 keeps the protocol pipe as the background job's stdin (a
# non-interactive shell would otherwise give it /dev/null).
"$real" "$@" <&0 2> "$fifo" &
child=$!

# Do not hold the protocol pipes open: when the real binary goes away, the
# parent must see stdout close just as it would without this wrapper.
exec 0<&- 1>&-

# wait returns early when a trapped signal arrives; keep waiting until the
# child has really been reaped.
while :; do
  wait "$child"
  status=$?
  kill -0 "$child" 2>/dev/null || break
done
wait "$tee_pid" 2>/dev/null
rm -f "$fifo"

if [ "$status" -eq 0 ]; then
  rm -f "$stderr_file"
  exit 0
fi

signal=""
if [ "$status" -gt 128 ]; then
  signal="SIG$(kill -l $((status - 128)) 2>/dev/null)"
fi
args=$(printf ' %q' "$@")
record="$log_dir/exit-$(date -u +%Y%m%dT%H%M%SZ)-$$.log"
{
  echo "=== esbuild failed at $(date -u +%Y-%m-%dT%H:%M:%SZ) (started $started_at, ran ${SECONDS}s)"
  echo "binary:      $real"
  echo "wrapper pid: $$ (parent $PPID)"
  echo "child pid:   $child"
  echo "cwd:         $PWD"
  echo "args:       $args"
  echo "exit status: $status${signal:+ (killed by $signal)}"
  echo "forwarded:  ${forwarded:- none}"
  echo "--- last 50 lines of stderr ---"
  tail -n 50 "$stderr_file" 2>/dev/null
  echo "--- end ---"
} > "$record.tmp" 2>/dev/null && mv -f "$record.tmp" "$record"
rm -f "$stderr_file"
exit "$status"
