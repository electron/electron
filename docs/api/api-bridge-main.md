# apiBridgeMain

> Create the events and stores of APIs you pass to frames.

Process: [Main](../glossary.md#main-process)

apiBridge lets the main process hand an API object straight to a page. Pass the
object to one frame with [`frame.apiBridge.pass()`](api-bridge-frame-main.md), or to
every frame of a session with [`ses.apiBridge.pass()`](api-bridge-session.md), and it
appears on the page's `navigator.electron` under the name you gave it, before
any page script runs. There is no preload script, context bridge or channel
name involved: a frame can only use the APIs it was given, and only while it
shows a document of an origin the API was passed for.

An API is a plain object. Each of its own enumerable properties becomes a
member of the object on `navigator.electron`:

* A function becomes an async method: the page gets a function that returns a
  `Promise` for the function's (awaited) return value. If the function throws
  or rejects, the promise rejects with an error that has the same `name` and
  `message`. A `TypeError`, `RangeError`, `ReferenceError` or `SyntaxError`
  keeps its type. The stack and other properties stay in the main process.
* A function wrapped with [`apiBridgeMain.sync()`](#apibridgemainsyncfn) becomes a sync
  method: the page's function returns the value, or throws.
* A function wrapped with [`apiBridgeMain.withCaller()`](#apibridgemainwithcallerfn) is
  also told which frame called it.
* An [`ApiBridgeEvent`](api-bridge-event.md) becomes an object with an `on(listener)`
  function. `on` returns a function that removes the listener.
* An [`ApiBridgeStore`](api-bridge-store.md) becomes an object with `get()`, which returns
  the current value without asking the main process, and `subscribe(listener)`,
  which calls `listener` with each new value and returns a function that
  removes it.

Arguments, return values, event arguments and store values are copied with the
[Structured Clone Algorithm][SCA]. Functions and prototypes are not copied.

The same API object can be passed to any number of frames. Every frame calls
the same functions, and its events and stores reach all of them.

```js
// Main process
const { app, BrowserWindow, BaseWindow, apiBridgeMain, session, webContents } = require('electron')

app.whenReady().then(() => {
  const theme = apiBridgeMain.store('light')
  const saved = apiBridgeMain.event()

  // Every window of the default session that shows https://example.com gets
  // navigator.electron.notes.
  session.defaultSession.apiBridge.pass('notes', {
    getVersion: () => app.getVersion(),
    setTheme: (value) => { theme.set(value) },
    save: async (text) => {
      console.log('Saving', text)
      saved.emit(Date.now())
    },
    closeWindow: apiBridgeMain.withCaller((caller) => {
      const contents = webContents.fromFrame(caller.frame)
      if (contents) BaseWindow.fromId(contents.id)?.close()
    }),
    theme,
    saved
  }, { origin: 'https://example.com' })

  new BrowserWindow().loadURL('https://example.com')
  new BrowserWindow().loadURL('https://example.com/settings')
})
```

```js @ts-nocheck
// Web page, no preload needed
const { notes } = navigator.electron
console.log(await notes.getVersion())
document.body.dataset.theme = notes.theme.get()
const unsubscribe = notes.theme.subscribe((theme) => {
  document.body.dataset.theme = theme
})
notes.saved.on((time) => console.log('Saved at', time))
```

## `navigator.electron`

`navigator.electron` exists only while a document has at least one API. It is
added with the first API and removed with the last. A page that was never
given an API can't use it to tell that it runs in Electron, and a page that
runs both in a browser and in your app can check for it with
`'electron' in navigator`. It adds nothing to `window`, so API names never
collide with the page's own globals. Each API on it is a frozen, read-only
property.

When the APIs of a document change after it has loaded, Electron fires an
`electronapichange` event at the page's `window`, one for each API. The event's
`detail` has the API's `name`, and a `change` that is `added`, `removed` or
`replaced`:

```js @ts-nocheck
// Web page
window.addEventListener('electronapichange', (event) => {
  const { name, change } = event.detail
  console.log(`The ${name} API was ${change}`)
})
```

## Preload scripts

[`frame.apiBridge.passToIsolatedWorld()`](api-bridge-frame-main.md#apibridgepasstoisolatedworldname-api-options)
and [`ses.apiBridge.passToIsolatedWorld()`](api-bridge-session.md#apibridgepasstoisolatedworldname-api-options)
pass an API to the isolated world where preload scripts run with
`contextIsolation`, instead of to the page. The preload finds it on its own
`navigator.electron`; the page can't see it. Use it for APIs that only your
preload script should call. APIs in the isolated world don't fire
`electronapichange`, because the page would receive that event too.

APIs are on `navigator.electron` before a preload script's first line runs.
With `contextIsolation` disabled, the preload runs in the page's world and
sees the APIs passed with `pass()`.

## Security

* An API is only available in the frames it was passed to, and only to
  documents of an origin it was passed for. The main process checks the
  calling frame and its committed origin on every call, so a frame that
  navigated elsewhere can't use it. Navigating back to the origin makes the
  API available again.
* `file://` can't be an API's origin, because every local file has that
  origin. Load your pages from a custom protocol instead.
* A session API reaches main frames only unless you ask for all frames. It
  reaches windows that a page opens with `window.open()` and `<webview>` tags
  only if you ask for them, and never reaches prerendered pages or fenced
  frames.
* Every script in the page can call the API, and so can a same-origin iframe
  through `parent.navigator`, which the same-origin policy already lets script
  the parent. Calls made that way count as calls from the frame that has the
  API. Cross-origin iframes can't reach it.
* Only the object's own enumerable properties are exposed, never anything it
  inherits.
* Values are copied, so the page never gets a reference to a main process
  object. Your methods still receive whatever the page sends: check arguments
  before you act on them.
* When an API is revoked, calls that are still running are cancelled: the
  page's promise rejects at once, and
  [`caller.signal`](structures/api-bridge-caller.md) aborts. The method itself keeps
  running; its result is dropped.

## Methods

The `apiBridgeMain` module has the following methods:

### `apiBridgeMain.event()`

Returns [`ApiBridgeEvent`](api-bridge-event.md) - An event to put on an API.

### `apiBridgeMain.store([initialValue])`

* `initialValue` any (optional) - The store's first value. It must be possible
  to copy it with the [Structured Clone Algorithm][SCA].

Returns [`ApiBridgeStore`](api-bridge-store.md) - A store to put on an API.

### `apiBridgeMain.sync(fn)`

* `fn` (...args: any[]) => any

Returns `(...args: any[]) => any` - `fn`, marked so that pages call it
synchronously.

The page is blocked until `fn` returns, or until the promise it returns
settles. Use it only for quick calls a page can't do without, such as reading
configuration before it renders.

### `apiBridgeMain.withCaller(fn)`

* `fn` (...args: any[]) => any - Called with an
  [`ApiBridgeCaller`](structures/api-bridge-caller.md) followed by the arguments the page
  passed.

Returns `(...args: any[]) => any` - `fn`, marked so that it is told which
frame called it.

The page calls the method without the caller argument. Use it for methods of
an API that several frames share, such as a session API, when the method acts
on the calling window. It combines with `apiBridgeMain.sync()` in either order.

[SCA]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm
