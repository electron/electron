// The internal JavaScript bundles baked into the Electron binary via
// electron_js2c (see BUILD.gn), one per kind of context Electron runs
// JavaScript in. Each is built from lib/<name>/init.ts by bundle.mjs.
//
// alwaysHasNode:
//   The bundle runs inside a Node.js environment and is called with Node's
//   `process` and internal `require` (see shell/common/js2c_bundle_ids.h), so
//   Node.js built-in and `internal/*` modules are left as runtime require()
//   calls. Bundles without it must be fully self-contained; they get a minimal
//   `process` shim (lib/webview/process.ts) and a native EventEmitter in place
//   of `events` (lib/common/node-events.ts).
// loadElectronFromAlternateTarget:
//   Resolve `require('electron')` to lib/<alternate>/api/exports/electron.ts
//   instead of the target's own module list.
// targetDeletesNodeGlobals:
//   `process`, `global` and `Buffer` may be deleted from the global scope
//   before bundle code runs later on, so free references to them are rewritten
//   to the copies captured by lib/common/node-globals.ts.
// wrapInitWithProfilingTimeout:
//   Defer running the bundle by a tick when --profile-electron-init is passed.
// wrapInitWithTryCatch:
//   Log (rather than throw) if the bundle fails to evaluate.
export const targets = {
  browser: {
    alwaysHasNode: true
  },
  renderer: {
    alwaysHasNode: true,
    targetDeletesNodeGlobals: true,
    wrapInitWithProfilingTimeout: true,
    wrapInitWithTryCatch: true
  },
  worker: {
    loadElectronFromAlternateTarget: 'renderer',
    alwaysHasNode: true,
    targetDeletesNodeGlobals: true,
    wrapInitWithTryCatch: true
  },
  webview: {
    alwaysHasNode: false,
    loadElectronFromAlternateTarget: 'renderer',
    wrapInitWithTryCatch: true
  },
  isolated_renderer: {
    alwaysHasNode: false,
    wrapInitWithTryCatch: true
  },
  node: {
    alwaysHasNode: true
  },
  utility: {
    alwaysHasNode: true
  }
};
