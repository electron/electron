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
> Please also note that it is also possible to use Chromium's
> `GlobalShortcutsPortal` implementation, which allows apps to bind global
> shortcuts when running within a Wayland session.

```js
const { app, globalShortcut } = require('electron')

// Enable usage of Portal's globalShortcuts. This is essential for cases when
// the app runs in a Wayland session.
app.commandLine.appendSwitch('enable-features', 'GlobalShortcutsPortal')

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

### `globalShortcut.listShortcuts()` _Linux_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/52239
```
-->

Returns `Promise<GlobalShortcutInfo[]>` - Resolves with an array of
[`GlobalShortcutInfo`](structures/global-shortcut-info.md) objects for the
shortcuts that are currently registered with the
[GlobalShortcuts desktop portal][gs-portal] for this application, including
shortcuts approved during previous application sessions.

When shortcut registration is handled by the desktop portal (e.g. when running
in a Wayland session with the `GlobalShortcutsPortal` feature enabled), calling
`globalShortcut.register` for a shortcut that has not yet been approved may
require user approval — depending on the compositor, this can show a dialog
or open the system's shortcut settings. This method lets an application check
whether a shortcut has already been approved — and therefore can be
re-registered without prompting — before registering it:

```js
const { app, globalShortcut } = require('electron')

app.commandLine.appendSwitch('enable-features', 'GlobalShortcutsPortal')

app.whenReady().then(async () => {
  const shortcuts = await globalShortcut.listShortcuts()

  // 'CommandOrControl+X' is normalized to 'Ctrl+X' on Linux.
  const approved = shortcuts.some((shortcut) => shortcut.accelerator === 'Ctrl+X')
  console.log(`Ctrl+X is ${approved ? 'already approved' : 'not yet approved'}`)
})
```

The returned promise is rejected when shortcut registration is not handled by
the desktop portal, e.g. on Windows and macOS, in an X11 session, or when the
portal service is unavailable.

Approvals are managed by the compositor and persist across restarts;
`globalShortcut.unregister` and `globalShortcut.unregisterAll` do not revoke
them. Results may briefly lag behind registrations made in the same
session.

[gs-portal]: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.GlobalShortcuts.html

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
