## Class: AppShimHost

> Show windows as another app, with a Dock tile of its own.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

An app shim is a small macOS app, in a bundle of its own, that shows windows on
your app's behalf. Your app still creates each window, loads its page and
handles its events, but the native window belongs to the shim's process. So
the Dock, the app switcher, Mission Control and Stage Manager show the window
under the shim's name and icon, and not under your app's.

An `AppShimHost` is your app's end of the connection to one running shim. You
get one from the [`app-shim-connected`](app.md#event-app-shim-connected-macos)
event, after you have called
[`app.startAppShimServer()`](app.md#appstartappshimserveroptions-macos).

```js
const { app, BrowserWindow } = require('electron')

app.whenReady().then(() => {
  app.startAppShimServer()
})

app.on('app-shim-connected', (event, shim) => {
  // Anything running as the same user can connect. Check who did.
  if (!shim.satisfiesCodeRequirement('anchor apple generic and certificate leaf[subject.OU] = "ABCDE12345"')) {
    shim.quit()
    return
  }

  const win = new BrowserWindow({ appShim: shim })
  win.loadURL('https://example.com')

  shim.on('activate', (event, hasVisibleWindows) => {
    if (!hasVisibleWindows) win.show()
  })
})
```

**Making an app shim**

A shim is an app bundle whose executable is a copy of
`Electron Framework.framework/Helpers/app_shim_loader` from your packaged app.
The loader links nothing from Electron. It loads your app's Electron framework
when it starts, so a shim made by one version of your app keeps working after
your app updates.

The shim's `Info.plist` holds the usual keys (`CFBundleExecutable`,
`CFBundleIdentifier`, `CFBundleName`, `CFBundleIconFile` and so on) and these:

* `ElectronAppShimHostPath` string - The absolute path of your app's `.app`
  bundle.
* `ElectronAppShimHostBundleIdentifier` string - Your app's bundle identifier,
  used to find your app when `ElectronAppShimHostPath` is absent.
* `ElectronAppShimServerName` string (optional) - The `name` your app passes
  to `app.startAppShimServer()`, when it passes one.
* `ElectronAppShimHostArguments` string[] (optional) - Command-line arguments
  to start your app with, when the shim starts and your app is not running.

When a shim starts, it connects to your app. If your app is not running, the
shim starts it, without activating it, and waits up to a minute for it to call
`app.startAppShimServer()`.

In a signed app, macOS only lets the shim load your framework when both are
signed by the same team.

**Limitations**

A window in an app shim has no native window in your app's process, so the
parts of the window API that work on that native window directly do nothing
yet. That includes vibrancy, `titleBarStyle` values other than the default,
traffic light positioning, sheets, `parent` and `modal`, tabs, the Touch Bar
and Quick Look previews. `win.getNativeWindowHandle()` and
`win.getMediaSourceId()` name a hidden stand-in window, not the one on screen.

The shim's process carries out what you ask of a window, and reports back
afterwards. So a change is not always visible to your app straight away: after
`win.show()`, for example, `win.isVisible()` is `false` until the window's
`show` event.

While a shim is the active app, the menu bar is a short standard menu that the
shim builds. Your app's application menu, and its accelerators, are not shown.

Each window belongs to one shim, chosen when the window is created.

App shims are not available in Mac App Store builds.

### Instance Events

#### Event: 'activate'

Returns:

* `event` Event
* `hasVisibleWindows` boolean

Emitted when the person clicks the shim's Dock tile, or opens the shim again
while it is running.

#### Event: 'open-file'

Returns:

* `event` Event
* `path` string

Emitted when the shim is asked to open a file.

#### Event: 'open-url'

Returns:

* `event` Event
* `url` string

Emitted when the shim is asked to open a URL.

#### Event: 'quit-requested'

Returns:

* `event` Event

Emitted when the person asks the shim to quit, for example with
<kbd>Cmd</kbd>+<kbd>Q</kbd> or from its Dock menu. By default the shim then
quits, which closes its windows without emitting their `close` events. Call
`event.preventDefault()` to keep it running; you can then close its windows
yourself and call `shim.quit()`.

#### Event: 'disconnected'

Returns:

* `event` Event

Emitted when the shim's process has gone away. Every window that was created
in it is already closed.

### Instance Methods

#### `shim.isConnected()`

Returns `boolean` - Whether the shim's process is still connected.

#### `shim.satisfiesCodeRequirement(requirement)`

* `requirement` string - A [code requirement](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/RequirementLang/RequirementLang.html).

Returns `boolean` - Whether the code signature of the shim's process meets
`requirement`. The check is made on the process that connected, not on the
bundle at `shim.bundlePath`.

Throws if `requirement` cannot be parsed.

#### `shim.setBadge(text)`

* `text` string

Sets the text shown on the shim's Dock tile. An empty string clears it.

#### `shim.bounce([type])`

* `type` string (optional) - Can be `critical` or `informational`. The default is
 `informational`

Bounces the shim's Dock tile, as [`dock.bounce()`](dock.md#dockbouncetype-macos)
does for your app's.

#### `shim.cancelBounce()`

Stops the bounce that `shim.bounce()` started.

#### `shim.quit()`

Ends the shim's process and closes its windows. The windows' `close` events are
not emitted; their `closed` events are.

### Instance Properties

#### `shim.pid` _Readonly_

A `number` that is the process ID of the shim.

#### `shim.bundlePath` _Readonly_

A `string` that is the path of the app bundle the shim's process runs from.

#### `shim.bundleId` _Readonly_

A `string` that is the bundle identifier in that bundle's `Info.plist`. Anyone
can make a bundle with any identifier, so do not use it to decide whether to
trust the shim.
