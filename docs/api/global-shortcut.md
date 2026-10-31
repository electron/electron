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
* **Shortcuts carry a description.** `register()` accepts an object so an app can label a shortcut with
  what it does; that text is what KDE and GNOME show in their shortcut settings. The description is not
  part of a shortcut's identity, so changing it relabels the existing entry instead of adding a new one.
  It only takes effect when the portal binds the shortcut, i.e. at startup or on the next launch.
* **The portal path is enabled by default.** The `GlobalShortcutsPortal` and
  `GlobalShortcutsPortalPreferredTrigger` features are on by default, so no feature flags are needed —
  including on GNOME (see [#52221](https://github.com/electron/electron/pull/52221)).

## Events

The `globalShortcut` module emits the following events:

### Event: 'registration-resolved' _Linux_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/52224
```
-->

Returns:

* `accelerator` string - The accelerator, as it was passed to `register()`. When the shortcut was
  registered with an object, this is its `accelerator` property.
* `bound` boolean - Whether the portal accepted the bind request.

Emitted on Linux Wayland when the `org.freedesktop.portal.GlobalShortcuts` portal resolves a
registration request. The bind on this path is asynchronous and consent-gated, so the boolean
returned by `register()` only refers to if the request was successfully submitted; this event reports the real outcome.
`bound` is `false` when the portal denied the bind (for example an invalid app identity — see
[`app.setDesktopName()`](app.md#appsetdesktopnamename-linux) — or a dismissed consent dialog) or the
portal is unavailable.

The event is always emitted asynchronously, and may be emitted more than once for the same
accelerator: registering an additional shortcut re-binds the whole set, which re-resolves
previously bound accelerators.

This event is not emitted on X11 or on other platforms, where the return value of `register()` is
already authoritative.

## Methods

The `globalShortcut` module has the following methods:

### `globalShortcut.register(shortcut, callback)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
changes:
  - pr-url: https://github.com/electron/electron/pull/55167
    description: "The `shortcut` parameter now accepts a `ShortcutRegistration` object in addition to an accelerator string."
```
-->

* `shortcut` (string | [ShortcutRegistration](structures/shortcut-registration.md)) - An
  [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut, or an object describing it.
* `callback` Function

Returns `boolean` - Whether or not the shortcut was registered successfully.

Registers a global shortcut of `shortcut`. The `callback` is called when
the registered shortcut is pressed by the user.

When the accelerator is already taken by other applications, this call will
silently fail. This behavior is intended by operating systems, since they don't
want applications to fight for global shortcuts.

On Linux Wayland, the return value only indicates that the bind request was submitted to the
desktop portal; the portal resolves it asynchronously and the actual outcome is reported by the
[`registration-resolved`](#event-registration-resolved-linux) event.

The following accelerators will not be registered successfully on macOS 10.14 Mojave unless
the app has been authorized as a [trusted accessibility client](https://developer.apple.com/library/archive/documentation/Accessibility/Conceptual/AccessibilityMacOSX/OSXAXTestingApps.html):

* "Media Play/Pause"
* "Media Next Track"
* "Media Previous Track"
* "Media Stop"

### `globalShortcut.registerAll(shortcuts, callback)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/15542
changes:
  - pr-url: https://github.com/electron/electron/pull/55167
    description: "The `shortcuts` parameter now accepts `ShortcutRegistration` objects in addition to accelerator strings."
```
-->

* `shortcuts` (string | [ShortcutRegistration](structures/shortcut-registration.md))[] - An array of
  [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcuts, or objects describing them.
  Strings and objects may be mixed in the same array.
* `callback` Function

Registers a global shortcut of all `shortcut` items in `shortcuts`. The `callback` is called when any of the registered shortcuts are pressed by the user.

When a given accelerator is already taken by other applications, this call will
silently fail. This behavior is intended by operating systems, since they don't
want applications to fight for global shortcuts.

The following accelerators will not be registered successfully on macOS 10.14 Mojave unless
the app has been authorized as a [trusted accessibility client](https://developer.apple.com/library/archive/documentation/Accessibility/Conceptual/AccessibilityMacOSX/OSXAXTestingApps.html):

* "Media Play/Pause"
* "Media Next Track"
* "Media Previous Track"
* "Media Stop"

### `globalShortcut.isRegistered(shortcut)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
changes:
  - pr-url: https://github.com/electron/electron/pull/55167
    description: "The `shortcut` parameter now accepts a `ShortcutRegistration` object in addition to an accelerator string."
```
-->

* `shortcut` (string | [ShortcutRegistration](structures/shortcut-registration.md)) - An
  [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut, or an object describing it.
  The object's `description`, if any, is ignored.

Returns `boolean` - Whether this application has registered `shortcut`.

When the accelerator is already taken by other applications, this call will
still return `false`. This behavior is intended by operating systems, since they
don't want applications to fight for global shortcuts.

### `globalShortcut.unregister(shortcut)`

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/534
changes:
  - pr-url: https://github.com/electron/electron/pull/55167
    description: "The `shortcut` parameter now accepts a `ShortcutRegistration` object in addition to an accelerator string."
```
-->

* `shortcut` (string | [ShortcutRegistration](structures/shortcut-registration.md)) - An
  [accelerator](../tutorial/keyboard-shortcuts.md#accelerators) shortcut, or an object describing it.
  The object's `description`, if any, is ignored.

Unregisters the global shortcut of `shortcut`.

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
