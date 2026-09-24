// Operations whose completion depends on Electron driving the uv loop. Each
// calls done() (or fail()) from the callback that completes it; `expect` is the
// built-in delay to subtract from the measured latency and `budget` the slack
// allowed on top of it. `deferred` ops go through a promise reaction or
// process.nextTick, which Node.js holds back while another callback is still
// on the stack, e.g. inside a nested run loop; `continuation` ops start their
// uv work from a promise reaction, which a bound callback's caller only runs
// at the next task.
const cp = require('node:child_process');
const crypto = require('node:crypto');
const fs = require('node:fs');
const net = require('node:net');
const os = require('node:os');
const path = require('node:path');
const timers = require('node:timers');
const timersP = require('node:timers/promises');
const { MessageChannel } = require('node:worker_threads');
const zlib = require('node:zlib');

// One scratch directory per app run, shared with the renderer through env.
process.env.UV_WAKE_TMP ??= fs.mkdtempSync(path.join(os.tmpdir(), 'uv-wake-'));
const watched = path.join(process.env.UV_WAKE_TMP, `watched-${process.type}`);
fs.writeFileSync(watched, '0');

// The other process touches the watched file or connects to a port on
// request, so that starting an op never wakes this process's loop itself.
let other = { touch: () => {}, connect: () => {} };

const ops = {
  'timers.setTimeout': { expect: 50, run: (done) => timers.setTimeout(done, 50) },
  'timers/promises.setTimeout': { expect: 50, deferred: true, run: (done) => timersP.setTimeout(50).then(done) },
  // Unwrapped, so only a check after the microtask checkpoint notices it.
  'timers/promises.setTimeout from a promise continuation': {
    expect: 50,
    deferred: true,
    continuation: true,
    run: (done) => Promise.resolve().then(() => timersP.setTimeout(50).then(done))
  },
  setImmediate: { run: (done) => setImmediate(done) },
  'timers/promises.setImmediate': { deferred: true, run: (done) => timersP.setImmediate().then(done) },
  'process.nextTick': { deferred: true, run: (done) => process.nextTick(done) },
  'fs.readFile': { run: (done, fail) => fs.readFile(__filename, (e) => (e ? fail(e) : done())) },
  'fs.promises.readFile': { deferred: true, run: (done, fail) => fs.promises.readFile(__filename).then(done, fail) },
  // The other process writes the file (after a Blink-timed delay, so kqueue
  // has the watcher by then), so only the watcher's own wake is measured.
  'fs.watch': {
    expect: 30,
    run: (done, fail) => {
      const w = fs.watch(watched, () => {
        w.close();
        done();
      });
      w.on('error', fail);
      other.touch(watched, 30);
    }
  },
  'crypto.pbkdf2': { run: (done, fail) => crypto.pbkdf2('pw', 'salt', 1, 16, 'sha256', (e) => (e ? fail(e) : done())) },
  // Completes through stream events, which are queued with process.nextTick.
  'zlib.deflate': { deferred: true, run: (done, fail) => zlib.deflate('x', (e) => (e ? fail(e) : done())) },
  // listen() queues the socket's watcher synchronously and the connection
  // arrives from the other process, so only a deadline check gets it polled.
  'net.listen': {
    run: (done, fail) => {
      const server = net.createServer((sock) => {
        sock.destroy();
        server.close();
        done();
      });
      server.on('error', fail);
      // Without a host, listen() binds synchronously and address() is set.
      server.listen(0);
      other.connect(server.address().port);
    }
  },
  'worker_threads.MessageChannel': {
    run: (done) => {
      const { port1, port2 } = new MessageChannel();
      port2.once('message', () => {
        port1.close();
        port2.close();
        done();
      });
      port1.postMessage(1);
    }
  },
  'child_process.exit': {
    budget: 500,
    run: (done, fail) => {
      const child =
        process.platform === 'win32'
          ? cp.spawn('cmd.exe', ['/c', 'exit', '0'], { stdio: 'ignore' })
          : cp.spawn('/bin/sh', ['-c', 'exit 0'], { stdio: 'ignore' });
      child.on('error', fail);
      child.on('exit', () => done());
    }
  }
};

// Testing builds only: a raw uv timer started by native code from a Chromium
// task, with no JavaScript on the stack when it is started.
try {
  const testing = process._linkedBinding('electron_common_testing');
  ops['native uv_timer_start from a task'] = { expect: 20, run: (done) => testing.startUvTimerFromTask(20, done) };
} catch {}

for (const op of Object.values(ops)) {
  op.expect ??= 0;
  op.budget ??= 100;
}

module.exports = {
  ops,
  setOtherProcess: (impl) => {
    other = impl;
  },
  cleanup: () => fs.rmSync(process.env.UV_WAKE_TMP, { recursive: true, force: true })
};
