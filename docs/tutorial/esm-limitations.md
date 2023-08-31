# ESM Limitations

This document serves to outline the limitations / differences between ESM in Electron and ESM in Node.js and Chromium.

## ESM Support Matrix

This table gives a general overview of where ESM is supported and most importantly which ESM loader is used.

| | Supported | Loader | Supported in Preload | Loader in Preload | Applicable Requirements |
|-|-|-|-|-|-|
| Main Process | Yes | Node.js | N/A | N/A | <ul><li> [You must `await` generously in the main process to avoid race conditions](#you-must-use-await-generously-in-the-main-process-to-avoid-race-conditions) </li></ul> |
| Sandboxed Renderer | Yes | Chromium | No | | <ul><li> [Sandboxed preload scripts can't use ESM imports](#sandboxed-preload-scripts-cant-use-esm-imports) </li></ul> |
| Node.js Renderer + Context Isolation | Yes | Chromium | Yes | Node.js | <ul><li> [Node.js ESM Preload Scripts will run after page load on pages with no content](#nodejs-esm-preload-scripts-will-run-after-page-load-on-pages-with-no-content) </li> <li>[ESM Preload Scripts must have the `.mjs` extension](#esm-preload-scripts-must-have-the-mjs-extension)</li></ul> |
| Node.js Renderer + No Context Isolation | Yes | Chromium | Yes | Node.js | <ul><li> [Non-context-isolated renderers can't use dynamic Node.js ESM imports](#non-context-isolated-renderers-cant-use-dynamic-nodejs-esm-imports) </li> <li>[ESM Preload Scripts must have the `.mjs` extension](#esm-preload-scripts-must-have-the-mjs-extension)</li></ul> |

## Requirements

### You must use `await` generously in the main process to avoid race conditions

Certain APIs in Electron (`app.setPath` for instance) are documented as needing to be called **before** the `app.on('ready')` event is emitted.  When using ESM in the main process it is only guaranteed that the `ready` event hasn't been emitted while executing the side-effects of the primary import.  i.e. if `index.mjs` calls `import('./set-up-paths.mjs')` at the top level the app will likely already be "ready" by the time that dynamic import resolves.  To avoid this you should `await import('./set-up-paths.mjs')` at the top level of `index.mjs`.  It's not just import calls you should await, if you are reading files asynchronously or performing other asynchronous actions you must await those at the top-level as well to ensure the app does not resume initialization and become ready too early.

### Sandboxed preload scripts can't use ESM imports

Sandboxed preload scripts are run as plain javascript without an ESM context.  It is recommended that preload scripts are bundled via something like `webpack` or `vite` for performance reasons regardless, so your preload script should just be a single file that doesn't need to use ESM imports.  Loading the `electron` API is still done via `require('electron')`.

### Node.js ESM Preload Scripts will run after page load on pages with no content

If the response body for the page is **completely** empty, i.e. `Content-Length: 0`, the preload script will not block the page load, which may result in race conditions. If this impacts you, change your response body to have _something_ in it, for example an empty `html` tag (`<html></html>`) or swap back to using a CommonJS preload script (`.js` or `.cjs`) which will block the page load.

### ESM Preload Scripts must have the `.mjs` extension

In order to load an ESM preload script it must have a `.mjs` file extension.  Using `type: module` in a nearby package.json is not sufficient.  Please also note the limitation above around not blocking page load if the page is empty.

### Non-context-isolated renderers can't use dynamic Node.js ESM imports

If your renderer process does not have `contextIsolation` enabled you can not `import()` ESM files via the Node.js module loader.  This means that you can't `import('fs')` or `import('./foo')`.  If you want to be able to do so you must enable context isolation.  This is because in the renderer Chromium's `import()` function takes precedence and without context isolation there is no way for Electron to know which loader to route the request to.

If you enable context isolation `import()` from the isolated preload context will use the Node.js loader and `import()` from the main context will continue using Chromium's loader.
