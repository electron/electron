# Startup benchmark harness (Linux)

Measures process spawn -> first paint of a red `data:` URL page, with an Xvfb
pixel probe as ground truth plus app-side milestones and (when the build has
`shell/common/bench_stamp.h` stamps) native `BENCHTS:` timestamps.

```
bash mkdist.sh <out/Release> ./dist          # stripped, dist-like layout, app at resources/app
xvfb-run -a -s "-screen 0 1280x800x24" dbus-run-session -- \
  python3 run.py --electron ./dist/electron --runs 20 --warmup 2 \
  --label base --out results.jsonl --user-data ./userdata -- [extra electron args]
python3 trace_an.py startup_trace.json --until 260   # for --trace-startup-format=json traces
python3 perf_an.py perf.script perf.stdout            # perf script text + BENCHTS stdout
```

Environment switches in the instrumented build: `ELECTRON_BENCH_STAMPS=1`,
`ELECTRON_EXP_LATE_LINUXUI=1` (GTK/LinuxUi init moved after child process
launch), `ELECTRON_EXP_EARLY_SPARE=1` (spare renderer warmed before `ready`).
