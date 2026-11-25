// Preloaded (NODE_OPTIONS=--require) into every Electron process the specs
// start when they run with --coverage (see electron-pool.ts). Node.js writes
// NODE_V8_COVERAGE output from its exit hooks, which some ways for a browser
// or utility process to end skip; take the coverage before those instead:
//
// - process.crash() aborts.
// - A utility process killed by its parent (child.kill()) gets SIGTERM.
// - app.exit() before 'ready' calls exit() directly (Browser::ExitWithCode).
//
// Does nothing in other processes (renderers have no NODE_V8_COVERAGE support;
// see renderer-coverage.ts) or without NODE_V8_COVERAGE.
'use strict';

if (process.env.NODE_V8_COVERAGE && (process.type === 'browser' || process.type === 'utility')) {
  const v8 = require('node:v8');
  const flush = () => {
    try {
      v8.takeCoverage();
    } catch {}
  };

  // Wraps process[name], including whatever is assigned to it later.
  const hook = (name, wrap) => {
    let current = process[name] && wrap(process[name]);
    Object.defineProperty(process, name, {
      configurable: true,
      enumerable: true,
      get: () => current,
      set: (fn) => {
        current = typeof fn === 'function' ? wrap(fn) : fn;
      }
    });
  };

  hook(
    'crash',
    (crash) =>
      function () {
        flush();
        return crash.apply(this, arguments);
      }
  );

  if (process.type === 'utility') {
    const onSigterm = () => {
      flush();
      // Somebody else handles SIGTERM; leave it to them.
      if (process.listenerCount('SIGTERM') > 1) return;
      // Otherwise die of it as if nobody had listened.
      process.removeListener('SIGTERM', onSigterm);
      process.kill(process.pid, 'SIGTERM');
    };
    process.on('SIGTERM', onSigterm);
  } else {
    // lib/browser/init.ts maps process.exit to app.exit once it has loaded
    // 'electron', so that assignment is the first point where app.exit can be
    // wrapped.
    let appExitWrapped = false;
    hook('exit', (exit) => {
      if (!appExitWrapped) {
        try {
          const { app } = require('electron');
          const appExit = app.exit;
          app.exit = function () {
            if (!app.isReady()) flush();
            return appExit.apply(this, arguments);
          };
          appExitWrapped = true;
        } catch {}
      }
      return exit;
    });
  }
}
