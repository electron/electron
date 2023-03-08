# ESM Limitations

This document serves to outline the limitations / differences between ESM in Electron and ESM in Node.js and Chromium.

## Sandboxed preload scripts can't use ESM imports

Sandboxed preload scripts are run as plain javascript without an ESM context.  It is reccomended that preload scripts are bundled via something like `webpack` or `vite` for performance reasons regardless, so your preload script should just be a single file that doesn't need to use ESM imports.  Loading the `electron` API is still done via `require('electron')`.

## Non context isolated renderers can't use Node.js ESM imports

If your renderer process does not have `contextIsolation` enabled you can not `import` ESM files via the Node.js module loader.  This means that you can't `import('fs')` or `import('./foo')`.  If you want to be able to do so you must enable context isolation.  This is because in the renderer Chromium's `import()` function takes precedence and without context isolation there is no way for Electron to know which loader to route the request to.

If you enable context isolation `import()` from the isolated preload context will use the Node.js loader and `import()` from the main context will continue using Chromium's loader.

## You must use `await` generously in the main process to avoid race conditions

Certain APIs in Electron (`app.setPath` for instance) are documented as needing to be called **before** the `app.ready` event is emitted.  When using ESM in the main process it is only guarunteed that the `ready` event hasn't been emitted while executing the side-effects of the primary import.  i.e. if `index.mjs` calls `import('./set-up-paths.mjs')` at the top level the app will likely already be "ready" by the time that dynamic import resolves.  To avoid this you should `await import('./set-up-paths.mjs')` at the top level of `index.mjs`.  It's not just import calls you should await, if you are reading files asyncronously or performing other asycronous actions you must await those at the top-level as well to ensure the app does not resume initialization and become ready too early.
