// PGO profile collection driver.
//
// This app runs inside the *instrumented* Electron build (chrome_pgo_phase = 1)
// and drives the workloads that train the profile.
//
// Workload phases:
//   1. Browser benchmarks - Speedometer 3 (x3), JetStream 2, MotionMark; the
//      same set Chromium's own PGO bots use (tools/pgo/generate_profile.py).
//   2. Main-process workload - Node.js Buffer/crypto/fs/JSON loops. Real apps
//      run significant main-process code that browser benchmarks never touch;
//      without this phase those paths are cold-marked by the profile (measured
//      at -63% on Buffer operations).
//   3. IPC + contextBridge workload - bridge marshaling and ipcRenderer.invoke
//      round trips at multiple payload sizes. contextBridge is Electron's
//      hottest app-facing native code and is invisible to browser benchmarks.
//   4. Network workload - renderer fetch/WebSocket loops plus Node-side HTTPS,
//      training Chromium's TLS/H2/network-service code and Node's separate
//      TLS/HTTP stack.
//
// Design notes:
//  - The app quits itself with app.quit() when every workload has finished.
//    A clean process exit is REQUIRED for profile data to be written: child
//    processes dump their counters when the browser process coordinates a
//    normal shutdown. Killing the process loses all profile data.
//  - Workload completion is detected by polling well-known globals inside the
//    page (each benchmark exposes its runner state on `window`).
//  - Benchmark BrowserWindows use default webPreferences so the renderer
//    configuration matches what apps ship; the bridge workload window uses
//    contextIsolation + a preload, matching Electron's security defaults.
//
// Environment:
//   PGO_BENCHMARK_BASE_URL  base URL of the local benchmark server
//                           (default http://127.0.0.1:8765)
//   PGO_RESULTS_FILE        where to write a JSON summary of what ran
//   PGO_WORKLOAD_FILTER     comma-separated list of workload names to run
//                           (default: all). Useful for debugging a single
//                           phase without waiting for the full set.
const { app, BrowserWindow, ipcMain } = require('electron');
const crypto = require('node:crypto');
const fs = require('node:fs');
const https = require('node:https');
const os = require('node:os');
const path = require('node:path');

const BASE_URL = process.env.PGO_BENCHMARK_BASE_URL || 'http://127.0.0.1:8765';
const RESULTS_FILE = process.env.PGO_RESULTS_FILE || path.join(app.getPath('temp'), 'pgo-benchmark-results.json');

// ---------------------------------------------------------------------------
// Phase 1: browser benchmarks
// ---------------------------------------------------------------------------

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
const SPEEDOMETER_SUCCESS =
  '!!(window.benchmarkClient && window.benchmarkClient.metrics && window.benchmarkClient.metrics.Score)';

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

function log(...args) {
  // eslint-disable-next-line no-console
  console.log('[pgo-benchmark]', ...args);
}

async function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// ---------------------------------------------------------------------------
// Failure diagnostics
//
// Workload failures on CI are otherwise nearly undebuggable - official builds
// crash without messages and a hung page looks identical to a slow one. These
// observers record enough state to tell the difference: per-resource network
// accounting (completed / failed / still pending), renderer crashes with
// their reason, renderer hangs, and a memory + DOM snapshot at failure time.
// ---------------------------------------------------------------------------

function attachDiagnostics(win) {
  const diag = {
    requests: new Map(),
    consoleErrors: [],
    rendererGone: null,
    unresponsive: false,
    reset() {
      this.requests.clear();
      this.consoleErrors = [];
      this.rendererGone = null;
      this.unresponsive = false;
    }
  };

  win.webContents.on('render-process-gone', (_event, details) => {
    diag.rendererGone = details;
    log(`DIAGNOSTIC: renderer process gone: ${JSON.stringify(details)}`);
  });
  win.webContents.on('unresponsive', () => {
    diag.unresponsive = true;
    log('DIAGNOSTIC: renderer became unresponsive');
  });
  win.webContents.on('console-message', (event, legacyLevel, legacyMessage) => {
    const level = typeof event.level === 'string' ? event.level : legacyLevel;
    const message = event.message !== undefined ? event.message : legacyMessage;
    if (level === 'error' || level === 3) {
      diag.consoleErrors.push(String(message).slice(0, 500));
    }
  });

  // Non-blocking observers: track every resource the page requests so a
  // failure report can say exactly which loads never finished.
  const filter = { urls: ['*://*/*'] };
  const { webRequest } = win.webContents.session;
  webRequest.onSendHeaders(filter, (details) => {
    diag.requests.set(details.url, 'pending');
  });
  webRequest.onCompleted(filter, (details) => {
    diag.requests.set(details.url, 'completed');
  });
  webRequest.onErrorOccurred(filter, (details) => {
    diag.requests.set(details.url, `failed: ${details.error}`);
  });

  return diag;
}

async function reportWorkloadFailure(win, workloadName, diag) {
  log(`=== failure diagnostics for ${workloadName} ===`);

  if (diag.rendererGone) {
    log(`renderer process gone: ${JSON.stringify(diag.rendererGone)}`);
  }
  if (diag.unresponsive) log('renderer is unresponsive (main thread hung)');

  const entries = [...diag.requests.entries()];
  const pending = entries.filter(([, s]) => s === 'pending');
  const failed = entries.filter(([, s]) => String(s).startsWith('failed'));
  log(
    `resources: ${entries.length} requested, ` +
      `${entries.length - pending.length - failed.length} completed, ` +
      `${failed.length} failed, ${pending.length} never finished`
  );
  for (const [url, status] of failed.slice(0, 15)) log(`  ${status}: ${url}`);
  for (const [url] of pending.slice(0, 15)) log(`  never finished: ${url}`);

  if (diag.consoleErrors.length > 0) {
    log(`console errors (${diag.consoleErrors.length}):`);
    for (const message of diag.consoleErrors.slice(0, 15)) log(`  ${message}`);
  }

  // Memory + DOM snapshot, best effort: a hung renderer may never respond,
  // so give it a strict deadline rather than hanging the collection further.
  try {
    const state = await Promise.race([
      win.webContents.executeJavaScript(
        `JSON.stringify({
          memory: typeof performance !== 'undefined' && performance.memory ? {
            usedJSHeapMB: Math.round(performance.memory.usedJSHeapSize / 1048576),
            heapLimitMB: Math.round(performance.memory.jsHeapSizeLimit / 1048576)
          } : null,
          readyState: document.readyState,
          bodyPreview: document.body ? document.body.innerText.slice(0, 400) : null
        })`,
        true
      ),
      sleep(10000).then(() => {
        throw new Error('renderer did not respond to state query within 10s');
      })
    ]);
    log(`page state: ${state}`);
  } catch (err) {
    log(`could not query page state: ${err.message}`);
  }
}

async function runWorkload(win, workload, diag) {
  log(`starting workload: ${workload.name}`);
  if (diag) diag.reset();
  const startTime = Date.now();
  await win.loadURL(workload.url);

  // Some benchmarks need an explicit start once their driver has loaded.
  // The polling window only needs to fail fast on genuinely broken pages
  // (server died, driver threw - the start control never appears); it costs
  // nothing on healthy runs, which exit the moment the page is ready. It must
  // be generous enough for the slowest legitimate case: JetStream loads ~100
  // test files over TLS, on an instrumented 32-bit binary under WOW64, which
  // exceeds 60 seconds.
  if (workload.startExpr) {
    let started = false;
    for (let i = 0; i < 300 && !started; i++) {
      await sleep(1000);
      try {
        started = await win.webContents.executeJavaScript(workload.startExpr, true);
      } catch {
        /* page busy - retry */
      }
    }
    if (!started) {
      if (diag) await reportWorkloadFailure(win, workload.name, diag);
      throw new Error(`workload ${workload.name} never became startable`);
    }
  }

  // Poll for completion.
  const deadline = Date.now() + workload.timeoutMin * 60 * 1000;
  while (Date.now() < deadline) {
    await sleep(5000);
    let done = false;
    try {
      done = await win.webContents.executeJavaScript(workload.doneExpr, true);
    } catch {
      /* page busy running benchmark - retry */
    }
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
        if (diag) await reportWorkloadFailure(win, workload.name, diag);
        throw new Error(`workload ${workload.name} ended in an error state after ${elapsed}s`);
      }
      log(`finished workload: ${workload.name} in ${elapsed}s`);
      return { name: workload.name, ok: true, seconds: Number(elapsed) };
    }
  }
  if (diag) await reportWorkloadFailure(win, workload.name, diag);
  throw new Error(`workload ${workload.name} timed out after ${workload.timeoutMin} minutes`);
}

// ---------------------------------------------------------------------------
// Phase 2: main-process workload
//
// Trains Node.js code paths that only exist in the main process: Buffer
// operations (node::Buffer + simdutf base64/hex), crypto, synchronous fs, and
// V8 JSON in the Node context. Loops are chunked with setImmediate yields so
// the event loop stays responsive.
// ---------------------------------------------------------------------------

async function runMainProcessWorkload(durationMs) {
  log('starting workload: main-process');
  const startTime = Date.now();
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'pgo-main-workload-'));

  const filePayload = crypto.randomBytes(4096);
  const bufA = Buffer.alloc(16 * 1024, 1);
  const bufB = Buffer.alloc(16 * 1024, 2);
  const deepObj = {
    users: Array.from({ length: 100 }, (_, i) => ({
      id: i,
      name: `user-${i}`,
      tags: ['a', 'b', 'c'],
      prefs: { theme: 'dark', nested: { list: [1, 2, 3], flag: i % 2 === 0 } },
      history: Array.from({ length: 10 }, (_, j) => ({ ts: 1700000000 + j, action: `act-${j}` }))
    }))
  };
  const deepJson = JSON.stringify(deepObj);

  const deadline = Date.now() + durationMs;
  let iterations = 0;
  while (Date.now() < deadline) {
    for (let i = 0; i < 50; i++) {
      // Buffer operations: concat, slice, compare, base64/hex encode + decode.
      const joined = Buffer.concat([bufA, bufB]);
      joined.subarray(1024, 8192);
      bufA.compare(bufB);
      const b64 = joined.toString('base64');
      Buffer.from(b64, 'base64');
      const hex = bufA.toString('hex');
      Buffer.from(hex, 'hex');
      bufA.toString('utf8', 0, 1024);

      // Crypto: hashing, HMAC, random generation.
      crypto.createHash('sha256').update(joined).digest();
      crypto.createHmac('sha256', 'pgo-key').update(filePayload).digest();
      crypto.randomBytes(1024);

      // Synchronous fs: write/read/stat/unlink cycles.
      const f = path.join(tmp, `f${i % 20}.bin`);
      fs.writeFileSync(f, filePayload);
      fs.readFileSync(f);
      fs.statSync(f);

      // V8 JSON in the Node context.
      JSON.parse(deepJson);
      JSON.stringify(deepObj);

      iterations++;
    }
    // Yield so timers/IPC stay serviceable.
    await new Promise((resolve) => setImmediate(resolve));
  }

  fs.rmSync(tmp, { recursive: true, force: true });
  const elapsed = ((Date.now() - startTime) / 1000).toFixed(0);
  log(`finished workload: main-process in ${elapsed}s (${iterations} iterations)`);
  return { name: 'main-process', ok: true, seconds: Number(elapsed) };
}

// ---------------------------------------------------------------------------
// Phase 3: IPC + contextBridge workload
//
// Drives contextBridge marshaling (electron_api_context_bridge.cc) and
// ipcRenderer.invoke round trips at multiple payload sizes from a window
// configured the way real apps are configured (contextIsolation + preload).
// ---------------------------------------------------------------------------

async function runIpcBridgeWorkload(durationMs) {
  log('starting workload: ipc-contextbridge');
  const startTime = Date.now();

  ipcMain.handle('pgo-ping', (_event, payload) => payload);

  const win = new BrowserWindow({
    width: 800,
    height: 600,
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  });
  await win.loadURL('data:text/html,<html><body>pgo ipc workload</body></html>');

  // The driver runs in the main world and calls across the bridge. Payload
  // sizes cover the spectrum apps use: small control messages, medium JSON
  // payloads, and large binary transfers.
  const result = await win.webContents.executeJavaScript(
    `(async () => {
    const small = { id: 1, type: 'msg', body: 'hello world' };
    const medium = { rows: Array.from({ length: 200 }, (_, i) => ({ i, name: 'row-' + i, values: [i, i * 2, i * 3] })) };
    const arr = Array.from({ length: 500 }, (_, i) => ({ i, label: 'item-' + i }));
    const typed = new Uint8Array(64 * 1024).fill(7);
    const big = new Uint8Array(${5 * 1024 * 1024}).fill(7);

    const deadline = Date.now() + ${durationMs};
    let bridgeCalls = 0;
    let ipcCalls = 0;
    while (Date.now() < deadline) {
      // Pure bridge marshaling (no IPC): primitives, objects, arrays, typed arrays.
      for (let i = 0; i < 200; i++) {
        window.pgoBridge.echo(42);
        window.pgoBridge.echo('a string value');
        window.pgoBridge.echo(small);
        window.pgoBridge.echo(arr);
        window.pgoBridge.echo(typed);
        window.pgoBridge.transform(small);
        window.pgoBridge.withCallback(small, (v) => v);
        bridgeCalls += 7;
      }
      // Bridge + IPC round trips at multiple sizes.
      await window.pgoBridge.invoke('pgo-ping', small);
      await window.pgoBridge.invoke('pgo-ping', medium);
      await window.pgoBridge.invoke('pgo-ping', typed);
      await window.pgoBridge.invoke('pgo-ping', big);
      ipcCalls += 4;
    }
    return { bridgeCalls, ipcCalls };
  })()`,
    true
  );

  win.destroy();
  ipcMain.removeHandler('pgo-ping');

  const elapsed = ((Date.now() - startTime) / 1000).toFixed(0);
  log(
    `finished workload: ipc-contextbridge in ${elapsed}s ` +
      `(${result.bridgeCalls} bridge calls, ${result.ipcCalls} ipc round trips)`
  );
  return { name: 'ipc-contextbridge', ok: true, seconds: Number(elapsed) };
}

// ---------------------------------------------------------------------------
// Phase 4: network workload
//
// Renderer side: fetch loops (parallel requests, varied sizes, repeats for
// cache hits) and WebSocket echo against the local benchmark server. When the
// server is HTTPS this trains Chromium's TLS handshake, certificate
// verification, and HTTP/2 session code.
//
// Main-process side: Node https.get and globalThis.fetch (undici) loops -
// Node's TLS/HTTP stack is entirely separate from Chromium's and is used by
// apps that make API calls from the main process. Requires NODE_EXTRA_CA_CERTS
// to point at the collection CA (set by collect-profile.js).
// ---------------------------------------------------------------------------

async function runNetworkWorkload(win, durationMs) {
  log('starting workload: network');
  const startTime = Date.now();
  const isTls = BASE_URL.startsWith('https:');
  const wsUrl = BASE_URL.replace(/^http/, 'ws') + '/__pgo/ws';

  // Renderer: parallel fetches + WebSocket echo. Runs for the first half of
  // the budget; the Node-side loop runs for the second half.
  const rendererBudget = Math.floor(durationMs / 2);
  await win.loadURL(`${BASE_URL}/speedometer/`);
  const rendererResult = await win.webContents.executeJavaScript(
    `(async () => {
    const deadline = Date.now() + ${rendererBudget};
    let requests = 0;
    let wsMessages = 0;

    // Fetch loops: parallel requests at varied sizes plus POST echoes.
    while (Date.now() < deadline) {
      await Promise.all([
        fetch('/__pgo/data?bytes=1024').then(r => r.arrayBuffer()),
        fetch('/__pgo/data?bytes=65536').then(r => r.arrayBuffer()),
        fetch('/__pgo/data?bytes=1048576').then(r => r.arrayBuffer()),
        fetch('/__pgo/echo', { method: 'POST', body: JSON.stringify({ seq: requests, data: 'x'.repeat(2048) }) }).then(r => r.json())
      ]);
      requests += 4;
    }

    // WebSocket echo for a fixed slice of the budget.
    try {
      const ws = new WebSocket('${wsUrl}');
      await new Promise((resolve, reject) => {
        ws.onopen = resolve;
        ws.onerror = () => reject(new Error('websocket failed to connect'));
      });
      const wsDeadline = Date.now() + 10000;
      const payload = 'pgo-websocket-payload-'.repeat(50);
      while (Date.now() < wsDeadline) {
        await new Promise((resolve) => {
          ws.onmessage = resolve;
          ws.send(payload);
        });
        wsMessages++;
      }
      ws.close();
    } catch (err) {
      // WebSocket failure degrades coverage but should not fail collection.
      console.warn('websocket workload failed:', err.message);
    }

    return { requests, wsMessages };
  })()`,
    true
  );

  // Main process: Node-side HTTPS/HTTP requests.
  let nodeRequests = 0;
  const nodeDeadline = Date.now() + Math.floor(durationMs / 2);
  const dataUrl = `${BASE_URL}/__pgo/data?bytes=65536`;
  while (Date.now() < nodeDeadline) {
    // node:https / node:http via the classic API.
    await new Promise((resolve, reject) => {
      const mod = isTls ? https : require('node:http');
      mod
        .get(dataUrl, (res) => {
          res.on('data', () => {});
          res.on('end', resolve);
        })
        .on('error', reject);
    }).catch(() => {});
    // undici fetch (Node's fetch implementation - a separate HTTP stack).
    try {
      const res = await fetch(dataUrl);
      await res.arrayBuffer();
    } catch {
      /* degraded coverage only */
    }
    nodeRequests += 2;
  }

  const elapsed = ((Date.now() - startTime) / 1000).toFixed(0);
  log(
    `finished workload: network in ${elapsed}s ` +
      `(renderer: ${rendererResult.requests} fetches + ${rendererResult.wsMessages} ws messages, node: ${nodeRequests} requests)`
  );
  return { name: 'network', ok: true, seconds: Number(elapsed) };
}

// ---------------------------------------------------------------------------
// Orchestration
// ---------------------------------------------------------------------------

// Durations for the non-browser phases. Long enough for meaningful counter
// volume on instrumented builds (which run at roughly half speed), short
// relative to the 30-50 minute browser benchmark phase.
const MAIN_PROCESS_WORKLOAD_MS = 75 * 1000;
const IPC_BRIDGE_WORKLOAD_MS = 75 * 1000;
const NETWORK_WORKLOAD_MS = 90 * 1000;

app.whenReady().then(async () => {
  const win = new BrowserWindow({
    width: 1200,
    height: 900,
    useContentSize: true,
    show: true
  });

  const results = [];
  let exitCode = 0;

  const diag = attachDiagnostics(win);

  let phases = [
    ...WORKLOADS.map((workload) => () => runWorkload(win, workload, diag)),
    () => runMainProcessWorkload(MAIN_PROCESS_WORKLOAD_MS),
    () => runIpcBridgeWorkload(IPC_BRIDGE_WORKLOAD_MS),
    () => runNetworkWorkload(win, NETWORK_WORKLOAD_MS)
  ];
  let phaseNames = [...WORKLOADS.map((w) => w.name), 'main-process', 'ipc-contextbridge', 'network'];

  if (process.env.PGO_WORKLOAD_FILTER) {
    const allowed = process.env.PGO_WORKLOAD_FILTER.split(',').map((s) => s.trim());
    phases = phases.filter((_, i) => allowed.includes(phaseNames[i]));
    phaseNames = phaseNames.filter((name) => allowed.includes(name));
    log(`workload filter active: ${phaseNames.join(', ')}`);
  }

  for (let i = 0; i < phases.length; i++) {
    try {
      results.push(await phases[i]());
    } catch (err) {
      log(`ERROR: ${err.message}`);
      results.push({ name: phaseNames[i], ok: false, error: err.message });
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

// The bridge workload closes its own window mid-run; never let that quit the app.
app.on('window-all-closed', () => {});
