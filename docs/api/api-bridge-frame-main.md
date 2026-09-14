## Class: ApiBridgeFrameMain

> Pass APIs to a frame.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Get it from [`frame.apiBridge`](web-frame-main.md#frameapibridge-readonly). See
[`apiBridgeMain`](api-bridge-main.md) for an overview, and [`ses.apiBridge`](api-bridge-session.md)
to give an API to every frame of a session instead.

### Instance Methods

#### `apiBridge.pass(name, api[, options])`

* `name` string - The name of the API on the page's `navigator.electron`. It
  must be a JavaScript identifier, such as `notes` or `appSettings`.
* `api` Record\<string, any\> - The API. Each own enumerable property must be a
  function, a function wrapped with [`apiBridgeMain.sync()`](api-bridge-main.md#apibridgemainsyncfn)
  or [`apiBridgeMain.withCaller()`](api-bridge-main.md#apibridgemainwithcallerfn), an
  [`ApiBridgeEvent`](api-bridge-event.md) or an [`ApiBridgeStore`](api-bridge-store.md). Methods are
  called with `api` as `this`.
* `options` Object (optional)
  * `origin` string | string[] (optional) - The origins of the documents that
    get the API, such as `https://example.com` or `app://my-app`. Defaults to
    the origin of the frame's current document. You must set it if the frame
    has not loaded a document with an origin yet, for example a new window
    before `loadURL()`. A custom protocol must be registered as `standard` with
    [`protocol.registerSchemesAsPrivileged()`](protocol.md#protocolregisterschemesasprivilegedcustomschemes)
    to have an origin. `file://` is not accepted: every local file has that
    origin, so load your pages from a custom protocol instead.

Passes `api` to this frame. Every document of one of the origins the frame
loads gets it as `navigator.electron[name]` before any of its scripts run; if
the current document has one of the origins, the API appears there shortly
after this call.

The same `api` object can be passed to any number of frames. Passing a `name`
this frame already has replaces that API; the page's old object stops working.
An API passed to the frame takes precedence over a session API with the same
name.

#### `apiBridge.passToIsolatedWorld(name, api[, options])`

* `name` string - The name of the API on the preload's `navigator.electron`.
* `api` Record\<string, any\> - The API, as for `apiBridge.pass()`.
* `options` Object (optional)
  * `origin` string | string[] (optional) - As for `apiBridge.pass()`.

Passes `api` to the isolated world where this frame's preload scripts run,
instead of to the page. The preload finds it on its own `navigator.electron`;
the page can't see or call it. Use it for APIs that only your preload script
should call.

Preload scripts run in an isolated world only with `contextIsolation`, and
only in the main frame or, with `nodeIntegrationInSubFrames`, in iframes. In
other frames the API is never delivered. The names of the two worlds are
separate, so the same `name` can be passed to both.

#### `apiBridge.revoke(name)`

* `name` string

Returns `boolean` - Whether an API was passed under `name`.

Takes the API away from the frame: it is removed from `navigator.electron`,
members of objects the page kept throw (or reject), and later documents don't
get it. Calls that are still running are cancelled: the page's promise rejects
at once, and their [`caller.signal`](structures/api-bridge-caller.md) aborts. A
session API with the same name takes its place, if there is one.

#### `apiBridge.revokeFromIsolatedWorld(name)`

* `name` string

Returns `boolean` - Whether an API was passed to the isolated world under
`name`.

Like `apiBridge.revoke()`, for an API passed with `apiBridge.passToIsolatedWorld()`.
