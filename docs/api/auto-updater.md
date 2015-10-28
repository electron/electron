# autoUpdater

This module provides an interface for the `Squirrel` auto-updater framework.

## Platform notices

Though `autoUpdater` provides an uniform API for different platforms, there are
still some subtle differences on each platform.

### OS X

On OS X the `autoUpdater` module is built upon [Squirrel.Mac][squirrel-mac], you
don't need any special setup to make it work. For server-side requirements, you
can read [Server Support][server-support].

### Windows

On Windows you have to install your app into user's machine before you can use
the auto-updater, it is recommended to use [grunt-electron-installer][installer]
module to generate a Windows installer.

The server-side setup is also different from OS X, you can read the documents of
[Squirrel.Windows][squirrel-windows] to get more details.

### Linux

There is not built-in support for auto-updater on Linux, it is recommended to
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
* `updateUrl` String

Emitted when an update has been downloaded.

On Windows only `releaseName` is available.

## Methods

The `autoUpdater` object has the following methods:

### `autoUpdater.setFeedUrl(url)`

* `url` String

Sets the `url` and initialize the auto updater. The `url` cannot be changed
once it is set.

### `autoUpdater.checkForUpdates()`

Asks the server whether there is an update. You must call `setFeedUrl` before
using this API.

### `autoUpdater.quitAndUpdate()`

Restarts the app and install the update after it has been downloaded. It should
only be called after `update-downloaded` has been emitted.

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[squirrel-windows]: https://github.com/Squirrel/Squirrel.Windows
[installer]: https://github.com/atom/grunt-electron-installer
