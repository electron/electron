# autoUpdater

> Enable apps to automatically update themselves.

Process: [Main](../glossary.md#main-process)

**See also: [A detailed guide about how to implement updates in your application](../tutorial/updates.md).**

`autoUpdater` is an [EventEmitter][event-emitter].

## Platform Notices

Currently, only macOS and Windows are supported. There is no built-in support
for auto-updater on Linux, so it is recommended to use the
distribution's package manager to update your app.

In addition, there are some subtle differences on each platform:

### macOS

On macOS, the `autoUpdater` module is built upon [Squirrel.Mac][squirrel-mac],
meaning you don't need any special setup to make it work. For server-side
requirements, you can read [Server Support][server-support]. Note that
[App Transport Security](https://developer.apple.com/library/content/documentation/General/Reference/InfoPlistKeyReference/Articles/CocoaKeys.html#//apple_ref/doc/uid/TP40009251-SW35)
(ATS) applies to all requests made as part of the
update process. Apps that need to disable ATS can add the
`NSAllowsArbitraryLoads` key to their app's plist.

> [!IMPORTANT]
> Your application must be signed for automatic updates on macOS.
> This is a requirement of `Squirrel.Mac`.

### Windows

On Windows, the `autoUpdater` module automatically selects the appropriate update mechanism
based on how your app is packaged:

* **MSIX packages**: If your app is running as an MSIX package (created with [electron-windows-msix][msix-lib] and detected via [`process.windowsStore`](process.md#processwindowsstore-readonly)),
  the module uses the MSIX updater, which supports direct MSIX file links and JSON update feeds.
* **Squirrel.Windows**: For apps installed via traditional installers (created with
  [electron-winstaller][installer-lib] or [Electron Forge's Squirrel.Windows maker][electron-forge-lib]),
  the module uses Squirrel.Windows for updates.

You don't need to configure which updater to use; Electron automatically detects the packaging
format and uses the appropriate one.

#### Squirrel.Windows

Apps built with Squirrel.Windows will trigger [custom launch events](https://github.com/Squirrel/Squirrel.Windows/blob/51f5e2cb01add79280a53d51e8d0cfa20f8c9f9f/docs/using/custom-squirrel-events-non-cs.md#application-startup-commands)
that must be handled by your Electron application to ensure proper setup and teardown.

Squirrel.Windows apps will launch with the `--squirrel-firstrun` argument immediately
after installation. During this time, Squirrel.Windows will obtain a file lock on
your app, and `autoUpdater` requests will fail until the lock is released. In practice,
this means that you won't be able to check for updates on first launch for the first
few seconds. You can work around this by not checking for updates when `process.argv`
contains the `--squirrel-firstrun` flag or by setting a 10-second timeout on your
update checks (see [electron/electron#7155](https://github.com/electron/electron/issues/7155)
for more information).

The installer generated with Squirrel.Windows will create a shortcut icon with an
[Application User Model ID][app-user-model-id] in the format of
`com.squirrel.PACKAGE_ID.YOUR_EXE_WITHOUT_DOT_EXE`, examples are
`com.squirrel.slack.Slack` and `com.squirrel.code.Code`. You have to use the
same ID for your app with `app.setAppUserModelId` API, otherwise Windows will
not be able to pin your app properly in task bar.

#### MSIX Packages

When your app is packaged as an MSIX, the `autoUpdater` module provides additional
functionality:

* Use the `allowAnyVersion` option in `setFeedURL()` to allow updates to older versions (downgrades)
* Support for direct MSIX file links or JSON update feeds (similar to Squirrel.Mac format)

## Events

The `autoUpdater` object emits the following events:

### Event: 'error'

Returns:

* `error` Error

Emitted when there is an error while updating.

### Event: 'checking-for-update'

Emitted when checking for an available update has started.

### Event: 'update-available'

Emitted when there is an available update. The update is downloaded
automatically.

### Event: 'update-not-available'

Emitted when there is no available update.

### Event: 'update-downloaded'

Returns:

* `event` Event
* `releaseNotes` string
* `releaseName` string
* `releaseDate` Date
* `updateURL` string

Emitted when an update has been downloaded.

With Squirrel.Windows only `releaseName` is available.

> [!NOTE]
> It is not strictly necessary to handle this event. A successfully
> downloaded update will still be applied the next time the application starts.

### Event: 'before-quit-for-update'

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/12619
```
-->

This event is emitted after a user calls `quitAndInstall()`.

When this API is called, the `before-quit` event is not emitted before all windows are closed. As a result you should listen to this event if you wish to perform actions before the windows are closed while a process is quitting, as well as listening to `before-quit`.

## Methods

The `autoUpdater` object has the following methods:

### `autoUpdater.setFeedURL(options)`

<!--
```YAML history
changes:
  - pr-url: https://github.com/electron/electron/pull/5879
    description: "Added `headers` as a second parameter."
  - pr-url: https://github.com/electron/electron/pull/11925
    description: "Changed API to accept a single `options` argument (contains `url`, `headers`, and `serverType` properties)."
```
-->

* `options` Object
  * `url` string - The update server URL. For _Windows_ MSIX, this can be either a direct link to an MSIX file (e.g., `https://example.com/update.msix`) or a JSON endpoint that returns update information (see the [Squirrel.Mac][squirrel-mac] README for more information).
  * `headers` Record\<string, string\> (optional) _macOS_ - HTTP request headers.
  * `serverType` string (optional) _macOS_ - Can be `json` or `default`, see the [Squirrel.Mac][squirrel-mac]
    README for more information.
  * `allowAnyVersion` boolean (optional) _Windows_ - If `true`, allows downgrades to older versions for MSIX packages.
    Defaults to `false`.

Sets the `url` and initialize the auto updater.

### `autoUpdater.getFeedURL()`

Returns `string` - The current update feed URL.

### `autoUpdater.checkForUpdates()`

Asks the server whether there is an update. You must call `setFeedURL` before
using this API.

> [!NOTE]
> If an update is available it will be downloaded automatically.
> Calling `autoUpdater.checkForUpdates()` twice will download the update two times.

### `autoUpdater.quitAndInstall()`

Restarts the app and installs the update after it has been downloaded. It
should only be called after `update-downloaded` has been emitted.

Under the hood calling `autoUpdater.quitAndInstall()` will close all application
windows first, and automatically call `app.quit()` after all windows have been
closed.

> [!NOTE]
> It is not strictly necessary to call this function to apply an update,
> as a successfully downloaded update will always be applied the next time the
> application starts.

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[installer-lib]: https://github.com/electron/windows-installer
[electron-forge-lib]: https://www.electronforge.io/config/makers/squirrel.windows
[app-user-model-id]: https://learn.microsoft.com/en-us/windows/win32/shell/appids
[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
[msix-lib]:  https://github.com/electron-userland/electron-windows-msix
