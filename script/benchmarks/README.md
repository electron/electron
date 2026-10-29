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
| `loading` | `first_load`, `reload` (warm process, same page), `resize` | FCP, LCP and `loadEventEnd` in ms; resize median and p90 in ms |
| `file_loading` | 1,500 small classic scripts and ES modules, loaded through `file://`, `protocol.registerFileProtocol`, `protocol.handle` + `fs`, and `protocol.handle` + `net.fetch` | Navigation start to "all files evaluated" in ms, and the ratio of each custom protocol to `file://` |
| `speedometer` | Speedometer 3.1 | Score |
| `jetstream` | JetStream 2.2 | Score |

The press benchmarks are served locally from the WebKit revisions pinned in
[`script/pgo/serve-benchmarks.js`](../pgo/serve-benchmarks.js), so they match
what PGO trains on and nothing is fetched from the network during a run.
MotionMark is not included yet: the revision PGO pins no longer exposes the
`window.benchmarkRunnerClient` global that crossbench's MotionMark probe reads.

Resize is timed from `BrowserWindow.setSize()` to the second
`requestAnimationFrame` at the new size. That floors at about two frames, so it
only catches large regressions; a trace-based metric (`SetBounds` to the next
presented frame) will replace it. Chrome for Testing has no harness, so it
skips `resize`, and it only runs the `file://` classic-script story because
Chrome blocks module scripts from `file://`.

## How it works

* [`fetch-targets.mjs`](fetch-targets.mjs) resolves the versions, downloads
  the Electron and matching `chromedriver` zips from the
  [electron/electron](https://github.com/electron/electron/releases) or
  [electron/nightlies](https://github.com/electron/nightlies/releases) releases,
  checks them against the release's `SHASUMS256.txt`, and downloads Chrome for
  Testing at the candidate's Chromium version. It writes a crossbench browser
  config per target and `targets.json`.
* [`run.mjs`](run.mjs) installs crossbench at the pinned revision
  (`CROSSBENCH_REVISION`) into a virtualenv, serves the pages and press
  benchmarks, generates the page configs, and runs the rounds. It records the
  CPU model, kernel, runner name and (on Azure) VM size in `metadata.json`.
* [`compare.mjs`](compare.mjs) takes one sample per round and target (the
  median of that round's repetitions) and compares the medians. A change is
  flagged when Welch's t-test and the Mann-Whitney U test both give
  p < 0.05 **and** the medians differ by at least 10%. It writes
  `comparison.json` and a Markdown summary, and sends the medians
  (`electron.perf.<metric>`) and relative changes in percent
  (`electron.perf.delta.<metric>`) to Datadog.
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

Linux x64 only for now. You need Node.js, Python 3.11 or 3.12, `git`, `unzip`,
`xvfb-run` and `xrandr`:

```sh
node script/benchmarks/fetch-targets.mjs --out /tmp/bench/targets \
  --candidate 47.0.0-nightly.20261028 --baseline 47.0.0-nightly.20261027
xvfb-run -a node script/benchmarks/run.mjs \
  --targets /tmp/bench/targets/targets.json --out /tmp/bench/results \
  --rounds 3 --suites loading,file_loading --python python3.12
node script/benchmarks/compare.mjs --results /tmp/bench/results --dry-run
```

`--dry-run` prints the Datadog payload instead of sending it. Leave out
`--candidate` and `--baseline` to compare the two latest nightlies, and pass
`--cft none` to skip Chrome for Testing. The comparator's tests run with
`node --test script/benchmarks/test/*.test.mjs`.

## Caveats

* The thresholds are not calibrated. Treat the first weeks of results as a
  measurement of the noise floor before alerting on them.
* With fewer than four rounds the Mann-Whitney U test can't reach p < 0.05,
  so short runs never flag anything. The workflow defaults to 10 rounds.
* Runs are headless, so paint and resize timings don't include a real
  compositor or GPU.
* Only Linux x64 runs today. Windows and macOS need more callers of
  `pipeline-segment-benchmarks.yml`, plus the Windows desktop workarounds from
  `pipeline-segment-pgo-collect.yml`.
* The harness uses `node:` imports, so it needs Electron 15 or newer.
