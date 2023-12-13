# BrowserWindowConstructorOptions Object extends `BaseWindowConstructorOptions`

* `webPreferences` [WebPreferences](web-preferences.md?inline) (optional) - Settings of web page's features.
* `paintWhenInitiallyHidden` boolean (optional) - Whether the renderer should be active when `show` is `false` and it has just been created.  In order for `document.visibilityState` to work correctly on first load with `show: false` you should set this to `false`.  Setting this to `false` will cause the `ready-to-show` event to not fire.  Default is `true`.
* `titleBarOverlay` Object | Boolean (optional) -  When using a frameless window in conjunction with `win.setWindowButtonVisibility(true)` on macOS or using a `titleBarStyle` so that the standard window controls ("traffic lights" on macOS) are visible, this property enables the Window Controls Overlay [JavaScript APIs][overlay-javascript-apis] and [CSS Environment Variables][overlay-css-env-vars]. Specifying `true` will result in an overlay with default system colors. Default is `false`.
  * `color` String (optional) _Windows_ - The CSS color of the Window Controls Overlay when enabled. Default is the system color.
  * `symbolColor` String (optional) _Windows_ - The CSS color of the symbols on the Window Controls Overlay when enabled. Default is the system color.
  * `height` Integer (optional) _macOS_ _Windows_ - The height of the title bar and Window Controls Overlay in pixels. Default is system height.

[overlay-css-env-vars]: https://github.com/WICG/window-controls-overlay/blob/main/explainer.md#css-environment-variables
[overlay-javascript-apis]: https://github.com/WICG/window-controls-overlay/blob/main/explainer.md#javascript-apis
