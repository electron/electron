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

* `accelerator` string - An [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut.

Returns `boolean` - Whether this application has registered `accelerator`.

When the accelerator is already taken by other applications, this call will
still return `false`. This behavior is intended by operating systems, since they
don't want applications to fight for global shortcuts.

### `globalShortcut.unregister(accelerator)`

* `accelerator` string - An [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut.

Unregisters the global shortcut of `accelerator`.

### `globalShortcut.unregisterAll()`

Unregisters all of the global shortcuts.
