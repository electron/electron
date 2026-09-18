#!/bin/bash
# usage: run_perf.sh <outfile> [extra electron args]
out=$1; shift
export BENCH_EXIT_WHEN_DONE=200 BENCH_USER_DATA=$(dirname $0)/userdata/base ELECTRON_BENCH_STAMPS=1
exec perf record -F 9000 -k CLOCK_MONOTONIC -g -o "$out" -- ${ELECTRON_BIN:-out/Release/electron} "$@" $(dirname $0)/app
