# sandbox-stranded-document

Standalone repro for the sandboxed-renderer error:

```
Electron sandboxed_renderer.bundle.js script failed to run
TypeError: Cannot destructure property 'preloadScripts' of 'binding.startupData' as it is null.
```

## Production trigger

Apps commonly install main-process navigation guards that cancel unwanted
navigations — e.g. blocking an auth redirect:

```js
webContents.on('will-redirect', (event) => event.preventDefault());
```

When the guard cancels the *first* navigation of a sandboxed WebContents, the
navigation never reaches `ReadyToCommitNavigation`, which is where the browser
pushes preload scripts + process info to the renderer over the
`ElectronFrameStartup` mojo channel. The WebContents is stranded on its
initial empty document with no startup data cached renderer-side.

If a script context is later forced onto that stranded document — DevTools
attach, a CDP `Runtime.enable`, `webFrameMain.executeJavaScript()`, etc. —
the sandbox bundle runs with `binding.startupData === null`. Before the fix
it threw the TypeError above, which surfaced as "script failed to run" and
pointed debugging at the renderer, when the actual cause was the app's own
(deliberate) navigation guard. The throw also aborted bundle init before the
internal IPC handlers were registered, so APIs like
`webContents.executeJavaScript()` hung silently.

A cancelled navigation is not strictly required: the same null was observable
as a race on a perfectly valid navigation, by forcing a script context while
the first navigation was still pending (e.g. a CDP client like Puppeteer
attaching at startup, racing a slow server). The committed document was never
affected — its context is created after the commit-time push by mojo
ordering — only the transient initial-document context was.

The fix pushes the startup data at frame creation as well as ahead of each
committed navigation, so the initial empty document observes preload scripts
like any other document (matching the pre-push, sync-IPC behavior of
Electron <= 41). If an unexpected edge case still reaches the bundle without
startup data, it logs a single warning naming the document URL and completes
a preload-less initialization instead of throwing.

## Running

```sh
electron spec/fixtures/apps/sandbox-stranded-document
```

- Unfixed build: logs the TypeError; `PRELOAD-RAN` never appears.
- Fixed build: logs `PRELOAD-RAN` for the forced context on the initial empty
  document; no TypeError, no warning.

`STRAND_MODE=devtools` provokes the context via DevTools instead of the CDP
debugger; `STRAND_MODE=nothing` only strands the document (no error either
way — the context is created lazily).
