// Measures how long uv-backed operations take to complete when they are
// started from an otherwise idle process, per process type and per kind of
// JS entry that starts them. Prints a JSON array of
// { proc, context, op, budget, samples } where each sample is the latency in
// ms beyond the op's own delay, or a string describing a failure.
const { app, BrowserWindow, globalShortcut, ipcMain } = require('electron');

const fs = require('node:fs');
const net = require('node:net');
const path = require('node:path');

const { ops, touchRepeatedly, setOtherProcess, cleanup } = require('./ops.js');

const arg = (name, fallback) => {
  const found = process.argv.find((a) => a.startsWith(`--${name}=`));
  return found ? found.slice(name.length + 3) : fallback;
};
const iterations = Number(arg('iterations', 3));
const watchdogMs = Number(arg('watchdog', 1000));
const outPath = arg('out', null);
const only = arg('only', null);
const procs = arg('procs', 'browser,renderer').split(',');
// Run the renderer cases from a preload in an isolated world instead of from
// the page's main world.
const isolated = process.argv.includes('--isolated');

const browserContexts = [
  'uv',
  'task',
  'microtask',
  'nested-loop',
  'started-then-nested',
  'native-event',
  'native-addon-call',
  'native-event-in-nested-loop',
  'native-shortcut'
];
const rendererContexts = ['uv', 'task', 'microtask', 'raf', 'input-event'];

let w;
let nextId = 0;
const waiting = new Map();
let pendingPing = null;
let pendingShortcut = null;

ipcMain.on('uv-wake:result', (_e, id, value) => {
  waiting.get(id)?.(value);
  waiting.delete(id);
});
ipcMain.on('uv-wake:ping', () => {
  const fn = pendingPing;
  pendingPing = null;
  fn?.();
});
// Each process touches the other's watched file, or connects to the other's
// port, on request, so the requester's loop is not woken by the asking.
ipcMain.on('uv-wake:touch', (_e, file) => touchRepeatedly(file));
ipcMain.on('uv-wake:connect', (_e, port) => {
  net.connect(port, '127.0.0.1').on('error', () => {});
});
setOtherProcess({
  touch: (file) => w.webContents.send('uv-wake:touch', file),
  connect: (port) => w.webContents.send('uv-wake:connect', port)
});

ipcMain.on('uv-wake:send-key', () => {
  w.webContents.sendInputEvent({ type: 'keyDown', keyCode: 'A' });
  w.webContents.sendInputEvent({ type: 'keyUp', keyCode: 'A' });
});

// A delay served by Blink in the renderer, so waiting on it does not itself
// give the browser's uv loop a reason to run.
const blinkDelay = (ms) => w.webContents.executeJavaScript(`new Promise(r => window.setTimeout(r, ${ms}))`);

// Testing builds only.
const testing = (() => {
  try {
    return process._linkedBinding('electron_common_testing');
  } catch {
    return null;
  }
})();

const browserTriggers = {
  uv: (fn) => setImmediate(fn),
  task: (fn) => {
    pendingPing = fn;
    w.webContents.send('uv-wake:ping-me', 20);
  },
  microtask: (fn) => {
    blinkDelay(20).then(fn);
  },
  // A gin callback run from the platform's key event dispatch. The key is
  // injected by a tool spawned from the renderer so that this process's loop
  // stays idle; yields no-trigger where that tool is unavailable.
  'native-shortcut': (fn) => {
    pendingShortcut = fn;
    w.webContents.send('uv-wake:spawn', process.env.UV_WAKE_XDOTOOL || 'xdotool', ['key', 'F9']);
  },
  // A task run by a nested run loop entered from JavaScript, like an IPC
  // handled while a synchronous dialog is open.
  'nested-loop': (fn) => {
    if (!testing) return;
    pendingPing = fn;
    w.webContents.send('uv-wake:ping-me', 20);
    testing.runNestedLoopForTesting(500);
  },
  // Work started by a JavaScript frame that then enters a nested run loop
  // itself, like a timer set right before a synchronous dialog.
  'started-then-nested': (fn) => {
    if (!testing) return;
    fn();
    testing.runNestedLoopForTesting(500);
  },
  // JavaScript called from a platform event source rather than a task: the
  // way Electron emits its own events (tray, menu, window), and the way a
  // native module calls a stored function from its own OS callback.
  'native-event': (fn) => {
    testing?.invokeFromNativeSourceForTesting(fn, true);
  },
  'native-addon-call': (fn) => {
    testing?.invokeFromNativeSourceForTesting(fn, false);
  },
  // A native event while a nested run loop entered from JavaScript is open.
  'native-event-in-nested-loop': (fn) => {
    if (!testing) return;
    testing.invokeFromNativeSourceForTesting(fn, true);
    testing.runNestedLoopForTesting(500);
  }
};

const meta = { platform: process.platform, isolated };

function runInBrowser(context, opName) {
  const op = ops[opName];
  // Node.js holds ticks and promise reactions while another of its callbacks is
  // on the stack, a bare call from native code runs no checkpoint at all, and
  // a handle that needs a loop run to reach the kernel, started from a native
  // event while JavaScript blocks in a nested loop, waits for that loop to end
  // (docs/breaking-changes.md).
  const nested = ['nested-loop', 'started-then-nested', 'native-event-in-nested-loop'].includes(context);
  const bareCall = context === 'native-addon-call' || context === 'native-shortcut';
  if (
    (nested && op.deferred) ||
    (bareCall && op.continuation) ||
    (context === 'native-event-in-nested-loop' && op.registers)
  ) {
    return Promise.resolve('skipped');
  }
  return new Promise((resolve) => {
    let triggered = false;
    blinkDelay(watchdogMs).then(() => {
      if (!triggered) resolve('no-trigger');
    });
    browserTriggers[context](() => {
      if (triggered) return;
      triggered = true;
      const t0 = performance.now();
      let settled = false;
      op.run(
        () => {
          if (settled) return;
          settled = true;
          resolve(performance.now() - t0 - op.expect);
        },
        (err) => {
          settled = true;
          resolve(`error: ${err && err.message}`);
        }
      );
      blinkDelay(watchdogMs).then(() => {
        if (settled) return;
        settled = true;
        // Kick the loop so a stuck op has a chance to drain, but do not wait.
        process.activateUvLoop();
        resolve('timeout');
      });
    });
  });
}

function runInRenderer(context, opName) {
  return new Promise((resolve) => {
    const id = nextId++;
    waiting.set(id, resolve);
    w.webContents.send('uv-wake:run', id, context, opName, watchdogMs);
  });
}

async function main() {
  await app.whenReady();
  const webPreferences = isolated
    ? {
        sandbox: false,
        contextIsolation: true,
        preload: path.join(__dirname, 'renderer.js'),
        backgroundThrottling: false
      }
    : { nodeIntegration: true, contextIsolation: false, backgroundThrottling: false };
  w = new BrowserWindow({ width: 300, height: 200, x: 0, y: 0, webPreferences });
  await w.loadFile(path.join(__dirname, 'index.html'));
  globalShortcut.register('F9', () => {
    const fn = pendingShortcut;
    pendingShortcut = null;
    fn?.();
  });

  const results = [];
  const plan = [];
  for (const [proc, contexts] of [
    ['browser', browserContexts],
    ['renderer', rendererContexts]
  ]) {
    if (!procs.includes(proc)) continue;
    for (const c of contexts) for (const o of Object.keys(ops)) plan.push([proc, c, o]);
  }

  for (const [proc, context, op] of plan) {
    if (only && !`${proc}:${context}:${op}`.includes(only)) continue;
    const samples = [];
    for (let i = 0; i < iterations; i++) {
      const sample = proc === 'browser' ? await runInBrowser(context, op) : await runInRenderer(context, op);
      samples.push(typeof sample === 'number' ? Math.round(sample * 10) / 10 : sample);
      if (sample === 'no-trigger' || sample === 'skipped') break;
      // Let the loop go quiet again before the next sample.
      await blinkDelay(20);
    }
    const entry = { proc, context, op, budget: ops[op].budget, samples };
    results.push(entry);
    if (process.env.UV_WAKE_VERBOSE) console.error(JSON.stringify(entry));
  }

  const json = JSON.stringify({ meta, cases: results }, null, outPath ? 2 : 0);
  if (outPath) fs.writeFileSync(outPath, json);
  else fs.writeSync(1, json + '\n');
  cleanup();
  app.exit(0);
}

main().catch((err) => {
  console.error(err);
  app.exit(1);
});
