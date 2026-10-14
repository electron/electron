## Class: AppShimHost

> Show windows as another app, with a Dock tile of its own.

> [!NOTE]
> App shims are experimental. This API may change in a future Electron
> release.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

An app shim is a small macOS app, in a bundle of its own, that shows windows on
your app's behalf. Your app still creates each window, loads its page and
handles its events, but the native window belongs to the shim's process. So
the Dock, the app switcher, Mission Control and Stage Manager show the window
under the shim's name and icon, and not under your app's.

An `AppShimHost` is your app's end of the connection to one running shim. You
get one from the [`app-shim-connected`](app.md#event-app-shim-connected-macos-experimental)
event, after you have called
[`app.startAppShimServer()`](app.md#appstartappshimserveroptions-macos-experimental).

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

A shim can only connect to your app while your app is running the version that
is on disk. If your app is replaced on disk while it runs, a shim that starts
afterwards quits without showing anything, and your app gets an
[`app-shim-version-mismatch`](app.md#event-app-shim-version-mismatch-macos-experimental)
event, in which to tell the person. Updates that are installed when your app
quits, as
[`autoUpdater`](auto-updater.md) installs them, do not cause this.

The shim's `Info.plist` holds the usual keys (`CFBundleExecutable`,
`CFBundleIdentifier`, `CFBundleName`, `CFBundleIconFile` and so on) and these:

* `ElectronAppShimHostPath` string - The absolute path of your app's `.app`
  bundle.
* `ElectronAppShimHostBundleIdentifier` string - Your app's bundle identifier,
  used to find your app when `ElectronAppShimHostPath` is absent or nothing is
  at that path any more.
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
yet. That includes the Touch Bar, the window controls overlay
(`titleBarOverlay`), simple fullscreen, `win.setEnabled()`,
`win.flashFrame()` (use [`shim.bounce()`](#shimbouncetype-experimental)),
`win.setRepresentedFilename()`, `win.setDocumentEdited()` and
`win.setHiddenInMissionControl()`. `win.getNativeWindowHandle()` and
`win.getMediaSourceId()` name a hidden stand-in window, not the one on screen.

Tabs are windows of one process, so `win.addTabbedWindow()` only takes two
windows of the same shim.

A window in an app shim can't be `modal`, can't have a `parent`, and can't be
the `parent` of another window: creating a window that way throws.

The [`dialog`](dialog.md) module's message boxes and open and save dialogs
work with such a window as their parent, as sheets that the shim's process
shows. The synchronous ones do not block your app meanwhile: other events,
and so other code of yours, run before they return. Other dialogs with such a
window as their parent are shown without one.

The shim's process carries out what you ask of a window, and reports back
afterwards. So a change is not always visible to your app straight away: after
`win.show()`, for example, `win.isVisible()` is `false` until the window's
`show` event.

While a shim is the active app, the menu bar is the shim's and not your app's:
the menu you gave it with [`shim.setMenu()`](#shimsetmenumenu-experimental), or
else a short standard menu that the shim builds. Your app's application menu
is not shown, and its accelerators do not apply to the shim's windows.

Each window belongs to one shim, chosen when the window is created.

App shims are not available in Mac App Store builds.

### Instance Events

#### Event: 'activate' _Experimental_

Returns:

* `event` Event
* `hasVisibleWindows` boolean

Emitted when the person clicks the shim's Dock tile, or opens the shim again
while it is running.

#### Event: 'open-file' _Experimental_

Returns:

* `event` Event
* `path` string

Emitted when the shim is asked to open a file.

#### Event: 'open-url' _Experimental_

Returns:

* `event` Event
* `url` string

Emitted when the shim is asked to open a URL.

#### Event: 'quit-requested' _Experimental_

Returns:

* `event` Event

Emitted when the shim is asked to quit: by the person, for example with
<kbd>Cmd</kbd>+<kbd>Q</kbd> or from its Dock menu, or by macOS at logout. By
default the shim then quits, which closes its windows without emitting their
`close` events. Call `event.preventDefault()` to keep it running; you can then
close its windows yourself and call `shim.quit()`.

The shim waits a few seconds for your app to answer, and quits if it hears
nothing, so answer in the listener itself and not after asynchronous work.

#### Event: 'disconnected' _Experimental_

Returns:

* `event` Event

Emitted when the shim's process has gone away. Every window that was created
in it is already closed.

### Instance Methods

#### `shim.isConnected()` _Experimental_

Returns `boolean` - Whether the shim's process is still connected.

#### `shim.satisfiesCodeRequirement(requirement)` _Experimental_

* `requirement` string - A [code requirement](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/RequirementLang/RequirementLang.html).

Returns `boolean` - Whether the code signature of the shim's process meets
`requirement`. The check is made on the process that connected, not on the
bundle at `shim.bundlePath`.

Throws if `requirement` cannot be parsed.

#### `shim.setBadge(text)` _Experimental_

* `text` string

Sets the text shown on the shim's Dock tile. An empty string clears it.

#### `shim.bounce([type])` _Experimental_

* `type` string (optional) - Can be `critical` or `informational`. The default is
 `informational`

Bounces the shim's Dock tile, as [`dock.bounce()`](dock.md#dockbouncetype-macos)
does for your app's.

#### `shim.cancelBounce()` _Experimental_

Stops the bounce that `shim.bounce()` started.

#### `shim.setMenu(menu)` _Experimental_

* `menu` [Menu](menu.md) | null

Sets the menu bar that is shown while the shim is the active app, as
[`Menu.setApplicationMenu()`](menu.md#menusetapplicationmenumenu) does for
your own app. `null` puts back the short standard menu that the shim builds.

The menu is yours: its items' `click` handlers and its `menu-will-show` event
run in your app. Items with a `role` act on the shim, so `quit` quits the shim
and not your app, and the default labels of the `about`, `hide` and `quit`
roles name the shim.

The shim shows the items as they were when you set the menu, and catches up
with later changes to them (a label, `enabled`, `checked`, `visible`) when it
becomes the active app, when one of its menus opens, and after an item is
clicked.

#### `shim.quit()` _Experimental_

Ends the shim's process and closes its windows. The windows' `close` events are
not emitted; their `closed` events are, shortly after this method returns.
`shim.isConnected()` is `false` as soon as it returns.

### Instance Properties

#### `shim.pid` _Readonly_ _Experimental_

A `number` that is the process ID of the shim.

#### `shim.bundlePath` _Readonly_ _Experimental_

A `string` that is the path of the app bundle the shim's process runs from.

#### `shim.bundleId` _Readonly_ _Experimental_

A `string` that is the bundle identifier in that bundle's `Info.plist`. Anyone
can make a bundle with any identifier, so do not use it to decide whether to
trust the shim.
