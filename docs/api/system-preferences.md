# systemPreferences

> Get system preferences.

Process: [Main](../glossary.md#main-process)

```javascript
const { systemPreferences } = require('electron')
console.log(systemPreferences.isDarkMode())
```

## Events

The `systemPreferences` object emits the following events:

### Event: 'accent-color-changed' _Windows_

Returns:

* `event` Event
* `newColor` String - The new RGBA color the user assigned to be their system
  accent color.

### Event: 'color-changed' _Windows_

Returns:

* `event` Event

### Event: 'inverted-color-scheme-changed' _Windows_

Returns:

* `event` Event
* `invertedColorScheme` Boolean - `true` if an inverted color scheme, such as
  a high contrast theme, is being used, `false` otherwise.

### Event: 'appearance-changed' _macOS_

Returns:

* `newAppearance` String - Can be `dark` or `light`

**NOTE:** This event is only emitted after you have called `startAppLevelAppearanceTrackingOS`

## Methods

### `systemPreferences.isDarkMode()` _macOS_

Returns `Boolean` - Whether the system is in Dark Mode.

### `systemPreferences.isSwipeTrackingFromScrollEventsEnabled()` _macOS_

Returns `Boolean` - Whether the Swipe between pages setting is on.

### `systemPreferences.postNotification(event, userInfo)` _macOS_

* `event` String
* `userInfo` Object

Posts `event` as native notifications of macOS. The `userInfo` is an Object
that contains the user information dictionary sent along with the notification.

### `systemPreferences.postLocalNotification(event, userInfo)` _macOS_

* `event` String
* `userInfo` Object

Posts `event` as native notifications of macOS. The `userInfo` is an Object
that contains the user information dictionary sent along with the notification.

### `systemPreferences.postWorkspaceNotification(event, userInfo)` _macOS_

* `event` String
* `userInfo` Object

Posts `event` as native notifications of macOS. The `userInfo` is an Object
that contains the user information dictionary sent along with the notification.

### `systemPreferences.subscribeNotification(event, callback)` _macOS_

* `event` String
* `callback` Function
  * `event` String
  * `userInfo` Object
  
Returns `Number` - The ID of this subscription

Subscribes to native notifications of macOS, `callback` will be called with
`callback(event, userInfo)` when the corresponding `event` happens. The
`userInfo` is an Object that contains the user information dictionary sent
along with the notification.

The `id` of the subscriber is returned, which can be used to unsubscribe the
`event`.

Under the hood this API subscribes to `NSDistributedNotificationCenter`,
example values of `event` are:

* `AppleInterfaceThemeChangedNotification`
* `AppleAquaColorVariantChanged`
* `AppleColorPreferencesChangedNotification`
* `AppleShowScrollBarsSettingChanged`

### `systemPreferences.subscribeLocalNotification(event, callback)` _macOS_

* `event` String
* `callback` Function
  * `event` String
  * `userInfo` Object
  
Returns `Number` - The ID of this subscription

Same as `subscribeNotification`, but uses `NSNotificationCenter` for local defaults.
This is necessary for events such as `NSUserDefaultsDidChangeNotification`.

### `systemPreferences.subscribeWorkspaceNotification(event, callback)` _macOS_

* `event` String
* `callback` Function
  * `event` String
  * `userInfo` Object

Same as `subscribeNotification`, but uses `NSWorkspace.sharedWorkspace.notificationCenter`.
This is necessary for events such as `NSWorkspaceDidActivateApplicationNotification`.

### `systemPreferences.unsubscribeNotification(id)` _macOS_

* `id` Integer

Removes the subscriber with `id`.

### `systemPreferences.unsubscribeLocalNotification(id)` _macOS_

* `id` Integer

Same as `unsubscribeNotification`, but removes the subscriber from `NSNotificationCenter`.

### `systemPreferences.unsubscribeWorkspaceNotification(id)` _macOS_

* `id` Integer

Same as `unsubscribeNotification`, but removes the subscriber from `NSWorkspace.sharedWorkspace.notificationCenter`.

### `systemPreferences.registerDefaults(defaults)` _macOS_		

* `defaults` Object - a dictionary of (`key: value`) user defaults			

Add the specified defaults to your application's `NSUserDefaults`.

### `systemPreferences.getUserDefault(key, type)` _macOS_

* `key` String
* `type` String - Can be `string`, `boolean`, `integer`, `float`, `double`,
  `url`, `array` or `dictionary`.

Returns `any` - The value of `key` in `NSUserDefaults`.

Some popular `key` and `type`s are:

* `AppleInterfaceStyle`: `string`
* `AppleAquaColorVariant`: `integer`
* `AppleHighlightColor`: `string`
* `AppleShowScrollBars`: `string`
* `NSNavRecentPlaces`: `array`
* `NSPreferredWebServices`: `dictionary`
* `NSUserDictionaryReplacementItems`: `array`

### `systemPreferences.setUserDefault(key, type, value)` _macOS_

* `key` String
* `type` String - See [`getUserDefault`](#systempreferencesgetuserdefaultkey-type-macos).
* `value` String

Set the value of `key` in `NSUserDefaults`.

Note that `type` should match actual type of `value`. An exception is thrown
if they don't.

Some popular `key` and `type`s are:

* `ApplePressAndHoldEnabled`: `boolean`

### `systemPreferences.removeUserDefault(key)` _macOS_

* `key` String

Removes the `key` in `NSUserDefaults`. This can be used to restore the default
or global value of a `key` previously set with `setUserDefault`.

### `systemPreferences.isAeroGlassEnabled()` _Windows_

Returns `Boolean` - `true` if [DWM composition][dwm-composition] (Aero Glass) is
enabled, and `false` otherwise.

An example of using it to determine if you should create a transparent window or
not (transparent windows won't work correctly when DWM composition is disabled):

```javascript
const { BrowserWindow, systemPreferences } = require('electron')
let browserOptions = { width: 1000, height: 800 }

// Make the window transparent only if the platform supports it.
if (process.platform !== 'win32' || systemPreferences.isAeroGlassEnabled()) {
  browserOptions.transparent = true
  browserOptions.frame = false
}

// Create the window.
let win = new BrowserWindow(browserOptions)

// Navigate.
if (browserOptions.transparent) {
  win.loadURL(`file://${__dirname}/index.html`)
} else {
  // No transparency, so we load a fallback that uses basic styles.
  win.loadURL(`file://${__dirname}/fallback.html`)
}
```

[dwm-composition]:https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx

### `systemPreferences.getAccentColor()` _Windows_

Returns `String` - The users current system wide accent color preference in RGBA
hexadecimal form.

```js
const color = systemPreferences.getAccentColor() // `"aabbccdd"`
const red = color.substr(0, 2) // "aa"
const green = color.substr(2, 2) // "bb"
const blue = color.substr(4, 2) // "cc"
const alpha = color.substr(6, 2) // "dd"
```

### `systemPreferences.getColor(color)` _Windows_

* `color` String - One of the following values:
  * `3d-dark-shadow` - Dark shadow for three-dimensional display elements.
  * `3d-face` - Face color for three-dimensional display elements and for dialog
    box backgrounds.
  * `3d-highlight` - Highlight color for three-dimensional display elements.
  * `3d-light` - Light color for three-dimensional display elements.
  * `3d-shadow` - Shadow color for three-dimensional display elements.
  * `active-border` - Active window border.
  * `active-caption` - Active window title bar. Specifies the left side color in
    the color gradient of an active window's title bar if the gradient effect is
    enabled.
  * `active-caption-gradient` - Right side color in the color gradient of an
    active window's title bar.
  * `app-workspace` - Background color of multiple document interface (MDI)
    applications.
  * `button-text` - Text on push buttons.
  * `caption-text` - Text in caption, size box, and scroll bar arrow box.
  * `desktop` - Desktop background color.
  * `disabled-text` - Grayed (disabled) text.
  * `highlight` - Item(s) selected in a control.
  * `highlight-text` - Text of item(s) selected in a control.
  * `hotlight` - Color for a hyperlink or hot-tracked item.
  * `inactive-border` - Inactive window border.
  * `inactive-caption` - Inactive window caption. Specifies the left side color
    in the color gradient of an inactive window's title bar if the gradient
    effect is enabled.
  * `inactive-caption-gradient` - Right side color in the color gradient of an
    inactive window's title bar.
  * `inactive-caption-text` - Color of text in an inactive caption.
  * `info-background` - Background color for tooltip controls.
  * `info-text` - Text color for tooltip controls.
  * `menu` - Menu background.
  * `menu-highlight` - The color used to highlight menu items when the menu
    appears as a flat menu.
  * `menubar` - The background color for the menu bar when menus appear as flat
    menus.
  * `menu-text` - Text in menus.
  * `scrollbar` - Scroll bar gray area.
  * `window` - Window background.
  * `window-frame` - Window frame.
  * `window-text` - Text in windows.

Returns `String` - The system color setting in RGB hexadecimal form (`#ABCDEF`).
See the [Windows docs][windows-colors] for more details.

### `systemPreferences.isInvertedColorScheme()` _Windows_

Returns `Boolean` - `true` if an inverted color scheme, such as a high contrast
theme, is active, `false` otherwise.

[windows-colors]:https://msdn.microsoft.com/en-us/library/windows/desktop/ms724371(v=vs.85).aspx

### `systemPreferences.getEffectiveAppearance()` _macOS_

Returns `String` - Can be `dark`, `light` or `unknown`.

Gets the macOS appearance setting that is currently applied to your application,
maps to [NSApplication.effectiveAppearance](https://developer.apple.com/documentation/appkit/nsapplication/2967171-effectiveappearance?language=objc)

Please note that until Electron is built targeting the 10.14 SDK, your application's
`effectiveAppearance` will default to 'light' and won't inherit the OS preference. In
the interim in order for your application to inherit the OS preference you must set the
`NSRequiresAquaSystemAppearance` key in your apps `Info.plist` to `false`.  If you are
using `electron-packager` or `electron-forge` just set the `enableDarwinDarkMode`
packager option to `true`.  See the [Electron Packager API](https://github.com/electron-userland/electron-packager/blob/master/docs/api.md#darwindarkmodesupport)
for more details.

### `systemPreferences.getAppLevelAppearance()` _macOS_

Returns `String` | `null` - Can be `dark`, `light` or `unknown`.

Gets the macOS appearance setting that you have declared you want for
your application, maps to [NSApplication.appearance](https://developer.apple.com/documentation/appkit/nsapplication/2967170-appearance?language=objc).
You can use the `setAppLevelAppearance` API to set this value.

### `systemPreferences.setAppLevelAppearance(appearance)` _macOS_

* `appearance` String | null - Can be `dark` or `light`

Sets the appearance setting for your application, this should override the
system default and override the value of `getEffectiveAppearance`.

### `systemPreferences.isTrustedAccessibilityClient(prompt)` _macOS_

* `prompt` Boolean - whether or not the user will be informed via prompt if the current process is untrusted.

Returns `Boolean` - `true` if the current process is a trusted accessibility client and `false` if it is not.

### `systemPreferences.getMediaAccessStatus(mediaType)` _macOS_

* `mediaType` String - `microphone` or `camera`.

Returns `String` - Can be `not-determined`, `granted`, `denied`, `restricted` or `unknown`.

This user consent was not required until macOS 10.14 Mojave, so this method will always return `granted` if your system is running 10.13 High Sierra or lower.

### `systemPreferences.askForMediaAccess(mediaType)` _macOS_

* `mediaType` String - the type of media being requested; can be `microphone`, `camera`.

Returns `Promise<Boolean>` - A promise that resolves with `true` if consent was granted and `false` if it was denied. If an invalid `mediaType` is passed, the promise will be rejected. If an access request was denied and later is changed through the System Preferences pane, a restart of the app will be required for the new permissions to take effect. If access has already been requested and denied, it _must_ be changed through the preference pane; an alert will not pop up and the promise will resolve with the existing access status.

**Important:** In order to properly leverage this API, you [must set](https://developer.apple.com/documentation/avfoundation/cameras_and_media_capture/requesting_authorization_for_media_capture_on_macos?language=objc) the `NSMicrophoneUsageDescription` and `NSCameraUsageDescription` strings in your app's `Info.plist` file. The values for these keys will be used to populate the permission dialogs so that the user will be properly informed as to the purpose of the permission request. See [Electron Application Distribution](https://electronjs.org/docs/tutorial/application-distribution#macos) for more information about how to set these in the context of Electron.

This user consent was not required until macOS 10.14 Mojave, so this method will always return `true` if your system is running 10.13 High Sierra or lower.