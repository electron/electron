# Benchmarks

Scripts that measure Electron's performance with
[crossbench](https://chromium.googlesource.com/crossbench), Chromium's
benchmark runner. The [Benchmarks workflow](../../.github/workflows/benchmarks.yml)
runs them every day against the latest nightly. A scheduled run is skipped
when the latest nightly's GitHub release was published more than 25 hours
earlier, so each nightly is normally benchmarked once, by the first daily run
after it is published; a nightly published within an hour before a run can be
benchmarked twice.

Every run is a **paired comparison on one machine**. CI runners are shared cloud
VMs that change hosts between jobs, so raw numbers from different runs are not
comparable. Instead, one job downloads both Electron builds it compares, a
candidate and a baseline, and runs them interleaved (A, B, B, A, ...) on the
same runner. By default the candidate is the latest nightly and the baseline
the nightly before it; any two versions can be compared when the workflow is
dispatched by hand. This catches regressions between Electron versions.

## What is measured

| Suite | Stories | Metrics |
| --- | --- | --- |
| `loading` | `first_load`, `reload` (warm process, same page) | FCP and `loadEventEnd` in ms; PSS in MiB after `first_load` (total, browser, renderer and GPU processes) |
| `trace` | `first_load`, `resize`, traced | FCP and LCP in ms; resize latency median and p90 in ms |
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
    `Widget::OnNativeWidgetSizeChanged` to the presentation (the end of the
    `PipelineReporter`) of the first renderer frame whose main frame committed
    after the page's `resize` event, 20 times per run. Frames are matched by
    their commit because a `PipelineReporter` starts at its BeginFrame
    timestamp, which can be earlier than the resize the frame shows.

Tracing slows the browser down, so the traced stories run in their own `trace`
invocation and the `loading` and `file_loading` numbers are never traced. The
`trace` suite's FCP is a cross-check of the untraced one and the only source of
LCP, which `performance.entries` does not read.

The press benchmarks are served locally from the WebKit revisions pinned in
[`script/pgo/serve-benchmarks.js`](../pgo/serve-benchmarks.js), so they match
what PGO trains on and nothing is fetched from the network during a run.
MotionMark is not included yet: the revision PGO pins no longer exposes the
`window.benchmarkRunnerClient` global that crossbench's MotionMark probe reads.

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
  are checked against the release's `SHASUMS256.txt`. It then writes a
  crossbench browser config per target and `targets.json`. Builds are for the
  host's platform and architecture.
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
  (`electron.perf.delta.<metric>`) to Datadog. For alerting it also sends two
  0/1 gauges per metric and comparison, `electron.perf.regression` and
  `electron.perf.improvement`, which are 1 when that comparison flags the
  metric that way. Every compared metric reports, flagged or not, so a flag
  clears on the next run. They are tagged `metric`, `benchmark`, `comparison`,
  `platform`, `arch`, `electron_version` and `baseline_version`, and nothing
  else, so a monitor can group them by `metric` and `comparison`. Only
  invocations that exited with 0 and reported every repetition are used. It
  exits non-zero when there are no usable results, or when candidate vs
  baseline has no comparable metric.
* [`harness-app/`](harness-app) is the Electron app crossbench launches. It
  registers one custom scheme per file-loading strategy and exposes
  `cbHarness.setSize()` to the page. Features an older Electron version lacks
  (for example `protocol.handle` before Electron 25) are reported by
  `--cb-capabilities` and their stories are skipped. The capabilities are
  probed after the app is ready, because before Electron 25 the `protocol`
  module has no methods until then.

crossbench has no Electron browser type yet, so the browser configs use a
workaround: `type: "chromium"`, an explicit Chromium `version`, Electron's own
`chromedriver` as the `driver`, and `--app=<harness-app>` as a flag. Runs must
use `--headless`, because crossbench's window setup calls
`Browser.getWindowForTarget`, which Electron does not implement. Custom schemes
are navigated with JavaScript from `about:blank`, because crossbench's `get`
action only accepts well-known schemes.

## Stored results

Datadog is the live view of the results; the `benchmarks` blob container is
the permanent record. Every run that sends to Datadog also uploads each job's
`metadata.json`, `comparison.json` and `summary.md` (not the raw rounds and
traces, which stay in the run's artifacts for 30 days) to
`https://dev-cdn.electronjs.org/benchmarks/`, at
`nightly/<YYYY-MM-DD>/<run id>-<attempt>/<platform>-<arch>/` for the nightly
workflow (the date is the day the run started, in UTC) and
`history/<anchor>/<major>/<run id>-<attempt>/<platform>-<arch>/` for the
history workflow. The upload is a separate `upload-results` job in the
`benchmarks-upload` environment, so the jobs that run Electron builds never
get its credentials. Existing files are never overwritten.

## History across major versions

The [Benchmarks History workflow](../../.github/workflows/benchmarks-history.yml)
measures how performance changed across Electron majors. It is dispatched by
hand and is meant as a one-time backfill, not an ongoing comparison. Every run
must use the same anchor, 45.3.0 (the default), so that ratios stay comparable
across runs. When a new major goes stable, dispatch the workflow with just that
major (e.g. `majors=46`) to add it to the series. [`resolve-history.mts`](resolve-history.mts) resolves every requested
major to its latest stable release with fiddle-core, and the workflow runs one
job per major (six at a time) on the same runner type and container as the
nightly run. In each job the candidate is that major's release and the
baseline is a fixed **anchor** version, run interleaved as usual.

The jobs run on different hosts, so their raw values can't be compared with
each other. Their ratios to the anchor can: each one is measured on a single
host, and the anchor is the same in every job. Each job sends:

* `electron.perf.history.ratio.<metric>`: the candidate's median divided by
  the anchor's. This is the number to chart across majors. Below 1 is faster
  (or smaller) than the anchor for times and memory, and worse for the
  Speedometer and JetStream scores.
* `electron.perf.history.<metric>`: the candidate's raw median, for context
  only.

Both are tagged `electron_major`, `electron_version`, `chromium_version`,
`anchor_version`, `history_run_id` (the workflow run's id), `benchmark`,
`platform`, `arch` and `cpu_model`. Filter a dashboard on
`anchor_version:45.3.0` so ratios from runs with different anchors are not
mixed.
No regression gauges or nightly series are sent, so history runs never
trigger the nightly monitors.

| Input | Default | |
| --- | --- | --- |
| `majors` | `15-latest` | Ranges and lists, e.g. `30-45` or `22,28,33-latest`. `latest` is the newest major with a stable release; majors without one are left out. |
| `anchor` | `45.3.0` | Any released version, or `latest-stable`. Keep the default so runs stay comparable. |
| `rounds` | `5` | Interleaved rounds per job. |
| `suites` | all five | Same as nightly. |
| `upload-to-datadog` | `true` | Also saves the summaries to blob storage (see [Stored results](#stored-results)). Unlike nightly, because the data is the point of the run. Turn it off for a trial run. |

Old versions are run with `run.mts --best-effort --timeout-minutes 15`:
stories whose APIs a version lacks are skipped (see the harness capabilities
above), a suite that fails for the candidate in both of the first two rounds
is not run again, and a job only fails when nothing could be measured. Which
suites were dropped is in each job's step summary and `metadata.json`
(`skippedSuites`).

Electron 15 is the oldest major the default range includes: the harness
uses `node:` imports, which the Node.js in Electron 14 and older lacks. What
older versions can run, from one round each on Ubuntu 24.04 under Xvfb:

* `loading` ran on every major tried (15, 18, 20, 22, 25, 28, 30, 33 and
  36), and `speedometer` and `jetstream` on every major they were tried on
  (15, 22, 25 and 30).
* `file_loading`: `protocol.handle` arrived in Electron 25, so older majors
  only run the `file://` and `registerFileProtocol` stories, and their
  `ratio_to_file` only for `registerFileProtocol`.
* `trace`: FCP and LCP from Electron 22 (Electron 15's traces have none of
  the events the queries need), and resize latency only from Electron 40
  (36 and older report none; 37 to 39 were not tried). Missing values are
  listed in the summary as incomplete.
* Memory: on some versions (36 in testing) the GPU and renderer processes
  keep the zygote's command line, so their PSS can't be told apart and reads
  as 0. History runs leave out values that are not positive.

A job takes about an hour: the nightly run takes about ten minutes a round on
the 32-core runner, and old versions run the press benchmarks more slowly.
The default range is 31 jobs, so a full run uses about 30 runner-hours and
takes five to six hours with six jobs at a time.

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
`--candidate` and `--baseline` to compare the two latest nightlies. For one
job of a history run, pass the major's release as `--candidate` and the
anchor as `--baseline`, add `--best-effort` to `run.mts` and `--history` to
`compare.mts`. The tests
run with `node --test script/benchmarks/test/*.test.mts`.

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
