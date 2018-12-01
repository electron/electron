# Mojave Dark Mode

In macOS 10.14 Mojave, Apple introduced a new [system-wide dark mode](https://developer.apple.com/design/human-interface-guidelines/macos/visual-design/dark-mode/)
for all macOS computers.  By default Electron apps do not automatically adjust their UI and native interfaces
to the dark mode setting when it's enabled. This is primarily due to Apple's own guidelines saying you **shouldn't**
use the dark mode native interfaces if your app's own interfaces don't support dark mode themselves.

If your app does have a dark mode, you can make your Electron app follow the system-wide dark mode setting.

## Automatically updating the native interfaces

"Native Interfaces" include the file picker, window border, dialogs, context menus and more; basically anything where
the UI comes from macOS and not your app.  In order to make these interfaces update to dark mode automatically, you need
to set the `NSRequiresAquaSystemAppearance` key in your app's `Info.plist` file to `false`.  E.g.

```xml
<plist>
<dict>
  ...
  <key>NSRequiresAquaSystemAppearance</key>
  <false />
  ...
</dict>
</plist>
```

If you are using [`electron-packager` >= 12.2.0](https://github.com/electron-userland/electron-packager) or
[`electron-forge` >= 6](https://github.com/electron-userland/electron-forge) you can set the
[`darwinDarkModeSupport`](https://github.com/electron-userland/electron-packager/blob/master/docs/api.md#darwindarkmodesupport)
option when packaging and this key will be set for you.

If you are using [`electron-builder` >= 20.37.0](https://github.com/electron-userland/electron-builder) you can set the [`darkModeSupport`](https://www.electron.build/configuration/mac.html) option.

## Automatically updating your own interfaces

If your app has its own dark mode you should toggle it on and off in sync with the system's dark mode setting.  You can do
this by listening for the theme changed event on Electron's `systemPreferences` module.  E.g.

```js
const { systemPreferences } = require('electron')

systemPreferences.subscribeNotification(
  'AppleInterfaceThemeChangedNotification',
  function theThemeHasChanged () {
    updateMyAppTheme(systemPreferences.isDarkMode())
  }
)
```
