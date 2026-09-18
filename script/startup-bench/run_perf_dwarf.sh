#!/bin/bash
out=$1; shift
export BENCH_EXIT_WHEN_DONE=200 BENCH_USER_DATA=$(dirname $0)/userdata/base ELECTRON_BENCH_STAMPS=1
exec perf record -N -B -F 5000 -k CLOCK_MONOTONIC --call-graph dwarf,16384 -o "$out" -- ${ELECTRON_BIN:-out/Release/electron} "$@" $(dirname $0)/app
