# auto-updater

The `auto-updater` module is a simple wrap around the
[Squirrel](https://github.com/Squirrel/Squirrel.Mac) framework, you should
follow Squirrel's instructions on setting the server.

## Event: checking-for-update

Emitted when checking for update has started.

## Event: update-available

Emitted when there is an available update, the update would be downloaded
automatically.

## Event: update-not-available

Emitted when there is no available update.

## Event: update-downloaded

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateUrl` String
* `quitAndUpdate` Function

Emitted when update has been downloaded, calling `quitAndUpdate()` would restart
the application and install the update.

## autoUpdater.setFeedUrl(url)

* `url` String

Set the `url` and initialize the auto updater. The `url` could not be changed
once it is set.

## autoUpdater.checkForUpdates()

Ask the server whether there is an update, you have to call `setFeedUrl` before
using this API.
