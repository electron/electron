const { ipcRenderer } = require('electron');

const path = require('node:path');

const { ops } = require(path.join(__dirname, 'ops.js'));

// Blink's timer, which does not touch the uv loop.
const blinkDelay = (ms) => new Promise((resolve) => window.setTimeout(resolve, ms));

const triggers = {
  uv: (fn) => setImmediate(fn),
  task: (fn) => window.setTimeout(fn, 20),
  microtask: (fn) => blinkDelay(20).then(fn),
  raf: (fn) => window.setTimeout(() => requestAnimationFrame(() => fn()), 20),
  'input-event': (fn) => {
    document.addEventListener('keydown', () => fn(), { once: true });
    ipcRenderer.send('uv-wake:send-key');
  }
};

ipcRenderer.on('uv-wake:run', (_e, id, context, opName, watchdogMs) => {
  const op = ops[opName];
  let triggered = false;
  const notTriggered = window.setTimeout(() => {
    if (!triggered) ipcRenderer.send('uv-wake:result', id, 'no-trigger');
  }, watchdogMs);
  triggers[context](() => {
    triggered = true;
    window.clearTimeout(notTriggered);
    const t0 = performance.now();
    let settled = false;
    op.run(
      () => {
        if (settled) return;
        settled = true;
        ipcRenderer.send('uv-wake:result', id, performance.now() - t0 - op.expect);
      },
      (err) => {
        settled = true;
        ipcRenderer.send('uv-wake:result', id, `error: ${err && err.message}`);
      }
    );
    blinkDelay(watchdogMs).then(() => {
      if (settled) return;
      settled = true;
      // Kick the loop so a stuck op has a chance to drain, but do not wait.
      process.activateUvLoop();
      ipcRenderer.send('uv-wake:result', id, 'timeout');
    });
  });
});

ipcRenderer.on('uv-wake:spawn', (_e, cmd, args) => {
  require('node:child_process')
    .spawn(cmd, args, { stdio: 'ignore' })
    .on('error', () => {});
});

ipcRenderer.on('uv-wake:ping-me', (_e, delay) => {
  window.setTimeout(() => ipcRenderer.send('uv-wake:ping'), delay);
});
