# app

> Control your application's event lifecycle.

Process: [Main](../glossary.md#main-process)

The following example shows how to quit the application when the last window is
closed:

```javascript
const { app } = require('electron')
app.on('window-all-closed', () => {
  app.quit()
})
```

## Events

The `app` object emits the following events:

### Event: 'will-finish-launching'

Emitted when the application has finished basic startup. On Windows and Linux,
the `will-finish-launching` event is the same as the `ready` event; on macOS,
this event represents the `applicationWillFinishLaunching` notification of
`NSApplication`. You would usually set up listeners for the `open-file` and
`open-url` events here, and start the crash reporter and auto updater.

In most cases, you should do everything in the `ready` event handler.

### Event: 'ready'

Returns:

* `launchInfo` unknown _macOS_

Emitted once, when Electron has finished initializing. On macOS, `launchInfo`
holds the `userInfo` of the `NSUserNotification` that was used to open the
application, if it was launched from Notification Center. You can also call
`app.isReady()` to check if this event has already fired and `app.whenReady()`
to get a Promise that is fulfilled when Electron is initialized.

### Event: 'window-all-closed'

Emitted when all windows have been closed.

If you do not subscribe to this event and all windows are closed, the default
behavior is to quit the app; however, if you subscribe, you control whether the
app quits or not. If the user pressed `Cmd + Q`, or the developer called
`app.quit()`, Electron will first try to close all the windows and then emit the
`will-quit` event, and in this case the `window-all-closed` event would not be
emitted.

### Event: 'before-quit'

Returns:

* `event` Event

Emitted before the application starts closing its windows.
Calling `event.preventDefault()` will prevent the default behavior, which is
terminating the application.

**Note:** If application quit was initiated by `autoUpdater.quitAndInstall()`,
then `before-quit` is emitted *after* emitting `close` event on all windows and
closing them.

**Note:** On Windows, this event will not be emitted if the app is closed due
to a shutdown/restart of the system or a user logout.

### Event: 'will-quit'

Returns:

* `event` Event

Emitted when all windows have been closed and the application will quit.
Calling `event.preventDefault()` will prevent the default behavior, which is
terminating the application.

See the description of the `window-all-closed` event for the differences between
the `will-quit` and `window-all-closed` events.

**Note:** On Windows, this event will not be emitted if the app is closed due
to a shutdown/restart of the system or a user logout.

### Event: 'quit'

Returns:

* `event` Event
* `exitCode` Integer

Emitted when the application is quitting.

**Note:** On Windows, this event will not be emitted if the app is closed due
to a shutdown/restart of the system or a user logout.

### Event: 'open-file' _macOS_

Returns:

* `event` Event
* `path` String

Emitted when the user wants to open a file with the application. The `open-file`
event is usually emitted when the application is already open and the OS wants
to reuse the application to open the file. `open-file` is also emitted when a
file is dropped onto the dock and the application is not yet running. Make sure
to listen for the `open-file` event very early in your application startup to
handle this case (even before the `ready` event is emitted).

You should call `event.preventDefault()` if you want to handle this event.

On Windows, you have to parse `process.argv` (in the main process) to get the
filepath.

### Event: 'open-url' _macOS_

Returns:

* `event` Event
* `url` String

Emitted when the user wants to open a URL with the application. Your application's
`Info.plist` file must define the URL scheme within the `CFBundleURLTypes` key, and
set `NSPrincipalClass` to `AtomApplication`.

You should call `event.preventDefault()` if you want to handle this event.

### Event: 'activate' _macOS_

Returns:

* `event` Event
* `hasVisibleWindows` Boolean

Emitted when the application is activated. Various actions can trigger
this event, such as launching the application for the first time, attempting
to re-launch the application when it's already running, or clicking on the
application's dock or taskbar icon.

### Event: 'continue-activity' _macOS_

Returns:

* `event` Event
* `type` String - A string identifying the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` unknown - Contains app-specific state stored by the activity on
  another device.

Emitted during [Handoff][handoff] when an activity from a different device wants
to be resumed. You should call `event.preventDefault()` if you want to handle
this event.

A user activity can be continued only in an app that has the same developer Team
ID as the activity's source app and that supports the activity's type.
Supported activity types are specified in the app's `Info.plist` under the
`NSUserActivityTypes` key.

### Event: 'will-continue-activity' _macOS_

Returns:

* `event` Event
* `type` String - A string identifying the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].

Emitted during [Handoff][handoff] before an activity from a different device wants
to be resumed. You should call `event.preventDefault()` if you want to handle
this event.

### Event: 'continue-activity-error' _macOS_

Returns:

* `event` Event
* `type` String - A string identifying the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `error` String - A string with the error's localized description.

Emitted during [Handoff][handoff] when an activity from a different device
fails to be resumed.

### Event: 'activity-was-continued' _macOS_

Returns:

* `event` Event
* `type` String - A string identifying the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` unknown - Contains app-specific state stored by the activity.

Emitted during [Handoff][handoff] after an activity from this device was successfully
resumed on another one.

### Event: 'update-activity-state' _macOS_

Returns:

* `event` Event
* `type` String - A string identifying the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` unknown - Contains app-specific state stored by the activity.

Emitted when [Handoff][handoff] is about to be resumed on another device. If you need to update the state to be transferred, you should call `event.preventDefault()` immediately, construct a new `userInfo` dictionary and call `app.updateCurrentActivity()` in a timely manner. Otherwise, the operation will fail and `continue-activity-error` will be called.

### Event: 'new-window-for-tab' _macOS_

Returns:

* `event` Event

Emitted when the user clicks the native macOS new tab button. The new
tab button is only visible if the current `BrowserWindow` has a
`tabbingIdentifier`

### Event: 'browser-window-blur'

Returns:

* `event` Event
* `window` [BrowserWindow](browser-window.md)

Emitted when a [browserWindow](browser-window.md) gets blurred.

### Event: 'browser-window-focus'

Returns:

* `event` Event
* `window` [BrowserWindow](browser-window.md)

Emitted when a [browserWindow](browser-window.md) gets focused.

### Event: 'browser-window-created'

Returns:

* `event` Event
* `window` [BrowserWindow](browser-window.md)

Emitted when a new [browserWindow](browser-window.md) is created.

### Event: 'web-contents-created'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)

Emitted when a new [webContents](web-contents.md) is created.

### Event: 'certificate-error'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` String
* `error` String - The error code
* `certificate` [Certificate](structures/certificate.md)
* `callback` Function
  * `isTrusted` Boolean - Whether to consider the certificate as trusted

Emitted when failed to verify the `certificate` for `url`, to trust the
certificate you should prevent the default behavior with
`event.preventDefault()` and call `callback(true)`.

```javascript
const { app } = require('electron')

app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  if (url === 'https://github.com') {
    // Verification logic.
    event.preventDefault()
    callback(true)
  } else {
    callback(false)
  }
})
```

### Event: 'select-client-certificate'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `certificateList` [Certificate[]](structures/certificate.md)
* `callback` Function
  * `certificate` [Certificate](structures/certificate.md) (optional)

Emitted when a client certificate is requested.

The `url` corresponds to the navigation entry requesting the client certificate
and `callback` can be called with an entry filtered from the list. Using
`event.preventDefault()` prevents the application from using the first
certificate from the store.

```javascript
const { app } = require('electron')

app.on('select-client-certificate', (event, webContents, url, list, callback) => {
  event.preventDefault()
  callback(list[0])
})
```

### Event: 'login'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `authenticationResponseDetails` Object
  * `url` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function
  * `username` String (optional)
  * `password` String (optional)

Emitted when `webContents` wants to do basic auth.

The default behavior is to cancel all authentications. To override this you
should prevent the default behavior with `event.preventDefault()` and call
`callback(username, password)` with the credentials.

```javascript
const { app } = require('electron')

app.on('login', (event, webContents, details, authInfo, callback) => {
  event.preventDefault()
  callback('username', 'secret')
})
```

If `callback` is called without a username or password, the authentication
request will be cancelled and the authentication error will be returned to the
page.

### Event: 'gpu-info-update'

Emitted whenever there is a GPU info update.

### Event: 'gpu-process-crashed'

Returns:

* `event` Event
* `killed` Boolean

Emitted when the GPU process crashes or is killed.

### Event: 'renderer-process-crashed'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `killed` Boolean

Emitted when the renderer process of `webContents` crashes or is killed.

**Deprecated:** This event is superceded by the `render-process-gone` event
which contains more information about why the render process dissapeared. It
isn't always because it crashed.  The `killed` boolean can be replaced by
checking `reason === 'killed'` when you switch to that event.

#### Event: 'render-process-gone'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `details` Object
  * `reason` String - The reason the render process is gone.  Possible values:
    * `clean-exit` - Process exited with an exit code of zero
    * `abnormal-exit` - Process exited with a non-zero exit code
    * `killed` - Process was sent a SIGTERM or otherwise killed externally
    * `crashed` - Process crashed
    * `oom` - Process ran out of memory
    * `launch-failure` - Process never successfully launched
    * `integrity-failure` - Windows code integrity checks failed

Emitted when the renderer process unexpectedly dissapears.  This is normally
because it was crashed or killed.

### Event: 'accessibility-support-changed' _macOS_ _Windows_

Returns:

* `event` Event
* `accessibilitySupportEnabled` Boolean - `true` when Chrome's accessibility
  support is enabled, `false` otherwise.

Emitted when Chrome's accessibility support changes. This event fires when
assistive technologies, such as screen readers, are enabled or disabled.
See https://www.chromium.org/developers/design-documents/accessibility for more
details.

### Event: 'session-created'

Returns:

* `session` [Session](session.md)

Emitted when Electron has created a new `session`.

```javascript
const { app } = require('electron')

app.on('session-created', (session) => {
  console.log(session)
})
```

### Event: 'second-instance'

Returns:

* `event` Event
* `argv` String[] - An array of the second instance's command line arguments
* `workingDirectory` String - The second instance's working directory

This event will be emitted inside the primary instance of your application
when a second instance has been executed and calls `app.requestSingleInstanceLock()`.

`argv` is an Array of the second instance's command line arguments,
and `workingDirectory` is its current working directory. Usually
applications respond to this by making their primary window focused and
non-minimized.

**Note:** If the second instance is started by a different user than the first, the `argv` array will not include the arguments.

This event is guaranteed to be emitted after the `ready` event of `app`
gets emitted.

**Note:** Extra command line arguments might be added by Chromium,
such as `--original-process-start-time`.

### Event: 'desktop-capturer-get-sources'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)

Emitted when `desktopCapturer.getSources()` is called in the renderer process of `webContents`.
Calling `event.preventDefault()` will make it return empty sources.

### Event: 'remote-require'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `moduleName` String

Emitted when `remote.require()` is called in the renderer process of `webContents`.
Calling `event.preventDefault()` will prevent the module from being returned.
Custom value can be returned by setting `event.returnValue`.

### Event: 'remote-get-global'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `globalName` String

Emitted when `remote.getGlobal()` is called in the renderer process of `webContents`.
Calling `event.preventDefault()` will prevent the global from being returned.
Custom value can be returned by setting `event.returnValue`.

### Event: 'remote-get-builtin'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `moduleName` String

Emitted when `remote.getBuiltin()` is called in the renderer process of `webContents`.
Calling `event.preventDefault()` will prevent the module from being returned.
Custom value can be returned by setting `event.returnValue`.

### Event: 'remote-get-current-window'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)

Emitted when `remote.getCurrentWindow()` is called in the renderer process of `webContents`.
Calling `event.preventDefault()` will prevent the object from being returned.
Custom value can be returned by setting `event.returnValue`.

### Event: 'remote-get-current-web-contents'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)

Emitted when `remote.getCurrentWebContents()` is called in the renderer process of `webContents`.
Calling `event.preventDefault()` will prevent the object from being returned.
Custom value can be returned by setting `event.returnValue`.

## Methods

The `app` object has the following methods:

**Note:** Some methods are only available on specific operating systems and are
labeled as such.

### `app.quit()`

Try to close all windows. The `before-quit` event will be emitted first. If all
windows are successfully closed, the `will-quit` event will be emitted and by
default the application will terminate.

This method guarantees that all `beforeunload` and `unload` event handlers are
correctly executed. It is possible that a window cancels the quitting by
returning `false` in the `beforeunload` event handler.

### `app.exit([exitCode])`

* `exitCode` Integer (optional)

Exits immediately with `exitCode`. `exitCode` defaults to 0.

All windows will be closed immediately without asking the user, and the `before-quit`
and `will-quit` events will not be emitted.

### `app.relaunch([options])`

* `options` Object (optional)
  * `args` String[] (optional)
  * `execPath` String (optional)

Relaunches the app when current instance exits.

By default, the new instance will use the same working directory and command line
arguments with current instance. When `args` is specified, the `args` will be
passed as command line arguments instead. When `execPath` is specified, the
`execPath` will be executed for relaunch instead of current app.

Note that this method does not quit the app when executed, you have to call
`app.quit` or `app.exit` after calling `app.relaunch` to make the app restart.

When `app.relaunch` is called for multiple times, multiple instances will be
started after current instance exited.

An example of restarting current instance immediately and adding a new command
line argument to the new instance:

```javascript
const { app } = require('electron')

app.relaunch({ args: process.argv.slice(1).concat(['--relaunch']) })
app.exit(0)
```

### `app.isReady()`

Returns `Boolean` - `true` if Electron has finished initializing, `false` otherwise.
See also `app.whenReady()`.

### `app.whenReady()`

Returns `Promise<void>` - fulfilled when Electron is initialized.
May be used as a convenient alternative to checking `app.isReady()`
and subscribing to the `ready` event if the app is not ready yet.

### `app.focus([options])`

* `options` Object (optional)
  * `steal` Boolean _macOS_ - Make the receiver the active app even if another app is
  currently active.

On Linux, focuses on the first visible window. On macOS, makes the application
the active app. On Windows, focuses on the application's first window.

You should seek to use the `steal` option as sparingly as possible.

### `app.hide()` _macOS_

Hides all application windows without minimizing them.

### `app.show()` _macOS_

Shows application windows after they were hidden. Does not automatically focus
them.

### `app.setAppLogsPath([path])`

* `path` String (optional) - A custom path for your logs. Must be absolute.

Sets or creates a directory your app's logs which can then be manipulated with `app.getPath()` or `app.setPath(pathName, newPath)`.

Calling `app.setAppLogsPath()` without a `path` parameter will result in this directory being set to `~/Library/Logs/YourAppName` on _macOS_, and inside the `userData` directory on _Linux_ and _Windows_.

### `app.getAppPath()`

Returns `String` - The current application directory.

### `app.getPath(name)`

* `name` String - You can request the following paths by the name:
  * `home` User's home directory.
  * `appData` Per-user application data directory, which by default points to:
    * `%APPDATA%` on Windows
    * `$XDG_CONFIG_HOME` or `~/.config` on Linux
    * `~/Library/Application Support` on macOS
  * `userData` The directory for storing your app's configuration files, which by
    default it is the `appData` directory appended with your app's name.
  * `cache`
  * `temp` Temporary directory.
  * `exe` The current executable file.
  * `module` The `libchromiumcontent` library.
  * `desktop` The current user's Desktop directory.
  * `documents` Directory for a user's "My Documents".
  * `downloads` Directory for a user's downloads.
  * `music` Directory for a user's music.
  * `pictures` Directory for a user's pictures.
  * `videos` Directory for a user's videos.
  * `recent` Directory for the user's recent files (Windows only).
  * `logs` Directory for your app's log folder.
  * `pepperFlashSystemPlugin` Full path to the system version of the Pepper Flash plugin.
  * `crashDumps` Directory where crash dumps are stored.

Returns `String` - A path to a special directory or file associated with `name`. On
failure, an `Error` is thrown.

If `app.getPath('logs')` is called without called `app.setAppLogsPath()` being called first, a default log directory will be created equivalent to calling `app.setAppLogsPath()` without a `path` parameter.

### `app.getFileIcon(path[, options])`

* `path` String
* `options` Object (optional)
  * `size` String
    * `small` - 16x16
    * `normal` - 32x32
    * `large` - 48x48 on _Linux_, 32x32 on _Windows_, unsupported on _macOS_.

Returns `Promise<NativeImage>` - fulfilled with the app's icon, which is a [NativeImage](native-image.md).

Fetches a path's associated icon.

On _Windows_, there a 2 kinds of icons:

* Icons associated with certain file extensions, like `.mp3`, `.png`, etc.
* Icons inside the file itself, like `.exe`, `.dll`, `.ico`.

On _Linux_ and _macOS_, icons depend on the application associated with file mime type.

### `app.setPath(name, path)`

* `name` String
* `path` String

Overrides the `path` to a special directory or file associated with `name`.
If the path specifies a directory that does not exist, an `Error` is thrown.
In that case, the directory should be created with `fs.mkdirSync` or similar.

You can only override paths of a `name` defined in `app.getPath`.

By default, web pages' cookies and caches will be stored under the `userData`
directory. If you want to change this location, you have to override the
`userData` path before the `ready` event of the `app` module is emitted.

### `app.getVersion()`

Returns `String` - The version of the loaded application. If no version is found in the
application's `package.json` file, the version of the current bundle or
executable is returned.

### `app.getName()`

Returns `String` - The current application's name, which is the name in the application's
`package.json` file.

Usually the `name` field of `package.json` is a short lowercase name, according
to the npm modules spec. You should usually also specify a `productName`
field, which is your application's full capitalized name, and which will be
preferred over `name` by Electron.

### `app.setName(name)`

* `name` String

Overrides the current application's name.

**Note:** This function overrides the name used internally by Electron; it does not affect the name that the OS uses.

### `app.getLocale()`

Returns `String` - The current application locale. Possible return values are documented [here](locales.md).

To set the locale, you'll want to use a command line switch at app startup, which may be found [here](https://github.com/electron/electron/blob/master/docs/api/command-line-switches.md).

**Note:** When distributing your packaged app, you have to also ship the
`locales` folder.

**Note:** On Windows, you have to call it after the `ready` events gets emitted.

### `app.getLocaleCountryCode()`

Returns `String` - User operating system's locale two-letter [ISO 3166](https://www.iso.org/iso-3166-country-codes.html) country code. The value is taken from native OS APIs.

**Note:** When unable to detect locale country code, it returns empty string.

### `app.addRecentDocument(path)` _macOS_ _Windows_

* `path` String

Adds `path` to the recent documents list.

This list is managed by the OS. On Windows, you can visit the list from the task
bar, and on macOS, you can visit it from dock menu.

### `app.clearRecentDocuments()` _macOS_ _Windows_

Clears the recent documents list.

### `app.setAsDefaultProtocolClient(protocol[, path, args])`

* `protocol` String - The name of your protocol, without `://`. For example,
  if you want your app to handle `electron://` links, call this method with
  `electron` as the parameter.
* `path` String (optional) _Windows_ - The path to the Electron executable.
  Defaults to `process.execPath`
* `args` String[] (optional) _Windows_ - Arguments passed to the executable.
  Defaults to an empty array

Returns `Boolean` - Whether the call succeeded.

Sets the current executable as the default handler for a protocol (aka URI
scheme). It allows you to integrate your app deeper into the operating system.
Once registered, all links with `your-protocol://` will be opened with the
current executable. The whole link, including protocol, will be passed to your
application as a parameter.

**Note:** On macOS, you can only register protocols that have been added to
your app's `info.plist`, which cannot be modified at runtime. However, you can
change the file during build time via [Electron Forge][electron-forge],
[Electron Packager][electron-packager], or by editing `info.plist` with a text
editor. Please refer to [Apple's documentation][CFBundleURLTypes] for details.

**Note:** In a Windows Store environment (when packaged as an `appx`) this API
will return `true` for all calls but the registry key it sets won't be accessible
by other applications.  In order to register your Windows Store application
as a default protocol handler you must [declare the protocol in your manifest](https://docs.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/element-uap-protocol).

The API uses the Windows Registry and `LSSetDefaultHandlerForURLScheme` internally.

### `app.removeAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - The name of your protocol, without `://`.
* `path` String (optional) _Windows_ - Defaults to `process.execPath`
* `args` String[] (optional) _Windows_ - Defaults to an empty array

Returns `Boolean` - Whether the call succeeded.

This method checks if the current executable as the default handler for a
protocol (aka URI scheme). If so, it will remove the app as the default handler.

### `app.isDefaultProtocolClient(protocol[, path, args])`

* `protocol` String - The name of your protocol, without `://`.
* `path` String (optional) _Windows_ - Defaults to `process.execPath`
* `args` String[] (optional) _Windows_ - Defaults to an empty array

Returns `Boolean` - Whether the current executable is the default handler for a
protocol (aka URI scheme).

**Note:** On macOS, you can use this method to check if the app has been
registered as the default protocol handler for a protocol. You can also verify
this by checking `~/Library/Preferences/com.apple.LaunchServices.plist` on the
macOS machine. Please refer to
[Apple's documentation][LSCopyDefaultHandlerForURLScheme] for details.

The API uses the Windows Registry and `LSCopyDefaultHandlerForURLScheme` internally.

### `app.getApplicationNameForProtocol(url)`

* `url` String - a URL with the protocol name to check. Unlike the other
  methods in this family, this accepts an entire URL, including `://` at a
  minimum (e.g. `https://`).

Returns `String` - Name of the application handling the protocol, or an empty
  string if there is no handler. For instance, if Electron is the default
  handler of the URL, this could be `Electron` on Windows and Mac. However,
  don't rely on the precise format which is not guaranteed to remain unchanged.
  Expect a different format on Linux, possibly with a `.desktop` suffix.

This method returns the application name of the default handler for the protocol
(aka URI scheme) of a URL.

### `app.setUserTasks(tasks)` _Windows_

* `tasks` [Task[]](structures/task.md) - Array of `Task` objects

Adds `tasks` to the [Tasks][tasks] category of the Jump List on Windows.

`tasks` is an array of [`Task`](structures/task.md) objects.

Returns `Boolean` - Whether the call succeeded.

**Note:** If you'd like to customize the Jump List even more use
`app.setJumpList(categories)` instead.

### `app.getJumpListSettings()` _Windows_

Returns `Object`:

* `minItems` Integer - The minimum number of items that will be shown in the
  Jump List (for a more detailed description of this value see the
  [MSDN docs][JumpListBeginListMSDN]).
* `removedItems` [JumpListItem[]](structures/jump-list-item.md) - Array of `JumpListItem`
  objects that correspond to items that the user has explicitly removed from custom categories in the
  Jump List. These items must not be re-added to the Jump List in the **next**
  call to `app.setJumpList()`, Windows will not display any custom category
  that contains any of the removed items.

### `app.setJumpList(categories)` _Windows_

* `categories` [JumpListCategory[]](structures/jump-list-category.md) | `null` - Array of `JumpListCategory` objects.

Sets or removes a custom Jump List for the application, and returns one of the
following strings:

* `ok` - Nothing went wrong.
* `error` - One or more errors occurred, enable runtime logging to figure out
  the likely cause.
* `invalidSeparatorError` - An attempt was made to add a separator to a
  custom category in the Jump List. Separators are only allowed in the
  standard `Tasks` category.
* `fileTypeRegistrationError` - An attempt was made to add a file link to
  the Jump List for a file type the app isn't registered to handle.
* `customCategoryAccessDeniedError` - Custom categories can't be added to the
  Jump List due to user privacy or group policy settings.

If `categories` is `null` the previously set custom Jump List (if any) will be
replaced by the standard Jump List for the app (managed by Windows).

**Note:** If a `JumpListCategory` object has neither the `type` nor the `name`
property set then its `type` is assumed to be `tasks`. If the `name` property
is set but the `type` property is omitted then the `type` is assumed to be
`custom`.

**Note:** Users can remove items from custom categories, and Windows will not
allow a removed item to be added back into a custom category until **after**
the next successful call to `app.setJumpList(categories)`. Any attempt to
re-add a removed item to a custom category earlier than that will result in the
entire custom category being omitted from the Jump List. The list of removed
items can be obtained using `app.getJumpListSettings()`.

Here's a very simple example of creating a custom Jump List:

```javascript
const { app } = require('electron')

app.setJumpList([
  {
    type: 'custom',
    name: 'Recent Projects',
    items: [
      { type: 'file', path: 'C:\\Projects\\project1.proj' },
      { type: 'file', path: 'C:\\Projects\\project2.proj' }
    ]
  },
  { // has a name so `type` is assumed to be "custom"
    name: 'Tools',
    items: [
      {
        type: 'task',
        title: 'Tool A',
        program: process.execPath,
        args: '--run-tool-a',
        icon: process.execPath,
        iconIndex: 0,
        description: 'Runs Tool A'
      },
      {
        type: 'task',
        title: 'Tool B',
        program: process.execPath,
        args: '--run-tool-b',
        icon: process.execPath,
        iconIndex: 0,
        description: 'Runs Tool B'
      }
    ]
  },
  { type: 'frequent' },
  { // has no name and no type so `type` is assumed to be "tasks"
    items: [
      {
        type: 'task',
        title: 'New Project',
        program: process.execPath,
        args: '--new-project',
        description: 'Create a new project.'
      },
      { type: 'separator' },
      {
        type: 'task',
        title: 'Recover Project',
        program: process.execPath,
        args: '--recover-project',
        description: 'Recover Project'
      }
    ]
  }
])
```

### `app.requestSingleInstanceLock()`

Returns `Boolean`

The return value of this method indicates whether or not this instance of your
application successfully obtained the lock.  If it failed to obtain the lock,
you can assume that another instance of your application is already running with
the lock and exit immediately.

I.e. This method returns `true` if your process is the primary instance of your
application and your app should continue loading.  It returns `false` if your
process should immediately quit as it has sent its parameters to another
instance that has already acquired the lock.

On macOS, the system enforces single instance automatically when users try to open
a second instance of your app in Finder, and the `open-file` and `open-url`
events will be emitted for that. However when users start your app in command
line, the system's single instance mechanism will be bypassed, and you have to
use this method to ensure single instance.

An example of activating the window of primary instance when a second instance
starts:

```javascript
const { app } = require('electron')
let myWindow = null

const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
  app.quit()
} else {
  app.on('second-instance', (event, commandLine, workingDirectory) => {
    // Someone tried to run a second instance, we should focus our window.
    if (myWindow) {
      if (myWindow.isMinimized()) myWindow.restore()
      myWindow.focus()
    }
  })

  // Create myWindow, load the rest of the app, etc...
  app.whenReady().then(() => {
  })
}
```

### `app.hasSingleInstanceLock()`

Returns `Boolean`

This method returns whether or not this instance of your app is currently
holding the single instance lock.  You can request the lock with
`app.requestSingleInstanceLock()` and release with
`app.releaseSingleInstanceLock()`

### `app.releaseSingleInstanceLock()`

Releases all locks that were created by `requestSingleInstanceLock`. This will
allow multiple instances of the application to once again run side by side.

### `app.setUserActivity(type, userInfo[, webpageURL])` _macOS_

* `type` String - Uniquely identifies the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` any - App-specific state to store for use by another device.
* `webpageURL` String (optional) - The webpage to load in a browser if no suitable app is
  installed on the resuming device. The scheme must be `http` or `https`.

Creates an `NSUserActivity` and sets it as the current activity. The activity
is eligible for [Handoff][handoff] to another device afterward.

### `app.getCurrentActivityType()` _macOS_

Returns `String` - The type of the currently running activity.

### `app.invalidateCurrentActivity()` _macOS_

Invalidates the current [Handoff][handoff] user activity.

### `app.resignCurrentActivity()` _macOS_

Marks the current [Handoff][handoff] user activity as inactive without invalidating it.

### `app.updateCurrentActivity(type, userInfo)` _macOS_

* `type` String - Uniquely identifies the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` any - App-specific state to store for use by another device.

Updates the current activity if its type matches `type`, merging the entries from
`userInfo` into its current `userInfo` dictionary.

### `app.setAppUserModelId(id)` _Windows_

* `id` String

Changes the [Application User Model ID][app-user-model-id] to `id`.

### `app.setActivationPolicy(policy)` _macOS_

* `policy` String - Can be 'regular', 'accessory', or 'prohibited'.

Sets the activation policy for a given app.

Activation policy types:
* 'regular' - The application is an ordinary app that appears in the Dock and may have a user interface.
* 'accessory' - The application doesn’t appear in the Dock and doesn’t have a menu bar, but it may be activated programmatically or by clicking on one of its windows.
* 'prohibited' - The application doesn’t appear in the Dock and may not create windows or be activated.

### `app.importCertificate(options, callback)` _Linux_

* `options` Object
  * `certificate` String - Path for the pkcs12 file.
  * `password` String - Passphrase for the certificate.
* `callback` Function
  * `result` Integer - Result of import.

Imports the certificate in pkcs12 format into the platform certificate store.
`callback` is called with the `result` of import operation, a value of `0`
indicates success while any other value indicates failure according to Chromium [net_error_list](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).

### `app.disableHardwareAcceleration()`

Disables hardware acceleration for current app.

This method can only be called before app is ready.

### `app.disableDomainBlockingFor3DAPIs()`

By default, Chromium disables 3D APIs (e.g. WebGL) until restart on a per
domain basis if the GPU processes crashes too frequently. This function
disables that behavior.

This method can only be called before app is ready.

### `app.getAppMetrics()`

Returns [`ProcessMetric[]`](structures/process-metric.md): Array of `ProcessMetric` objects that correspond to memory and CPU usage statistics of all the processes associated with the app.

### `app.getGPUFeatureStatus()`

Returns [`GPUFeatureStatus`](structures/gpu-feature-status.md) - The Graphics Feature Status from `chrome://gpu/`.

**Note:** This information is only usable after the `gpu-info-update` event is emitted.

### `app.getGPUInfo(infoType)`

* `infoType` String - Can be `basic` or `complete`.

Returns `Promise<unknown>`

For `infoType` equal to `complete`:
 Promise is fulfilled with `Object` containing all the GPU Information as in [chromium's GPUInfo object](https://chromium.googlesource.com/chromium/src/+/4178e190e9da409b055e5dff469911ec6f6b716f/gpu/config/gpu_info.cc). This includes the version and driver information that's shown on `chrome://gpu` page.

For `infoType` equal to `basic`:
  Promise is fulfilled with `Object` containing fewer attributes than when requested with `complete`. Here's an example of basic response:
```js
{ auxAttributes:
   { amdSwitchable: true,
     canSupportThreadedTextureMailbox: false,
     directComposition: false,
     directRendering: true,
     glResetNotificationStrategy: 0,
     inProcessGpu: true,
     initializationTime: 0,
     jpegDecodeAcceleratorSupported: false,
     optimus: false,
     passthroughCmdDecoder: false,
     sandboxed: false,
     softwareRendering: false,
     supportsOverlays: false,
     videoDecodeAcceleratorFlags: 0 },
gpuDevice:
   [ { active: true, deviceId: 26657, vendorId: 4098 },
     { active: false, deviceId: 3366, vendorId: 32902 } ],
machineModelName: 'MacBookPro',
machineModelVersion: '11.5' }
```

Using `basic` should be preferred if only basic information like `vendorId` or `driverId` is needed.

### `app.setBadgeCount(count)` _Linux_ _macOS_

* `count` Integer

Returns `Boolean` - Whether the call succeeded.

Sets the counter badge for current app. Setting the count to `0` will hide the
badge.

On macOS, it shows on the dock icon. On Linux, it only works for Unity launcher.

**Note:** Unity launcher requires the existence of a `.desktop` file to work,
for more information please read [Desktop Environment Integration][unity-requirement].

### `app.getBadgeCount()` _Linux_ _macOS_

Returns `Integer` - The current value displayed in the counter badge.

### `app.isUnityRunning()` _Linux_

Returns `Boolean` - Whether the current desktop environment is Unity launcher.

### `app.getLoginItemSettings([options])` _macOS_ _Windows_

* `options` Object (optional)
  * `path` String (optional) _Windows_ - The executable path to compare against.
    Defaults to `process.execPath`.
  * `args` String[] (optional) _Windows_ - The command-line arguments to compare
    against. Defaults to an empty array.

If you provided `path` and `args` options to `app.setLoginItemSettings`, then you
need to pass the same arguments here for `openAtLogin` to be set correctly.

Returns `Object`:

* `openAtLogin` Boolean - `true` if the app is set to open at login.
* `openAsHidden` Boolean _macOS_ - `true` if the app is set to open as hidden at login.
  This setting is not available on [MAS builds][mas-builds].
* `wasOpenedAtLogin` Boolean _macOS_ - `true` if the app was opened at login
  automatically. This setting is not available on [MAS builds][mas-builds].
* `wasOpenedAsHidden` Boolean _macOS_ - `true` if the app was opened as a hidden login
  item. This indicates that the app should not open any windows at startup.
  This setting is not available on [MAS builds][mas-builds].
* `restoreState` Boolean _macOS_ - `true` if the app was opened as a login item that
  should restore the state from the previous session. This indicates that the
  app should restore the windows that were open the last time the app was
  closed. This setting is not available on [MAS builds][mas-builds].

### `app.setLoginItemSettings(settings)` _macOS_ _Windows_

* `settings` Object
  * `openAtLogin` Boolean (optional) - `true` to open the app at login, `false` to remove
    the app as a login item. Defaults to `false`.
  * `openAsHidden` Boolean (optional) _macOS_ - `true` to open the app as hidden. Defaults to
    `false`. The user can edit this setting from the System Preferences so
    `app.getLoginItemSettings().wasOpenedAsHidden` should be checked when the app
    is opened to know the current value. This setting is not available on [MAS builds][mas-builds].
  * `path` String (optional) _Windows_ - The executable to launch at login.
    Defaults to `process.execPath`.
  * `args` String[] (optional) _Windows_ - The command-line arguments to pass to
    the executable. Defaults to an empty array. Take care to wrap paths in
    quotes.

Set the app's login item settings.

To work with Electron's `autoUpdater` on Windows, which uses [Squirrel][Squirrel-Windows],
you'll want to set the launch path to Update.exe, and pass arguments that specify your
application name. For example:

``` javascript
const appFolder = path.dirname(process.execPath)
const updateExe = path.resolve(appFolder, '..', 'Update.exe')
const exeName = path.basename(process.execPath)

app.setLoginItemSettings({
  openAtLogin: true,
  path: updateExe,
  args: [
    '--processStart', `"${exeName}"`,
    '--process-start-args', `"--hidden"`
  ]
})
```

### `app.isAccessibilitySupportEnabled()` _macOS_ _Windows_

Returns `Boolean` - `true` if Chrome's accessibility support is enabled,
`false` otherwise. This API will return `true` if the use of assistive
technologies, such as screen readers, has been detected. See
https://www.chromium.org/developers/design-documents/accessibility for more
details.

### `app.setAccessibilitySupportEnabled(enabled)` _macOS_ _Windows_

* `enabled` Boolean - Enable or disable [accessibility tree](https://developers.google.com/web/fundamentals/accessibility/semantics-builtin/the-accessibility-tree) rendering

Manually enables Chrome's accessibility support, allowing to expose accessibility switch to users in application settings. See [Chromium's accessibility docs](https://www.chromium.org/developers/design-documents/accessibility) for more
details. Disabled by default.

This API must be called after the `ready` event is emitted.

**Note:** Rendering accessibility tree can significantly affect the performance of your app. It should not be enabled by default.

### `app.showAboutPanel()`

Show the app's about panel options. These options can be overridden with `app.setAboutPanelOptions(options)`.

### `app.setAboutPanelOptions(options)`

* `options` Object
  * `applicationName` String (optional) - The app's name.
  * `applicationVersion` String (optional) - The app's version.
  * `copyright` String (optional) - Copyright information.
  * `version` String (optional) _macOS_ - The app's build version number.
  * `credits` String (optional) _macOS_ _Windows_ - Credit information.
  * `authors` String[] (optional) _Linux_ - List of app authors.
  * `website` String (optional) _Linux_ - The app's website.
  * `iconPath` String (optional) _Linux_ _Windows_ - Path to the app's icon. On Linux, will be shown as 64x64 pixels while retaining aspect ratio.

Set the about panel options. This will override the values defined in the app's `.plist` file on macOS. See the [Apple docs][about-panel-options] for more details. On Linux, values must be set in order to be shown; there are no defaults.

If you do not set `credits` but still wish to surface them in your app, AppKit will look for a file named "Credits.html", "Credits.rtf", and "Credits.rtfd", in that order, in the bundle returned by the NSBundle class method main. The first file found is used, and if none is found, the info area is left blank. See Apple [documentation](https://developer.apple.com/documentation/appkit/nsaboutpaneloptioncredits?language=objc) for more information.

### `app.isEmojiPanelSupported()`

Returns `Boolean` - whether or not the current OS version allows for native emoji pickers.

### `app.showEmojiPanel()` _macOS_ _Windows_

Show the platform's native emoji picker.

### `app.startAccessingSecurityScopedResource(bookmarkData)` _mas_

* `bookmarkData` String - The base64 encoded security scoped bookmark data returned by the `dialog.showOpenDialog` or `dialog.showSaveDialog` methods.

Returns `Function` - This function **must** be called once you have finished accessing the security scoped file. If you do not remember to stop accessing the bookmark, [kernel resources will be leaked](https://developer.apple.com/reference/foundation/nsurl/1417051-startaccessingsecurityscopedreso?language=objc) and your app will lose its ability to reach outside the sandbox completely, until your app is restarted.

```js
// Start accessing the file.
const stopAccessingSecurityScopedResource = app.startAccessingSecurityScopedResource(data)
// You can now access the file outside of the sandbox 🎉

// Remember to stop accessing the file once you've finished with it.
stopAccessingSecurityScopedResource()
```

Start accessing a security scoped resource. With this method Electron applications that are packaged for the Mac App Store may reach outside their sandbox to access files chosen by the user. See [Apple's documentation](https://developer.apple.com/library/content/documentation/Security/Conceptual/AppSandboxDesignGuide/AppSandboxInDepth/AppSandboxInDepth.html#//apple_ref/doc/uid/TP40011183-CH3-SW16) for a description of how this system works.

### `app.enableSandbox()`

Enables full sandbox mode on the app. This means that all renderers will be launched sandboxed, regardless of the value of the `sandbox` flag in WebPreferences.

This method can only be called before app is ready.

### `app.isInApplicationsFolder()` _macOS_

Returns `Boolean` - Whether the application is currently running from the
systems Application folder. Use in combination with `app.moveToApplicationsFolder()`

### `app.moveToApplicationsFolder([options])` _macOS_

* `options` Object (optional)
  * `conflictHandler` Function<Boolean> (optional) - A handler for potential conflict in move failure.
    * `conflictType` String - The type of move conflict encountered by the handler; can be `exists` or `existsAndRunning`, where `exists` means that an app of the same name is present in the Applications directory and `existsAndRunning` means both that it exists and that it's presently running.

Returns `Boolean` - Whether the move was successful. Please note that if
the move is successful, your application will quit and relaunch.

No confirmation dialog will be presented by default. If you wish to allow
the user to confirm the operation, you may do so using the
[`dialog`](dialog.md) API.

**NOTE:** This method throws errors if anything other than the user causes the
move to fail. For instance if the user cancels the authorization dialog, this
method returns false. If we fail to perform the copy, then this method will
throw an error. The message in the error should be informative and tell
you exactly what went wrong.

By default, if an app of the same name as the one being moved exists in the Applications directory and is _not_ running, the existing app will be trashed and the active app moved into its place. If it _is_ running, the pre-existing running app will assume focus and the the previously active app will quit itself. This behavior can be changed by providing the optional conflict handler, where the boolean returned by the handler determines whether or not the move conflict is resolved with default behavior.  i.e. returning `false` will ensure no further action is taken, returning `true` will result in the default behavior and the method continuing.

For example:

```js
app.moveToApplicationsFolder({
  conflictHandler: (conflictType) => {
    if (conflictType === 'exists') {
      return dialog.showMessageBoxSync({
        type: 'question',
        buttons: ['Halt Move', 'Continue Move'],
        defaultId: 0,
        message: 'An app of this name already exists'
      }) === 1
    }
  }
})
```

Would mean that if an app already exists in the user directory, if the user chooses to 'Continue Move' then the function would continue with its default behavior and the existing app will be trashed and the active app moved into its place.

## Properties

### `app.accessibilitySupportEnabled` _macOS_ _Windows_

A `Boolean` property that's `true` if Chrome's accessibility support is enabled, `false` otherwise. This property will be `true` if the use of assistive technologies, such as screen readers, has been detected. Setting this property to `true` manually enables Chrome's accessibility support, allowing developers to expose accessibility switch to users in application settings.

See [Chromium's accessibility docs](https://www.chromium.org/developers/design-documents/accessibility) for more details. Disabled by default.

This API must be called after the `ready` event is emitted.

**Note:** Rendering accessibility tree can significantly affect the performance of your app. It should not be enabled by default.

### `app.applicationMenu`

A `Menu | null` property that returns [`Menu`](menu.md) if one has been set and `null` otherwise.
Users can pass a [Menu](menu.md) to set this property.

### `app.badgeCount` _Linux_ _macOS_

An `Integer` property that returns the badge count for current app. Setting the count to `0` will hide the badge.

On macOS, setting this with any nonzero integer shows on the dock icon. On Linux, this property only works for Unity launcher.

**Note:** Unity launcher requires the existence of a `.desktop` file to work,
for more information please read [Desktop Environment Integration][unity-requirement].

**Note:** On macOS, you need to ensure that your application has the permission
to display notifications for this property to take effect.

### `app.commandLine` _Readonly_

A [`CommandLine`](./command-line.md) object that allows you to read and manipulate the
command line arguments that Chromium uses.

### `app.dock` _macOS_ _Readonly_

A [`Dock`](./dock.md) `| undefined` object that allows you to perform actions on your app icon in the user's
dock on macOS.

### `app.isPackaged` _Readonly_

A `Boolean` property that returns  `true` if the app is packaged, `false` otherwise. For many apps, this property can be used to distinguish development and production environments.

[dock-menu]:https://developer.apple.com/macos/human-interface-guidelines/menus/dock-menus/
[tasks]:https://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[electron-forge]: https://www.electronforge.io/
[electron-packager]: https://github.com/electron/electron-packager
[CFBundleURLTypes]: https://developer.apple.com/library/ios/documentation/General/Reference/InfoPlistKeyReference/Articles/CoreFoundationKeys.html#//apple_ref/doc/uid/TP40009249-102207-TPXREF115
[LSCopyDefaultHandlerForURLScheme]: https://developer.apple.com/library/mac/documentation/Carbon/Reference/LaunchServicesReference/#//apple_ref/c/func/LSCopyDefaultHandlerForURLScheme
[handoff]: https://developer.apple.com/library/ios/documentation/UserExperience/Conceptual/Handoff/HandoffFundamentals/HandoffFundamentals.html
[activity-type]: https://developer.apple.com/library/ios/documentation/Foundation/Reference/NSUserActivity_Class/index.html#//apple_ref/occ/instp/NSUserActivity/activityType
[unity-requirement]: ../tutorial/desktop-environment-integration.md#unity-launcher
[mas-builds]: ../tutorial/mac-app-store-submission-guide.md
[Squirrel-Windows]: https://github.com/Squirrel/Squirrel.Windows
[JumpListBeginListMSDN]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378398(v=vs.85).aspx
[about-panel-options]: https://developer.apple.com/reference/appkit/nsapplication/1428479-orderfrontstandardaboutpanelwith?language=objc

### `app.name`

A `String` property that indicates the current application's name, which is the name in the application's `package.json` file.

Usually the `name` field of `package.json` is a short lowercase name, according
to the npm modules spec. You should usually also specify a `productName`
field, which is your application's full capitalized name, and which will be
preferred over `name` by Electron.

### `app.userAgentFallback`

A `String` which is the user agent string Electron will use as a global fallback.

This is the user agent that will be used when no user agent is set at the
`webContents` or `session` level.  It is useful for ensuring that your entire
app has the same user agent.  Set to a custom value as early as possible
in your app's initialization to ensure that your overridden value is used.

### `app.allowRendererProcessReuse`

A `Boolean` which when `true` disables the overrides that Electron has in place
to ensure renderer processes are restarted on every navigation.  The current
default value for this property is `false`.

The intention is for these overrides to become disabled by default and then at
some point in the future this property will be removed.  This property impacts
which native modules you can use in the renderer process.  For more information
on the direction Electron is going with renderer process restarts and usage of
native modules in the renderer process please check out this
[Tracking Issue](https://github.com/electron/electron/issues/18397).
