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

// Each workload: a URL and a JS expression that evaluates to true when done.
// Iteration counts are chosen so each workload contributes meaningful counter
// volume without making CI runs excessively long.
const WORKLOADS = [
  {
    name: 'speedometer3-run1',
    url: `${BASE_URL}/speedometer/?startAutomatically=true&iterationCount=10`,
    doneExpr: 'window.benchmarkClient && window.benchmarkClient._hasResults === true',
    timeoutMin: 30
  },
  {
    name: 'speedometer3-run2',
    url: `${BASE_URL}/speedometer/?startAutomatically=true&iterationCount=10`,
    doneExpr: 'window.benchmarkClient && window.benchmarkClient._hasResults === true',
    timeoutMin: 30
  },
  {
    name: 'speedometer3-run3',
    url: `${BASE_URL}/speedometer/?startAutomatically=true&iterationCount=10`,
    doneExpr: 'window.benchmarkClient && window.benchmarkClient._hasResults === true',
    timeoutMin: 30
  },
  {
    name: 'jetstream2',
    url: `${BASE_URL}/jetstream/index.html`,
    // JetStream needs an explicit start call once its driver is ready.
    startExpr: 'typeof JetStream !== "undefined" && JetStream.start && (JetStream.start(), true)',
    doneExpr: 'typeof JetStream !== "undefined" && JetStream.done === true',
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

  // Clean shutdown is what flushes the PGO counters from every process.
  process.exitCode = exitCode;
  app.quit();
});
