# globalShortcut

> Detect keyboard events when the application does not have keyboard focus.

Process: [Main](../glossary.md#main-process)

The `globalShortcut` module can register/unregister a global keyboard shortcut
with the operating system so that you can customize the operations for various
shortcuts.

> [!NOTE]
> The shortcut is global; it will work even if the app does
> not have the keyboard focus. This module cannot be used before the `ready`
> event of the app module is emitted.
> On Linux Wayland sessions, shortcuts are bound through the desktop portal —
> see [Usage on Linux](#usage-on-linux) below.

```js
const { app, globalShortcut } = require('electron')

app.whenReady().then(() => {
  // Register a 'CommandOrControl+X' shortcut listener.
  const ret = globalShortcut.register('CommandOrControl+X', () => {
    console.log('CommandOrControl+X is pressed')
  })

  if (!ret) {
    console.log('registration failed')
  }

  // Check whether a shortcut is registered.
  console.log(globalShortcut.isRegistered('CommandOrControl+X'))
})

app.on('will-quit', () => {
  // Unregister a shortcut.
  globalShortcut.unregister('CommandOrControl+X')

  // Unregister all shortcuts.
  globalShortcut.unregisterAll()
})
```

> [!TIP]
> See also: [A detailed guide on Keyboard Shortcuts](../tutorial/keyboard-shortcuts.md).

## Usage on Linux

On X11, shortcuts are grabbed directly from the X server and work like on other platforms.

On Wayland, compositors do not let apps grab keys directly. Electron instead uses the
[`org.freedesktop.portal.GlobalShortcuts`](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.GlobalShortcuts.html)
portal, which changes the module's behavior in several ways:

* **Your app must have a valid portal identity.** Set `desktopName` in `package.json` (or call
  [`app.setDesktopName()`](app.md#appsetdesktopnamename-linux)) to a reverse-DNS ID matching your installed
  `.desktop` file, e.g. `com.example.MyApp.desktop`. With an invalid or unresolvable ID, GNOME 50.0/50.1
  denies every bind — silently, from the app's perspective.
* **GNOME shows a consent dialog** the first time an app binds shortcuts, pre-filled with the accelerators
  the app requested. KDE Plasma binds silently without a dialog (shortcuts are visible and editable in
  System Settings).
* **Bindings persist across relaunches**, keyed by the app's portal identity: after the user accepts once,
  registering the same shortcuts on later launches re-binds them silently.
* **`register()` is optimistic on this path.** The portal bind is asynchronous and consent-gated, so
  `register()` returning `true` means the request was submitted, not that the shortcut is bound. If the
  portal denies the bind (invalid app identity, dismissed dialog), there is currently no way to observe the
  failure — `isRegistered()` will also report `true`. See
  [#52218](https://github.com/electron/electron/issues/52218).
* **On GNOME, the portal path currently requires a feature flag.** Chromium keeps the portal listener
  disabled on GNOME, so until [#52221](https://github.com/electron/electron/pull/52221) ships, apps must
  launch with `--enable-features=GlobalShortcutsPortalPreferredTrigger` (e.g. via
  `app.commandLine.appendSwitch()`) — otherwise `register()` returns `false` on GNOME Wayland. Other
  desktops only need the default-enabled `GlobalShortcutsPortal` feature.

## Methods

The `globalShortcut` module has the following methods:

### `globalShortcut.register(accelerator, callback)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
```
-->

* `accelerator` string - An [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut.
* `callback` Function

Returns `boolean` - Whether or not the shortcut was registered successfully.

Registers a global shortcut of `accelerator`. The `callback` is called when
the registered shortcut is pressed by the user.

When the accelerator is already taken by other applications, this call will
silently fail. This behavior is intended by operating systems, since they don't
want applications to fight for global shortcuts.

On Linux Wayland the returned value is optimistic — the portal resolves the
bind asynchronously. See [Usage on Linux](#usage-on-linux).

The following accelerators will not be registered successfully on macOS 10.14 Mojave unless
the app has been authorized as a [trusted accessibility client](https://developer.apple.com/library/archive/documentation/Accessibility/Conceptual/AccessibilityMacOSX/OSXAXTestingApps.html):

* "Media Play/Pause"
* "Media Next Track"
* "Media Previous Track"
* "Media Stop"

### `globalShortcut.registerAll(accelerators, callback)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/15542
```
-->

* `accelerators` string[] - An array of [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcuts.
* `callback` Function

Registers a global shortcut of all `accelerator` items in `accelerators`. The `callback` is called when any of the registered shortcuts are pressed by the user.

When a given accelerator is already taken by other applications, this call will
silently fail. This behavior is intended by operating systems, since they don't
want applications to fight for global shortcuts.

The following accelerators will not be registered successfully on macOS 10.14 Mojave unless
the app has been authorized as a [trusted accessibility client](https://developer.apple.com/library/archive/documentation/Accessibility/Conceptual/AccessibilityMacOSX/OSXAXTestingApps.html):

* "Media Play/Pause"
* "Media Next Track"
* "Media Previous Track"
* "Media Stop"

### `globalShortcut.isRegistered(accelerator)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
```
-->

* `accelerator` string - An [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut.

Returns `boolean` - Whether this application has registered `accelerator`.

When the accelerator is already taken by other applications, this call will
still return `false`. This behavior is intended by operating systems, since they
don't want applications to fight for global shortcuts.

On Linux Wayland this reflects whether a bind was requested, not whether the
portal granted it. See [Usage on Linux](#usage-on-linux).

### `globalShortcut.unregister(accelerator)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
```
-->

* `accelerator` string - An [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut.

Unregisters the global shortcut of `accelerator`.

### `globalShortcut.unregisterAll()`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
```
-->

Unregisters all of the global shortcuts.

### `globalShortcut.setSuspended(suspended)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/50425
```
-->

* `suspended` boolean - Whether global shortcut handling should be suspended.

Suspends or resumes global shortcut handling. When suspended, all registered
global shortcuts will stop listening for key presses. When resumed, all
previously registered shortcuts will begin listening again. New shortcut
registrations will fail while handling is suspended.

This can be useful when you want to temporarily allow the user to press key
combinations without your application intercepting them, for example while
displaying a UI to rebind shortcuts.

### `globalShortcut.isSuspended()`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/50425
```
-->

Returns `boolean` - Whether global shortcut handling is currently suspended.
