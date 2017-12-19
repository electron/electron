# `sandbox` Option

> Create a browser window with a renderer that can run inside Chromium OS sandbox. With this
option enabled, the renderer must communicate via IPC to the main process in order to access node APIs.
However, in order to enable the Chromium OS sandbox, electron must be run with the `--enable-sandbox`
command line argument.

One of the key security features of Chromium is that all blink rendering/JavaScript
code is executed within a sandbox. This sandbox uses OS-specific features to ensure
that exploits in the renderer process cannot harm the system.

In other words, when the sandbox is enabled, the renderers can only make changes
to the system by delegating tasks to the main process via IPC.
[Here's](https://www.chromium.org/developers/design-documents/sandbox) more
information about the sandbox.

Since a major feature in electron is the ability to run node.js in the
renderer process (making it easier to develop desktop applications using web
technologies), the sandbox is disabled by electron. This is because
most node.js APIs require system access. `require()` for example, is not
possible without file system permissions, which are not available in a sandboxed
environment.

Usually this is not a problem for desktop applications since the code is always
trusted, but it makes electron less secure than chromium for displaying
untrusted web content. For applications that require more security, the
`sandbox` flag will force electron to spawn a classic chromium renderer that is
compatible with the sandbox.

A sandboxed renderer doesn't have a node.js environment running and doesn't
expose node.js JavaScript APIs to client code. The only exception is the preload script,
which has access to a subset of the electron renderer API.

Another difference is that sandboxed renderers don't modify any of the default
JavaScript APIs. Consequently, some APIs such as `window.open` will work as they
do in chromium (i.e. they do not return a [`BrowserWindowProxy`](browser-window-proxy.md)).

## Example

To create a sandboxed window, simply pass `sandbox: true` to `webPreferences`:

```js
let win
app.on('ready', () => {
  win = new BrowserWindow({
    webPreferences: {
      sandbox: true
    }
  })
  win.loadURL('http://google.com')
})
```

In the above code the [`BrowserWindow`](browser-window.md) that was created has node.js disabled and can communicate
only via IPC. The use of this option stops electron from creating a node.js runtime in the renderer. Also,
within this new window `window.open` follows the native behaviour (by default electron creates a [`BrowserWindow`](browser-window.md)
and returns a proxy to this via `window.open`).

It is important to note that this option alone won't enable the OS-enforced sandbox. To enable this feature, the
`--enable-sandbox` command-line argument must be passed to electron, which will
force `sandbox: true` for all `BrowserWindow` instances.

To enable OS-enforced sandbox on `BrowserWindow` or `webview` process with `sandbox:true` without causing
entire app to be in sandbox, `--enable-mixed-sandbox` command-line argument must be passed to electron.
This option is currently only supported on macOS and Windows.

```js
let win
app.on('ready', () => {
  // no need to pass `sandbox: true` since `--enable-sandbox` was enabled.
  win = new BrowserWindow()
  win.loadURL('http://google.com')
})
```

Note that it is not enough to call
`app.commandLine.appendSwitch('--enable-sandbox')`, as electron/node startup
code runs after it is possible to make changes to chromium sandbox settings. The
switch must be passed to electron on the command-line:

```sh
electron --enable-sandbox app.js
```

It is not possible to have the OS sandbox active only for some renderers, if
`--enable-sandbox` is enabled, normal electron windows cannot be created.

If you need to mix sandboxed and non-sandboxed renderers in one application,
simply omit the `--enable-sandbox` argument. Without this argument, windows
created with `sandbox: true` will still have node.js disabled and communicate
only via IPC, which by itself is already a gain from security POV.

## Preload

An app can make customizations to sandboxed renderers using a preload script.
Here's an example:

```js
let win
app.on('ready', () => {
  win = new BrowserWindow({
    webPreferences: {
      sandbox: true,
      preload: 'preload.js'
    }
  })
  win.loadURL('http://google.com')
})
```

and preload.js:

```js
// This file is loaded whenever a javascript context is created. It runs in a
// private scope that can access a subset of electron renderer APIs. We must be
// careful to not leak any objects into the global scope!
const fs = require('fs')
const {ipcRenderer} = require('electron')

// read a configuration file using the `fs` module
const buf = fs.readFileSync('allowed-popup-urls.json')
const allowedUrls = JSON.parse(buf.toString('utf8'))

const defaultWindowOpen = window.open

function customWindowOpen (url, ...args) {
  if (allowedUrls.indexOf(url) === -1) {
    ipcRenderer.sendSync('blocked-popup-notification', location.origin, url)
    return null
  }
  return defaultWindowOpen(url, ...args)
}

window.open = customWindowOpen
```

Important things to notice in the preload script:

- Even though the sandboxed renderer doesn't have node.js running, it still has
  access to a limited node-like environment: `Buffer`, `process`, `setImmediate`
  and `require` are available.
- The preload script can indirectly access all APIs from the main process through the
  `remote` and `ipcRenderer` modules. This is how `fs` (used above) and other
  modules are implemented: They are proxies to remote counterparts in the main
  process.
- The preload script must be contained in a single script, but it is possible to have
  complex preload code composed with multiple modules by using a tool like
  browserify, as explained below. In fact, browserify is already used by
  electron to provide a node-like environment to the preload script.

To create a browserify bundle and use it as a preload script, something like
the following should be used:

```sh
  browserify preload/index.js \
    -x electron \
    -x fs \
    --insert-global-vars=__filename,__dirname -o preload.js
```

The `-x` flag should be used with any required module that is already exposed in
the preload scope, and tells browserify to use the enclosing `require` function
for it. `--insert-global-vars` will ensure that `process`, `Buffer` and
`setImmediate` are also taken from the enclosing scope(normally browserify
injects code for those).

Currently the `require` function provided in the preload scope exposes the
following modules:

- `child_process`
- `electron`
  - `crashReporter`
  - `remote`
  - `ipcRenderer`
  - `webFrame`
- `fs`
- `os`
- `timers`
- `url`

More may be added as needed to expose more electron APIs in the sandbox, but any
module in the main process can already be used through
`electron.remote.require`.

## Status

Please use the `sandbox` option with care, as it is still an experimental
feature. We are still not aware of the security implications of exposing some
electron renderer APIs to the preload script, but here are some things to
consider before rendering untrusted content:

- A preload script can accidentaly leak privileged APIs to untrusted code.
- Some bug in V8 engine may allow malicious code to access the renderer preload
  APIs, effectively granting full access to the system through the `remote`
  module.

Since rendering untrusted content in electron is still uncharted territory,
the APIs exposed to the sandbox preload script should be considered more
unstable than the rest of electron APIs, and may have breaking changes to fix
security issues.

One planned enhancement that should greatly increase security is to block IPC
messages from sandboxed renderers by default, allowing the main process to
explicitly define a set of messages the renderer is allowed to send.
