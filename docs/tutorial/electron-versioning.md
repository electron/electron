# Electron Versioning

If you are a seasoned Node developer, you are surely aware of `semver` - and
might be used to giving your dependency management systems only rough guidelines
rather than fixed version numbers. Due to the hard dependency on Node and
Chromium, Electron is in a slightly more difficult position and does not follow
semver. You should therefore always reference a specific version of Electron.

Version numbers are bumped using the following rules:

* Major: For breaking changes in Electron's API - if you upgrade from `0.37.0`
  to `1.0.0`, you will have to update your app.
* Minor: For major Chrome and minor Node upgrades; or significant Electron
  changes - if you upgrade from `1.0.0` to `1.1.0`, your app is supposed to
  still work, but you might have to work around small changes.
* Patch: For new features and bug fixes - if you upgrade from `1.0.0` to
  `1.0.1`, your app will continue to work as-is.

If you are using `electron-prebuilt`, we recommend that you set a fixed version
number (`1.1.0` instead of `^1.1.0`) to ensure that all upgrades of Electron are
a manual operation made by you, the developer.
