# auto-updater

`auto-updater` module is a simple wrap around the Sparkle framework, it
provides auto update service for the application.

Before using this module, you should edit the `Info.plist` following
https://github.com/andymatuschak/Sparkle/wiki.

## Event: will-install-update

* `event` Event
* `version` String
* `continueUpdate` Function

This event is emitted when the update is found and going to be installed.
Calling `event.preventDefault()` would pause it, and you can call
`continueUpdate` to continue the update.

## Event: ready-for-update-on-quit

* `event` Event
* `version` String
* `quitAndUpdate` Function

This event is emitted when user chose to delay the update until the quit.
Calling `quitAndUpdate()` would quit the application and install the update.

## autoUpdater.setFeedUrl(url)

* `url` String

## autoUpdater.setAutomaticallyChecksForUpdates(flag)

* `flag` Boolean

## autoUpdater.setAutomaticallyDownloadsUpdates(flag)

* `flag` Boolean

## autoUpdater.checkForUpdates()

## autoUpdater.checkForUpdatesInBackground()
