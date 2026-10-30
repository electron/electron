# Benchmarks

Scripts that measure Electron's performance with
[crossbench](https://chromium.googlesource.com/crossbench), Chromium's
benchmark runner. The [Benchmarks workflow](../../.github/workflows/benchmarks.yml)
runs them every day against the latest nightly.

Every run is a **paired comparison on one machine**. CI runners are shared cloud
VMs that change hosts between jobs, so raw numbers from different runs are not
comparable. Instead, one job downloads every build it compares and runs them
interleaved (A, B, C, B, C, A, ...) on the same runner:

* **candidate vs baseline**: the latest nightly against the previous one, or
  any two versions when dispatched by hand. This catches regressions.
* **candidate vs Chrome for Testing**: the same Chromium version in Chrome.
  The difference is Electron's own overhead.

## What is measured

| Suite | Stories | Metrics |
| --- | --- | --- |
| `loading` | `first_load`, `reload` (warm process, same page) | FCP and `loadEventEnd` in ms; PSS in MiB after `first_load` (total, browser, renderer and GPU processes) |
| `trace` | `first_load`, `resize` (Electron only), traced | FCP and LCP in ms; resize latency median and p90 in ms |
| `file_loading` | 1,500 small classic scripts and ES modules, loaded through `file://`, `protocol.registerFileProtocol`, `protocol.handle` + `fs`, and `protocol.handle` + `net.fetch` | Navigation start to "all files evaluated" in ms, and the ratio of each custom protocol to `file://` |
| `speedometer` | Speedometer 3.1 | Score |
| `jetstream` | JetStream 2.2 | Score |

Every measure comes from one of crossbench's probes:

* `performance.entries` reads paint timing (FCP), navigation timing and
  `performance.mark()` entries from the page after each story. The
  file-loading pages mark `files-loaded` themselves once every file has run,
  because only the page knows when that is.
* `meminfo` reads `/proc/<pid>/smaps_rollup` for every browser process, through
  a `meminfo` action at the end of `first_load`, after the timing.
* `tracing` records a Chrome-style startup trace (`--trace-startup`) with only
  the categories the metrics need (`TRACE_CATEGORIES` in
  [`lib/configs.mts`](lib/configs.mts)), and `trace_processor` runs
  `TRACE_QUERIES` on each trace. A perfetto system trace (the `perfetto` probe)
  is not used: it misses renderer events from before the renderer connects to
  `traced`, including FCP.
  * FCP and LCP: from the main frame's `navigationStart` to
    `firstContentfulPaint` and to the last `largestContentfulPaint::Candidate`.
  * Resize: `BrowserWindow.setSize()` from the browser's
    `Widget::OnNativeWidgetSizeChanged` to the presentation of the first
    renderer frame begun after it (the end of its `PipelineReporter`), 20 times
    per run.

Tracing slows the browser down, so the traced stories run in their own `trace`
invocation and the `loading` and `file_loading` numbers are never traced. The
`trace` suite's FCP is a cross-check of the untraced one and the only source of
LCP, which `performance.entries` does not read.

The press benchmarks are served locally from the WebKit revisions pinned in
[`script/pgo/serve-benchmarks.js`](../pgo/serve-benchmarks.js), so they match
what PGO trains on and nothing is fetched from the network during a run.
MotionMark is not included yet: the revision PGO pins no longer exposes the
`window.benchmarkRunnerClient` global that crossbench's MotionMark probe reads.

Chrome for Testing has no harness, so it skips `resize`, and it only runs the
`file://` classic-script story because Chrome blocks module scripts from
`file://`.

Other crossbench probes are left out on purpose. `system.stats` forks `ps`
every 100 ms for the whole run, which disturbs the timings, and only records
system-wide text snapshots. `v8.log` and `profiling` (`perf`) are heavy
diagnostics with no single number to compare, and `frequency` needs control of
the CPU governor, which the CI VMs don't expose.

## How it works

* [`fetch-targets.mts`](fetch-targets.mts) resolves the versions from
  [releases.electronjs.org](https://releases.electronjs.org) with
  [`@electron/fiddle-core`](https://github.com/electron/fiddle-core), installs
  each Electron build with fiddle-core's `Installer` and downloads the matching
  `chromedriver` with [`@electron/get`](https://github.com/electron/get). Both
  are checked against the release's `SHASUMS256.txt`. It then downloads Chrome
  for Testing at the candidate's Chromium version, and writes a crossbench
  browser config per target and `targets.json`. Builds are for the host's
  platform and architecture.
* [`run.mts`](run.mts) installs crossbench at the pinned revision
  (`CROSSBENCH_REVISION`) into a virtualenv, serves the pages and press
  benchmarks, generates the page configs, and runs the rounds. It records the
  CPU model, kernel, runner name and (on Azure) VM size in `metadata.json`.
  Each crossbench invocation runs in its own process group, and its whole
  process tree (chromedriver and the browser included) is killed when it
  times out or exits, so nothing lingers into the next target's run. Every
  round runs even when an invocation fails, but any failure makes `run.mts`
  exit non-zero.
* [`compare.mts`](compare.mts) takes one sample per round and target (the
  median of that round's repetitions) and compares the medians. A change is
  flagged when Welch's t-test and the Mann-Whitney U test both give
  p < 0.05 **and** the medians differ by at least 10%. It writes
  `comparison.json` and a Markdown summary, and sends the medians
  (`electron.perf.<metric>`) and relative changes in percent
  (`electron.perf.delta.<metric>`) to Datadog. Only invocations that exited
  with 0 and reported every repetition are used. It exits non-zero when there
  are no usable results, or when candidate vs baseline (or vs Chrome for
  Testing, unless it was skipped) has no comparable metric.
* [`harness-app/`](harness-app) is the Electron app crossbench launches. It
  registers one custom scheme per file-loading strategy and exposes
  `cbHarness.setSize()` to the page. Features an older Electron version lacks
  are reported by `--cb-capabilities` and their stories are skipped.

crossbench has no Electron browser type yet, so the browser configs use a
workaround: `type: "chromium"`, an explicit Chromium `version`, Electron's own
`chromedriver` as the `driver`, and `--app=<harness-app>` as a flag. Runs must
use `--headless`, because crossbench's window setup calls
`Browser.getWindowForTarget`, which Electron does not implement. Custom schemes
are navigated with JavaScript from `about:blank`, because crossbench's `get`
action only accepts well-known schemes.

## Running locally

Linux x64 only for now. You need Node.js 22.18 or newer (the scripts are
TypeScript, run with Node's built-in type stripping), the repository's
dependencies (`yarn install`), Python 3.11 or 3.12, `git`, `unzip`, `xvfb-run`
and `xrandr`:

```sh
node script/benchmarks/fetch-targets.mts --out /tmp/bench/targets \
  --candidate 47.0.0-nightly.20261028 --baseline 47.0.0-nightly.20261027
xvfb-run -a node script/benchmarks/run.mts \
  --targets /tmp/bench/targets/targets.json --out /tmp/bench/results \
  --rounds 3 --suites loading,trace,file_loading --python python3.12
node script/benchmarks/compare.mts --results /tmp/bench/results --dry-run
```

`--dry-run` prints the Datadog payload instead of sending it. Leave out
`--candidate` and `--baseline` to compare the two latest nightlies, and pass
`--cft none` to skip Chrome for Testing. The tests run with
`node --test script/benchmarks/test/*.test.mts`.

## Caveats

* The thresholds are not calibrated. Treat the first weeks of results as a
  measurement of the noise floor before alerting on them.
* With fewer than four rounds the Mann-Whitney U test can't reach p < 0.05,
  so short runs never flag anything. The workflow defaults to 10 rounds.
* Runs are headless, so paint and resize timings don't include a real
  compositor or GPU.
* The trace queries depend on Chromium trace event names
  (`Widget::OnNativeWidgetSizeChanged`, `PipelineReporter`,
  `firstContentfulPaint`). If a Chromium roll renames one, the `trace` suite
  reports no values for that metric and `compare.mts` lists it as incomplete.
* Only Linux x64 runs today. Windows and macOS need more callers of
  `pipeline-segment-benchmarks.yml`, plus the Windows desktop workarounds from
  `pipeline-segment-pgo-collect.yml`.
* The harness uses `node:` imports, so it needs Electron 15 or newer. It stays
  plain JavaScript because every Electron version under test loads it with
  `--app`, including ones whose Node.js can't strip types.
