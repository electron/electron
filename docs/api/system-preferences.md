# systemPreferences

> Get system preferences.

```javascript
const {systemPreferences} = require('electron');
console.log(systemPreferences.isDarkMode());
```

## Methods

### `systemPreferences.isDarkMode()` _macOS_

This method returns `true` if the system is in Dark Mode, and `false` otherwise.

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
let browserOptions = {width: 1000, height: 800};

// Make the window transparent only if the platform supports it.
if (process.platform !== 'win32' || systemPreferences.isAeroGlassEnabled()) {
  browserOptions.transparent = true;
  browserOptions.frame = false;
}

// Create the window.
let win = new BrowserWindow(browserOptions);

// Navigate.
if (browserOptions.transparent) {
  win.loadURL('file://' + __dirname + '/index.html');
} else {
  // No transparency, so we load a fallback that uses basic styles.
  win.loadURL('file://' + __dirname + '/fallback.html');
}
```

[dwm-composition]:https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx
