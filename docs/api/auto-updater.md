# autoUpdater

> Enable apps to automatically update themselves.

The `autoUpdater` module provides an interface for the
[Squirrel](https://github.com/Squirrel) framework.

You can quickly launch a multi-platform release server for distributing your
application by using one of these projects:

- [nuts][nuts]: *A smart release server for your applications, using GitHub as a backend. Auto-updates with Squirrel (Mac & Windows)*
- [electron-release-server][electron-release-server]: *A fully featured,
  self-hosted release server for electron applications, compatible with
  auto-updater*
- [squirrel-updates-server][squirrel-updates-server]: *A simple node.js server
  for Squirrel.Mac and Squirrel.Windows which uses GitHub releases*

## Platform notices

Though `autoUpdater` provides a uniform API for different platforms, there are
still some subtle differences on each platform.

### OS X

On OS X, the `autoUpdater` module is built upon [Squirrel.Mac][squirrel-mac],
meaning you don't need any special setup to make it work. For server-side
requirements, you can read [Server Support][server-support].

**Note:** Your application must be signed for automatic updates on Mac OS X.
This is a requirement of `Squirrel.Mac`.

### Windows

On Windows, you have to install your app into a user's machine before you can
use the `autoUpdater`, so it is recommended that you use the
[electron-winstaller][installer-lib] module or the [grunt-electron-installer][installer]
package to generate a Windows installer.

The installer generated with Squirrel will create a shortcut icon with an
[Application User Model ID][app-user-model-id] in the format of
`com.squirrel.PACKAGE_ID.YOUR_EXE_WITHOUT_DOT_EXE`, examples are
`com.squirrel.slack.Slack` and `com.squirrel.code.Code`. You have to use the
same ID for your app with `app.setAppUserModelId` API, otherwise Windows will
not be able to pin your app properly in task bar.

The server-side setup is also different from OS X. You can read the documents of
[Squirrel.Windows][squirrel-windows] to get more details.

### Linux

There is no built-in support for auto-updater on Linux, so it is recommended to
use the distribution's package manager to update your app.

## Events

The `autoUpdater` object emits the following events:

### Event: 'error'

Returns:

* `error` Error

Emitted when there is an error while updating.

### Event: 'checking-for-update'

Emitted when checking if an update has started.

### Event: 'update-available'

Emitted when there is an available update. The update is downloaded
automatically.

### Event: 'update-not-available'

Emitted when there is no available update.

### Event: 'update-downloaded'

Returns:

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateURL` String

Emitted when an update has been downloaded.

On Windows only `releaseName` is available.

## Methods

The `autoUpdater` object has the following methods:

### `autoUpdater.setFeedURL(url[, requestHeaders])`

* `url` String
* `requestHeaders` Object _OS X_ - HTTP request headers.

Sets the `url` and initialize the auto updater.

### `autoUpdater.checkForUpdates()`

Asks the server whether there is an update. You must call `setFeedURL` before
using this API.

### `autoUpdater.quitAndInstall()`

Restarts the app and installs the update after it has been downloaded. It
should only be called after `update-downloaded` has been emitted.

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[squirrel-windows]: https://github.com/Squirrel/Squirrel.Windows
[installer]: https://github.com/electron/grunt-electron-installer
[installer-lib]: https://github.com/electron/windows-installer
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[electron-release-server]: https://github.com/ArekSredzki/electron-release-server
[squirrel-updates-server]: https://github.com/Aluxian/squirrel-updates-server
[nuts]: https://github.com/GitbookIO/nuts
