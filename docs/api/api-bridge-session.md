## Class: ApiBridgeSession

> Pass APIs to every frame of a session.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Get it from [`ses.apiBridge`](session.md#sesapibridge-readonly). See
[`apiBridgeMain`](api-bridge-main.md) for an overview.

A session API reaches every window of the session, including windows opened
later, without a `pass()` call for each frame. One implementation serves them
all; wrap methods that need to know which frame called them with
[`apiBridgeMain.withCaller()`](api-bridge-main.md#apibridgemainwithcallerfn).

### Instance Methods

#### `apiBridge.pass(name, api, options)`

* `name` string - The name of the API on the page's `navigator.electron`. It
  must be a JavaScript identifier, such as `notes` or `appSettings`.
* `api` Record\<string, any\> - The API, as for
  [`frame.apiBridge.pass()`](api-bridge-frame-main.md#apibridgepassname-api-options).
* `options` [ApiBridgeSessionOptions](structures/api-bridge-session-options.md) - Which
  documents get the API.

Passes `api` to every frame of the session that shows a document of one of the
origins. Documents already open get it shortly after this call; every later
document gets it before any of its scripts run.

Windows that a page opens with `window.open()` and `<webview>` tags only get
the API if you set the `popups` or `guests` option. Prerendered pages and
fenced frames never get session APIs. In a frame that has an API of the same
name from [`frame.apiBridge.pass()`](api-bridge-frame-main.md), the frame's API is used
instead.

Passing a `name` the session already has replaces that API everywhere.

#### `apiBridge.passToIsolatedWorld(name, api, options)`

* `name` string - The name of the API on the preload's `navigator.electron`.
* `api` Record\<string, any\> - The API, as for `apiBridge.pass()`.
* `options` [ApiBridgeSessionOptions](structures/api-bridge-session-options.md) - Which
  documents get the API.

Like `apiBridge.pass()`, but passes `api` to the isolated world where preload
scripts run instead of to the page, as
[`frame.apiBridge.passToIsolatedWorld()`](api-bridge-frame-main.md#apibridgepasstoisolatedworldname-api-options)
does for one frame.

#### `apiBridge.revoke(name)`

* `name` string

Returns `boolean` - Whether the session had an API under `name`.

Takes the API away from every document of the session that has it; later
documents don't get it. Calls that are still running are cancelled.

#### `apiBridge.revokeFromIsolatedWorld(name)`

* `name` string

Returns `boolean` - Whether the session had an isolated-world API under
`name`.

Like `apiBridge.revoke()`, for an API passed with `apiBridge.passToIsolatedWorld()`.
