# BrowserWindowConstructorOptions Object extends `BaseWindowConstructorOptions`

* `webPreferences` [WebPreferences](web-preferences.md?inline) (optional) - Settings of web page's features.
* `paintWhenInitiallyHidden` boolean (optional) - Whether the renderer should be active when `show` is `false` and it has just been created.  In order for `document.visibilityState` to work correctly on first load with `show: false` you should set this to `false`.  Setting this to `false` will cause the `ready-to-show` event to not fire.  Default is `true`.
