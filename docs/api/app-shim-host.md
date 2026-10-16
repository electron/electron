## Class: AppShimHost

> Show windows under a separate app with its own Dock icon.

> [!NOTE]
> App shims are experimental. This API may change in a future Electron
> release.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

An app shim is a small macOS app, in its own bundle, that displays windows on
behalf of your app. Your app still creates each window, loads its content and
handles its events, but the native window belongs to the shim's process. As a
result, the Dock, the app switcher, Mission Control and Stage Manager show the
window under the shim's name and icon instead of your app's.

An `AppShimHost` represents your app's connection to one running shim. Your
app receives one in the
[`app-shim-connected`](app.md#event-app-shim-connected-macos-experimental)
event after calling
[`app.startAppShimServer()`](app.md#appstartappshimserveroptions-macos-experimental).

```js
const { app, BrowserWindow } = require('electron')

app.whenReady().then(() => {
  app.startAppShimServer()
})

app.on('app-shim-connected', (event, shim) => {
  // Any process running as the same user can connect, so verify the shim.
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

**Creating an app shim**

A shim is an app bundle whose executable is a copy of
`Electron Framework.framework/Helpers/app_shim_loader` from your packaged app.
The loader does not link against Electron. It loads your app's Electron
Framework when it starts, so a shim created by one version of your app keeps
working after your app is updated.

The shim's `Info.plist` holds the usual keys (`CFBundleExecutable`,
`CFBundleIdentifier`, `CFBundleName`, `CFBundleIconFile` and so on) as well as these keys:

* `ElectronAppShimHostPath` string - The absolute path of your app's `.app`
  bundle.
* `ElectronAppShimHostBundleIdentifier` string - Your app's bundle identifier.
  It is used to locate your app if `ElectronAppShimHostPath` is missing or no
  longer points to an app.
* `ElectronAppShimServerName` string (optional) - The `name` your app passes
  to `app.startAppShimServer()`, if it passes one.
* `ElectronAppShimHostArguments` string[] (optional) - Command-line arguments
  used to launch your app if it is not running when the shim starts.

When a shim starts, it connects to your app. If your app is not running, the
shim launches it in the background and waits up to one minute for it to call
`app.startAppShimServer()`.

In a signed app, macOS only allows the shim to load your framework if both are
signed by the same team.

**App updates**

A shim can only connect to your app while your app is running the same version
that is installed on disk. If your app's bundle is replaced while your app is
running, any shim launched afterwards quits immediately without showing any
UI, and your app receives an
[`app-shim-version-mismatch`](app.md#event-app-shim-version-mismatch-macos-experimental)
event. Use that event to ask the user to restart your app.

Updates installed by [`autoUpdater`](auto-updater.md) are applied after your
app quits, so they do not cause a mismatch.

**Limitations**

App shims are not available in Mac App Store builds.

A window in an app shim has no native window in your app's process. Window
APIs that operate directly on the native window are not supported yet and do
nothing:

* The Touch Bar
* The window controls overlay (`titleBarOverlay`)
* Simple fullscreen
* `win.setEnabled()`
* `win.flashFrame()` (use [`shim.bounce()`](#shimbouncetype-experimental) instead)
* `win.setProgressBar()`
* `win.setHasShadow()`
* `win.setRepresentedFilename()`
* `win.setDocumentEdited()`
* `win.setHiddenInMissionControl()`

`win.getNativeWindowHandle()` and `win.getMediaSourceId()` refer to a hidden
placeholder window in your app's process, not to the window on screen.

Other differences from a regular window:

* A window's shim is set when the window is created and cannot be changed.
* A window in a shim cannot be `modal`, cannot have a `parent` and cannot be
  the `parent` of another window. Attempting any of these throws an error.
* `win.addTabbedWindow()` only works between two windows in the same shim.
* Window operations are sent to the shim's process and applied asynchronously,
  so window state may not update immediately. For example, after calling
  `win.show()`, `win.isVisible()` returns `false` until the window emits its
  `show` event.
* Message boxes and open and save dialogs from the [`dialog`](dialog.md)
  module can use a window in a shim as their parent. The shim displays them
  as sheets attached to that window. The synchronous variants, such as
  `dialog.showMessageBoxSync()`, do not block your app while the dialog is
  open, so your app may handle other events before they return. Other dialogs
  are shown without a parent window.
* While a shim is the active app, macOS shows the shim's menu bar instead of
  your app's. Set it with
  [`shim.setApplicationMenu()`](#shimsetapplicationmenumenu-experimental). If
  no menu is set, the shim shows a minimal default menu. Your app's
  application menu and its accelerators do not apply to windows in a shim.

### Instance Events

#### Event: 'activate' _Experimental_

Returns:

* `event` Event
* `hasVisibleWindows` boolean

Emitted when the user clicks the shim's Dock icon, or launches the shim again
while it is already running.

#### Event: 'open-file' _Experimental_

Returns:

* `event` Event
* `path` string

Emitted when the user opens a file with the shim.

#### Event: 'open-url' _Experimental_

Returns:

* `event` Event
* `url` string

Emitted when the user opens a URL with the shim.

#### Event: 'quit-requested' _Experimental_

Returns:

* `event` Event

Emitted when the shim is asked to quit: by the user, for example with
<kbd>Cmd</kbd>+<kbd>Q</kbd> or from its Dock menu, or by macOS at logout.

By default, the shim quits and its windows are closed without emitting their
`close` events. To keep the shim running, call `event.preventDefault()`. You
can then close its windows yourself and call `shim.quit()` when you are done.

`event.preventDefault()` must be called synchronously in the listener. If your
app's main process is unresponsive for more than five seconds, the shim quits
without waiting for it.

#### Event: 'disconnected' _Experimental_

Returns:

* `event` Event

Emitted when the shim's process has exited or its connection was lost. All
windows that were created in the shim have already been closed.

### Instance Methods

#### `shim.isConnected()` _Experimental_

Returns `boolean` - Whether the shim's process is still connected.

#### `shim.satisfiesCodeRequirement(requirement)` _Experimental_

* `requirement` string - A [code requirement](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/RequirementLang/RequirementLang.html).

Returns `boolean` - Whether the code signature of the shim's process satisfies
`requirement`. The check is performed on the running process that connected,
not on the bundle at `shim.bundlePath`.

Throws if `requirement` cannot be parsed.

#### `shim.setBadge(text)` _Experimental_

* `text` string

Sets the text displayed on the shim's Dock icon. An empty string clears it.

#### `shim.bounce([type])` _Experimental_

* `type` string (optional) - Can be `critical` or `informational`. The default is
 `informational`

Bounces the shim's Dock icon, in the same way that
[`dock.bounce()`](dock.md#dockbouncetype-macos) bounces your app's.

#### `shim.cancelBounce()` _Experimental_

Cancels a bounce started by `shim.bounce()`.

#### `shim.setApplicationMenu(menu)` _Experimental_

* `menu` [Menu](menu.md) | null

Sets the menu bar that is shown while the shim is the active app. This is the
shim's equivalent of
[`Menu.setApplicationMenu()`](menu.md#menusetapplicationmenumenu). Passing
`null` restores the shim's minimal default menu.

The `click` handlers of the menu's items and the menu's `menu-will-show` event
run in your app's main process. Items with a `role` act on the shim instead of
your app: for example, the `quit` role quits the shim. The default labels of
the `about`, `hide` and `quit` roles use the shim's name.

The shim displays the menu's items as they were when this method was called.
Later changes to an item's `label`, `enabled`, `checked` or `visible`
properties are applied when the shim becomes the active app, when one of its
menus is opened, and after an item is clicked.

#### `shim.quit()` _Experimental_

Quits the shim's process and closes its windows. The windows do not emit
`close` events. They emit `closed` shortly after this method returns.
`shim.isConnected()` returns `false` as soon as this method returns.

### Instance Properties

#### `shim.pid` _Readonly_ _Experimental_

A `number` representing the process ID of the shim.

#### `shim.bundlePath` _Readonly_ _Experimental_

A `string` representing the path of the app bundle that the shim's process was
launched from.

#### `shim.bundleId` _Readonly_ _Experimental_

A `string` representing the bundle identifier from that bundle's `Info.plist`.
This value is not verified, so do not use it to decide whether to trust the
shim. Use
[`shim.satisfiesCodeRequirement()`](#shimsatisfiescoderequirementrequirement-experimental)
instead.
