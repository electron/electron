// PGO profile collection driver.
//
// This app runs inside the *instrumented* Electron build (chrome_pgo_phase = 1)
// and drives the benchmark workloads that train the profile. It mirrors the
// benchmark set Chromium's own PGO bots use (tools/pgo/generate_profile.py):
// Speedometer 3, JetStream 2, and MotionMark.
//
// Design notes:
//  - The app quits itself with app.quit() when every workload has finished.
//    A clean process exit is REQUIRED for profile data to be written: child
//    processes dump their counters when the browser process coordinates a
//    normal shutdown. Killing the process loses all profile data.
//  - Workload completion is detected by polling well-known globals inside the
//    page (each benchmark exposes its runner state on `window`).
//  - Every BrowserWindow uses default webPreferences so the renderer
//    configuration matches what apps ship.
//
// Environment:
//   PGO_BENCHMARK_BASE_URL  base URL of the local benchmark server
//                           (default http://127.0.0.1:8765)
//   PGO_RESULTS_FILE        where to write a JSON summary of what ran
const { app, BrowserWindow } = require('electron');
const fs = require('node:fs');
const path = require('node:path');

const BASE_URL = process.env.PGO_BENCHMARK_BASE_URL || 'http://127.0.0.1:8765';
const RESULTS_FILE = process.env.PGO_RESULTS_FILE || path.join(app.getPath('temp'), 'pgo-benchmark-results.json');

// Each workload:
//   url        - what to load
//   startExpr  - (optional) expression that starts the benchmark; it must
//                return false (without side effects) until the page is ready
//                to start, and true once the benchmark has been started.
//   doneExpr   - expression that evaluates to true when the run has ended
//                (successfully or not).
//   successExpr- (optional) evaluated after doneExpr fires; distinguishes a
//                successful run from an error state. Defaults to success.
// Iteration counts are chosen so each workload contributes meaningful counter
// volume without making CI runs excessively long.
const SPEEDOMETER_DONE = 'window.benchmarkClient && window.benchmarkClient._hasResults === true';
// _hasResults is set on both completion and error; real success means metrics
// were computed.
const SPEEDOMETER_SUCCESS = '!!(window.benchmarkClient && window.benchmarkClient.metrics && window.benchmarkClient.metrics.Score)';

const WORKLOADS = [
  {
    name: 'speedometer3-run1',
    url: `${BASE_URL}/speedometer/?startAutomatically=true&iterationCount=10`,
    doneExpr: SPEEDOMETER_DONE,
    successExpr: SPEEDOMETER_SUCCESS,
    timeoutMin: 30
  },
  {
    name: 'speedometer3-run2',
    url: `${BASE_URL}/speedometer/?startAutomatically=true&iterationCount=10`,
    doneExpr: SPEEDOMETER_DONE,
    successExpr: SPEEDOMETER_SUCCESS,
    timeoutMin: 30
  },
  {
    name: 'speedometer3-run3',
    url: `${BASE_URL}/speedometer/?startAutomatically=true&iterationCount=10`,
    doneExpr: SPEEDOMETER_DONE,
    successExpr: SPEEDOMETER_SUCCESS,
    timeoutMin: 30
  },
  {
    name: 'jetstream2',
    url: `${BASE_URL}/jetstream/index.html`,
    // The JetStream driver is ready once its async initialize() has run
    // prepareToRun(), which replaces the "Loading..." status with the
    // "Start Test" button. Calling start() before that point does nothing
    // useful, so gate on the button existing.
    startExpr: '!!document.querySelector("#status a.button") && (JetStream.start(), true)',
    // On completion the driver adds the "done" class to #result-summary
    // (JetStreamDriver.js); there is no JetStream.done property.
    doneExpr: '!!document.querySelector("#result-summary.done")',
    timeoutMin: 45
  },
  {
    name: 'motionmark',
    url: `${BASE_URL}/motionmark/MotionMark/index.html`,
    startExpr: 'typeof benchmarkController !== "undefined" && (benchmarkController.startBenchmark(), true)',
    // MotionMark shows the results section when the run completes.
    doneExpr: '!!document.querySelector("section#results.selected")',
    timeoutMin: 45
  }
];

function log (...args) {
  // eslint-disable-next-line no-console
  console.log('[pgo-benchmark]', ...args);
}

async function sleep (ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function runWorkload (win, workload) {
  log(`starting workload: ${workload.name}`);
  const startTime = Date.now();
  await win.loadURL(workload.url);

  // Some benchmarks need an explicit start once their driver has loaded.
  if (workload.startExpr) {
    let started = false;
    for (let i = 0; i < 60 && !started; i++) {
      await sleep(1000);
      try {
        started = await win.webContents.executeJavaScript(workload.startExpr, true);
      } catch { /* page busy - retry */ }
    }
    if (!started) throw new Error(`workload ${workload.name} never became startable`);
  }

  // Poll for completion.
  const deadline = Date.now() + workload.timeoutMin * 60 * 1000;
  while (Date.now() < deadline) {
    await sleep(5000);
    let done = false;
    try {
      done = await win.webContents.executeJavaScript(workload.doneExpr, true);
    } catch { /* page busy running benchmark - retry */ }
    if (done) {
      const elapsed = ((Date.now() - startTime) / 1000).toFixed(0);
      // The run ended - check whether it ended in success or in an error
      // state (e.g. Speedometer sets _hasResults on errors too).
      let succeeded = true;
      if (workload.successExpr) {
        try {
          succeeded = await win.webContents.executeJavaScript(workload.successExpr, true);
        } catch {
          succeeded = false;
        }
      }
      if (!succeeded) {
        throw new Error(`workload ${workload.name} ended in an error state after ${elapsed}s`);
      }
      log(`finished workload: ${workload.name} in ${elapsed}s`);
      return { name: workload.name, ok: true, seconds: Number(elapsed) };
    }
  }
  throw new Error(`workload ${workload.name} timed out after ${workload.timeoutMin} minutes`);
}

app.whenReady().then(async () => {
  const win = new BrowserWindow({
    width: 1200,
    height: 900,
    useContentSize: true,
    show: true
  });

  const results = [];
  let exitCode = 0;
  for (const workload of WORKLOADS) {
    try {
      results.push(await runWorkload(win, workload));
    } catch (err) {
      log(`ERROR: ${err.message}`);
      results.push({ name: workload.name, ok: false, error: err.message });
      // A failed workload reduces profile coverage but the data collected so
      // far is still valid - keep going and report partial failure.
      exitCode = 1;
    }
  }

  fs.writeFileSync(RESULTS_FILE, JSON.stringify(results, null, 2));
  log(`results written to ${RESULTS_FILE}`);

  // Clean shutdown is what flushes the PGO counters from every process. Note
  // that app.quit() always exits 0 regardless of process.exitCode (app.exit()
  // would honor a code but skips the clean shutdown we need), so failures are
  // reported through RESULTS_FILE - collect-profile.js reads it and sets the
  // CI exit code.
  if (exitCode !== 0) log('one or more workloads failed - see results file');
  app.quit();
});
