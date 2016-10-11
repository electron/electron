# systemPreferences

> Get system preferences.

```javascript
const {systemPreferences} = require('electron')
console.log(systemPreferences.isDarkMode())
```

## Events

The `systemPreferences` object emits the following events:

### Event: 'accent-color-changed' _Windows_

Returns:

* `event` Event
* `newColor` String - The new RGBA color the user assigned to be there system
  accent color.

### Event: 'inverted-color-scheme-changed' _Windows_

Returns:

* `event` Event
* `invertedColorScheme` Boolean - `true` if an inverted color scheme, such as
  a high contrast theme, is being used, `false` otherwise.

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

### `systemPreferences.subscribeNotification(event, callback)` _macOS_

* `event` String
* `callback` Function

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

### `systemPreferences.unsubscribeNotification(id)` _macOS_

* `id` Integer

Removes the subscriber with `id`.

### `systemPreferences.subscribeLocalNotification(event, callback)` _macOS_

* `event` String
* `callback` Function

Same as `subscribeNotification`, but uses `NSNotificationCenter` for local defaults.
This is necessary for events such as `NSUserDefaultsDidChangeNotification`

### `systemPreferences.unsubscribeLocalNotification(id)` _macOS_

* `id` Integer

Same as `unsubscribeNotification`, but removes the subscriber from `NSNotificationCenter`.

### `systemPreferences.getUserDefault(key, type)` _macOS_

* `key` String
* `type` String - Can be `string`, `boolean`, `integer`, `float`, `double`,
  `url`, `array`, `dictionary`

Get the value of `key` in system preferences.

This API reads from `NSUserDefaults` on macOS, some popular `key` and `type`s
are:

* `AppleInterfaceStyle: string`
* `AppleAquaColorVariant: integer`
* `AppleHighlightColor: string`
* `AppleShowScrollBars: string`
* `NSNavRecentPlaces: array`
* `NSPreferredWebServices: dictionary`
* `NSUserDictionaryReplacementItems: array`

### `systemPreferences.isAeroGlassEnabled()` _Windows_

This method returns `true` if [DWM composition][dwm-composition] (Aero Glass) is
enabled, and `false` otherwise.

An example of using it to determine if you should create a transparent window or
not (transparent windows won't work correctly when DWM composition is disabled):

```javascript
const {BrowserWindow, systemPreferences} = require('electron')
let browserOptions = {width: 1000, height: 800}

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
