# app

> Control your application's event lifecycle.

The following example shows how to quit the application when the last window is
closed:

```javascript
const {app} = require('electron')
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

In most cases, you should just do everything in the `ready` event handler.

### Event: 'ready'

Returns:

* `launchInfo` Object _macOS_

Emitted when Electron has finished initializing. On macOS, `launchInfo` holds
the `userInfo` of the `NSUserNotification` that was used to open the application,
if it was launched from Notification Center. You can call `app.isReady()` to
check if this event has already fired.

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
Calling `event.preventDefault()` will prevent the default behaviour, which is
terminating the application.

### Event: 'will-quit'

Returns:

* `event` Event

Emitted when all windows have been closed and the application will quit.
Calling `event.preventDefault()` will prevent the default behaviour, which is
terminating the application.

See the description of the `window-all-closed` event for the differences between
the `will-quit` and `window-all-closed` events.

### Event: 'quit'

Returns:

* `event` Event
* `exitCode` Integer

Emitted when the application is quitting.

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

Emitted when the user wants to open a URL with the application. The URL scheme
must be registered to be opened by your application.

You should call `event.preventDefault()` if you want to handle this event.

### Event: 'activate' _macOS_

Returns:

* `event` Event
* `hasVisibleWindows` Boolean

Emitted when the application is activated, which usually happens when the user
clicks on the application's dock icon.

### Event: 'continue-activity' _macOS_

Returns:

* `event` Event
* `type` String - A string identifying the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` Object - Contains app-specific state stored by the activity on
  another device.

Emitted during [Handoff][handoff] when an activity from a different device wants
to be resumed. You should call `event.preventDefault()` if you want to handle
this event.

A user activity can be continued only in an app that has the same developer Team
ID as the activity's source app and that supports the activity's type.
Supported activity types are specified in the app's `Info.plist` under the
`NSUserActivityTypes` key.

### Event: 'browser-window-blur'

Returns:

* `event` Event
* `window` BrowserWindow

Emitted when a [browserWindow](browser-window.md) gets blurred.

### Event: 'browser-window-focus'

Returns:

* `event` Event
* `window` BrowserWindow

Emitted when a [browserWindow](browser-window.md) gets focused.

### Event: 'browser-window-created'

Returns:

* `event` Event
* `window` BrowserWindow

Emitted when a new [browserWindow](browser-window.md) is created.

### Event: 'web-contents-created'

Returns:

* `event` Event
* `webContents` WebContents

Emitted when a new [webContents](web-contents.md) is created.

### Event: 'certificate-error'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `error` String - The error code
* `certificate` Object
  * `data` String - PEM encoded data
  * `issuerName` String - Issuer's Common Name
  * `subjectName` String - Subject's Common Name
  * `serialNumber` String - Hex value represented string
  * `validStart` Integer - Start date of the certificate being valid in seconds
  * `validExpiry` Integer - End date of the certificate being valid in seconds
  * `fingerprint` String - Fingerprint of the certificate
* `callback` Function

Emitted when failed to verify the `certificate` for `url`, to trust the
certificate you should prevent the default behavior with
`event.preventDefault()` and call `callback(true)`.

```javascript
const {app} = require('electron')

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
* `certificateList` [Objects]
  * `data` String - PEM encoded data
  * `issuerName` String - Issuer's Common Name
  * `subjectName` String - Subject's Common Name
  * `serialNumber` String - Hex value represented string
  * `validStart` Integer - Start date of the certificate being valid in seconds
  * `validExpiry` Integer - End date of the certificate being valid in seconds
  * `fingerprint` String - Fingerprint of the certificate
* `callback` Function

Emitted when a client certificate is requested.

The `url` corresponds to the navigation entry requesting the client certificate
and `callback` needs to be called with an entry filtered from the list. Using
`event.preventDefault()` prevents the application from using the first
certificate from the store.

```javascript
const {app} = require('electron')

app.on('select-client-certificate', (event, webContents, url, list, callback) => {
  event.preventDefault()
  callback(list[0])
})
```

### Event: 'login'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

Emitted when `webContents` wants to do basic auth.

The default behavior is to cancel all authentications, to override this you
should prevent the default behavior with `event.preventDefault()` and call
`callback(username, password)` with the credentials.

```javascript
const {app} = require('electron')

app.on('login', (event, webContents, request, authInfo, callback) => {
  event.preventDefault()
  callback('username', 'secret')
})
```

### Event: 'gpu-process-crashed'

Emitted when the gpu process crashes.

### Event: 'accessibility-support-changed' _macOS_ _Windows_

Returns:

* `event` Event
* `accessibilitySupportEnabled` Boolean - `true` when Chrome's accessibility
  support is enabled, `false` otherwise.

Emitted when Chrome's accessibility support changes. This event fires when
assistive technologies, such as screen readers, are enabled or disabled.
See https://www.chromium.org/developers/design-documents/accessibility for more
details.

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

Exits immediately with `exitCode`.  `exitCode` defaults to 0.

All windows will be closed immediately without asking user and the `before-quit`
and `will-quit` events will not be emitted.

### `app.relaunch([options])`

* `options` Object (optional)
  * `args` Array (optional)
  * `execPath` String (optional)

Relaunches the app when current instance exits.

By default the new instance will use the same working directory and command line
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
const {app} = require('electron')

app.relaunch({args: process.argv.slice(1) + ['--relaunch']})
app.exit(0)
```

### `app.isReady()`

Returns `true` if Electron has finished initializing, `false` otherwise.

### `app.focus()`

On Linux, focuses on the first visible window. On macOS, makes the application
the active app. On Windows, focuses on the application's first window.

### `app.hide()` _macOS_

Hides all application windows without minimizing them.

### `app.show()` _macOS_

Shows application windows after they were hidden. Does not automatically focus
them.

### `app.getAppPath()`

Returns the current application directory.

### `app.getPath(name)`

* `name` String

Retrieves a path to a special directory or file associated with `name`. On
failure an `Error` is thrown.

You can request the following paths by the name:

* `home` User's home directory.
* `appData` Per-user application data directory, which by default points to:
  * `%APPDATA%` on Windows
  * `$XDG_CONFIG_HOME` or `~/.config` on Linux
  * `~/Library/Application Support` on macOS
* `userData` The directory for storing your app's configuration files, which by
  default it is the `appData` directory appended with your app's name.
* `temp` Temporary directory.
* `exe` The current executable file.
* `module` The `libchromiumcontent` library.
* `desktop` The current user's Desktop directory.
* `documents` Directory for a user's "My Documents".
* `downloads` Directory for a user's downloads.
* `music` Directory for a user's music.
* `pictures` Directory for a user's pictures.
* `videos` Directory for a user's videos.
* `pepperFlashSystemPlugin`  Full path to the system version of the Pepper Flash plugin.

### `app.setPath(name, path)`

* `name` String
* `path` String

Overrides the `path` to a special directory or file associated with `name`. If
the path specifies a directory that does not exist, the directory will be
created by this method. On failure an `Error` is thrown.

You can only override paths of a `name` defined in `app.getPath`.

By default, web pages' cookies and caches will be stored under the `userData`
directory. If you want to change this location, you have to override the
`userData` path before the `ready` event of the `app` module is emitted.

### `app.getVersion()`

Returns the version of the loaded application. If no version is found in the
application's `package.json` file, the version of the current bundle or
executable is returned.

### `app.getName()`

Returns the current application's name, which is the name in the application's
`package.json` file.

Usually the `name` field of `package.json` is a short lowercased name, according
to the npm modules spec. You should usually also specify a `productName`
field, which is your application's full capitalized name, and which will be
preferred over `name` by Electron.

### `app.setName(name)`

* `name` String

Overrides the current application's name.

### `app.getLocale()`

Returns the current application locale. Possible return values are documented
[here](locales.md).

**Note:** When distributing your packaged app, you have to also ship the
`locales` folder.

**Note:** On Windows you have to call it after the `ready` events gets emitted.

### `app.addRecentDocument(path)` _macOS_ _Windows_

* `path` String

Adds `path` to the recent documents list.

This list is managed by the OS. On Windows you can visit the list from the task
bar, and on macOS you can visit it from dock menu.

### `app.clearRecentDocuments()` _macOS_ _Windows_

Clears the recent documents list.

### `app.setAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - The name of your protocol, without `://`. If you want your
  app to handle `electron://` links, call this method with `electron` as the
  parameter.
* `path` String (optional) _Windows_ - Defaults to `process.execPath`
* `args` Array (optional) _Windows_ - Defaults to an empty array

This method sets the current executable as the default handler for a protocol
(aka URI scheme). It allows you to integrate your app deeper into the operating
system. Once registered, all links with `your-protocol://` will be opened with
the current executable. The whole link, including protocol, will be passed to
your application as a parameter.

On Windows you can provide optional parameters path, the path to your executable,
and args, an array of arguments to be passed to your executable when it launches.

Returns `true` when the call succeeded, otherwise returns `false`.

**Note:** On macOS, you can only register protocols that have been added to
your app's `info.plist`, which can not be modified at runtime. You can however
change the file with a simple text editor or script during build time.
Please refer to [Apple's documentation][CFBundleURLTypes] for details.

The API uses the Windows Registry and LSSetDefaultHandlerForURLScheme internally.

### `app.removeAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - The name of your protocol, without `://`.
* `path` String (optional) _Windows_ - Defaults to `process.execPath`
* `args` Array (optional) _Windows_ - Defaults to an empty array

This method checks if the current executable as the default handler for a
protocol (aka URI scheme). If so, it will remove the app as the default handler.

Returns `true` when the call succeeded, otherwise returns `false`.

### `app.isDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - The name of your protocol, without `://`.
* `path` String (optional) _Windows_ - Defaults to `process.execPath`
* `args` Array (optional) _Windows_ - Defaults to an empty array

This method checks if the current executable is the default handler for a protocol
(aka URI scheme). If so, it will return true. Otherwise, it will return false.

**Note:** On macOS, you can use this method to check if the app has been
registered as the default protocol handler for a protocol. You can also verify
this by checking `~/Library/Preferences/com.apple.LaunchServices.plist` on the
macOS machine. Please refer to
[Apple's documentation][LSCopyDefaultHandlerForURLScheme] for details.

The API uses the Windows Registry and LSCopyDefaultHandlerForURLScheme internally.

### `app.setUserTasks(tasks)` _Windows_

* `tasks` Array - Array of `Task` objects

Adds `tasks` to the [Tasks][tasks] category of the JumpList on Windows.

`tasks` is an array of `Task` objects in the following format:

`Task` Object:

* `program` String - Path of the program to execute, usually you should
  specify `process.execPath` which opens the current program.
* `arguments` String - The command line arguments when `program` is
  executed.
* `title` String - The string to be displayed in a JumpList.
* `description` String - Description of this task.
* `iconPath` String - The absolute path to an icon to be displayed in a
  JumpList, which can be an arbitrary resource file that contains an icon. You
  can usually specify `process.execPath` to show the icon of the program.
* `iconIndex` Integer - The icon index in the icon file. If an icon file
  consists of two or more icons, set this value to identify the icon. If an
  icon file consists of one icon, this value is 0.

Returns `true` when the call succeeded, otherwise returns `false`.

**Note:** If you'd like to customize the Jump List even more use
`app.setJumpList(categories)` instead.

### `app.getJumpListSettings()` _Windows_

Returns an Object with the following properties:

* `minItems` Integer - The minimum number of items that will be shown in the
  Jump List (for a more detailed description of this value see the
  [MSDN docs][JumpListBeginListMSDN]).
* `removedItems` Array - Array of `JumpListItem` objects that correspond to
  items that the user has explicitly removed from custom categories in the
  Jump List. These items must not be re-added to the Jump List in the **next**
  call to `app.setJumpList()`, Windows will not display any custom category
  that contains any of the removed items.

### `app.setJumpList(categories)` _Windows_

* `categories` Array or `null` - Array of `JumpListCategory` objects.

Sets or removes a custom Jump List for the application, and returns one of the
following strings:

* `ok` - Nothing went wrong.
* `error` - One or more errors occured, enable runtime logging to figure out
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

`JumpListCategory` objects should have the following properties:

* `type` String - One of the following:
  * `tasks` - Items in this category will be placed into the standard `Tasks`
    category. There can be only one such category, and it will always be
    displayed at the bottom of the Jump List.
  * `frequent` - Displays a list of files frequently opened by the app, the
    name of the category and its items are set by Windows.
  * `recent` - Displays a list of files recently opened by the app, the name
    of the category and its items are set by Windows. Items may be added to
    this category indirectly using `app.addRecentDocument(path)`.
  * `custom` - Displays tasks or file links, `name` must be set by the app.
* `name` String - Must be set if `type` is `custom`, otherwise it should be
  omitted.
* `items` Array - Array of `JumpListItem` objects if `type` is `tasks` or
  `custom`, otherwise it should be omitted.

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

`JumpListItem` objects should have the following properties:

* `type` String - One of the following:
  * `task` - A task will launch an app with specific arguments.
  * `separator` - Can be used to separate items in the standard `Tasks`
    category.
  * `file` - A file link will open a file using the app that created the
    Jump List, for this to work the app must be registered as a handler for
    the file type (though it doesn't have to be the default handler).
* `path` String - Path of the file to open, should only be set if `type` is
  `file`.
* `program` String - Path of the program to execute, usually you should
  specify `process.execPath` which opens the current program. Should only be
  set if `type` is `task`.
* `args` String - The command line arguments when `program` is executed. Should
  only be set if `type` is `task`.
* `title` String - The text to be displayed for the item in the Jump List.
  Should only be set if `type` is `task`.
* `description` String - Description of the task (displayed in a tooltip).
  Should only be set if `type` is `task`.
* `iconPath` String - The absolute path to an icon to be displayed in a
  Jump List, which can be an arbitrary resource file that contains an icon
  (e.g. `.ico`, `.exe`, `.dll`). You can usually specify `process.execPath` to
  show the program icon.
* `iconIndex` Integer - The index of the icon in the resource file. If a
  resource file contains multiple icons this value can be used to specify the
  zero-based index of the icon that should be displayed for this task. If a
  resource file contains only one icon, this property should be set to zero.

Here's a very simple example of creating a custom Jump List:

```javascript
const {app} = require('electron')

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
        type: 'task', title: 'Tool A',
        program: process.execPath, args: '--run-tool-a',
        icon: process.execPath, iconIndex: 0,
        description: 'Runs Tool A'
      },
      {
        type: 'task', title: 'Tool B',
        program: process.execPath, args: '--run-tool-b',
        icon: process.execPath, iconIndex: 0,
        description: 'Runs Tool B'
      }
    ]
  },
  { type: 'frequent' },
  { // has no name and no type so `type` is assumed to be "tasks"
    items: [
      {
        type: 'task', title: 'New Project',
        program: process.execPath, args: '--new-project',
        description: 'Create a new project.'
      },
      { type: 'separator' },
      {
        type: 'task', title: 'Recover Project',
        program: process.execPath, args: '--recover-project',
        description: 'Recover Project'
      }
    ]
  }
])
```

### `app.makeSingleInstance(callback)`

* `callback` Function

This method makes your application a Single Instance Application - instead of
allowing multiple instances of your app to run, this will ensure that only a
single instance of your app is running, and other instances signal this
instance and exit.

`callback` will be called with `callback(argv, workingDirectory)` when a second
instance has been executed. `argv` is an Array of the second instance's command
line arguments, and `workingDirectory` is its current working directory. Usually
applications respond to this by making their primary window focused and
non-minimized.

The `callback` is guaranteed to be executed after the `ready` event of `app`
gets emitted.

This method returns `false` if your process is the primary instance of the
application and your app should continue loading. And returns `true` if your
process has sent its parameters to another instance, and you should immediately
quit.

On macOS the system enforces single instance automatically when users try to open
a second instance of your app in Finder, and the `open-file` and `open-url`
events will be emitted for that. However when users start your app in command
line the system's single instance mechanism will be bypassed and you have to
use this method to ensure single instance.

An example of activating the window of primary instance when a second instance
starts:

```javascript
const {app} = require('electron')
let myWindow = null

const shouldQuit = app.makeSingleInstance((commandLine, workingDirectory) => {
  // Someone tried to run a second instance, we should focus our window.
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore()
    myWindow.focus()
  }
})

if (shouldQuit) {
  app.quit()
}

// Create myWindow, load the rest of the app, etc...
app.on('ready', () => {
})
```

### `app.releaseSingleInstance()`

Releases all locks that were created by `makeSingleInstance`. This will allow
multiple instances of the application to once again run side by side.

### `app.setUserActivity(type, userInfo[, webpageURL])` _macOS_

* `type` String - Uniquely identifies the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` Object - App-specific state to store for use by another device.
* `webpageURL` String - The webpage to load in a browser if no suitable app is
  installed on the resuming device. The scheme must be `http` or `https`.

Creates an `NSUserActivity` and sets it as the current activity. The activity
is eligible for [Handoff][handoff] to another device afterward.

### `app.getCurrentActivityType()` _macOS_

Returns the type of the currently running activity.

### `app.setAppUserModelId(id)` _Windows_

* `id` String

Changes the [Application User Model ID][app-user-model-id] to `id`.

### `app.importCertificate(options, callback)` _LINUX_

* `options` Object
  * `certificate` String - Path for the pkcs12 file.
  * `password` String - Passphrase for the certificate.
* `callback` Function
  * `result` Integer - Result of import.

Imports the certificate in pkcs12 format into the platform certificate store.
`callback` is called with the `result` of import operation, a value of `0`
indicates success while any other value indicates failure according to chromium [net_error_list](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).

### `app.disableHardwareAcceleration()`

Disables hardware acceleration for current app.

This method can only be called before app is ready.

### `app.setBadgeCount(count)` _Linux_ _macOS_

* `count` Integer

Sets the counter badge for current app. Setting the count to `0` will hide the
badge. Returns `true` when the call succeeded, otherwise returns `false`.

On macOS it shows on the dock icon. On Linux it only works for Unity launcher,

**Note:** Unity launcher requires the exsistence of a `.desktop` file to work,
for more information please read [Desktop Environment Integration][unity-requiremnt].

### `app.getBadgeCount()` _Linux_ _macOS_

Returns the current value displayed in the counter badge.

### `app.isUnityRunning()` _Linux_

Returns whether current desktop environment is Unity launcher.

### `app.getLoginItemSettings()` _macOS_ _Windows_

Return an Object with the login item settings of the app.

* `openAtLogin` Boolean - `true` if the app is set to open at login.
* `openAsHidden` Boolean - `true` if the app is set to open as hidden at login.
  This setting is only supported on macOS.
* `wasOpenedAtLogin` Boolean - `true` if the app was opened at login
  automatically. This setting is only supported on macOS.
* `wasOpenedAsHidden` Boolean - `true` if the app was opened as a hidden login
  item. This indicates that the app should not open any windows at startup.
  This setting is only supported on macOS.
* `restoreState` Boolean - `true` if the app was opened as a login item that
  should restore the state from the previous session. This indicates that the
  app should restore the windows that were open the last time the app was
  closed. This setting is only supported on macOS.

### `app.setLoginItemSettings(settings)` _macOS_ _Windows_

* `settings` Object
  * `openAtLogin` Boolean - `true` to open the app at login, `false` to remove
    the app as a login item. Defaults to `false`.
  * `openAsHidden` Boolean - `true` to open the app as hidden. Defaults to
    `false`. The user can edit this setting from the System Preferences so
    `app.getLoginItemStatus().wasOpenedAsHidden` should be checked when the app
    is opened to know the current value. This setting is only supported on
    macOS.

Set the app's login item settings.

### `app.isAccessibilitySupportEnabled()` _macOS_ _Windows_

Returns a `Boolean`, `true` if Chrome's accessibility support is enabled,
`false` otherwise. This API will return `true` if the use of assistive
technologies, such as screen readers, has been detected. See
https://www.chromium.org/developers/design-documents/accessibility for more
details.

### `app.commandLine.appendSwitch(switch[, value])`

* `switch` String - A command-line switch
* `value` String (optional) - A value for the given switch

Append a switch (with optional `value`) to Chromium's command line.

**Note:** This will not affect `process.argv`, and is mainly used by developers
to control some low-level Chromium behaviors.

### `app.commandLine.appendArgument(value)`

* `value` String - The argument to append to the command line

Append an argument to Chromium's command line. The argument will be quoted
correctly.

**Note:** This will not affect `process.argv`.

### `app.dock.bounce([type])` _macOS_

* `type` String (optional) - Can be `critical` or `informational`. The default is
 `informational`

When `critical` is passed, the dock icon will bounce until either the
application becomes active or the request is canceled.

When `informational` is passed, the dock icon will bounce for one second.
However, the request remains active until either the application becomes active
or the request is canceled.

Returns an ID representing the request.

### `app.dock.cancelBounce(id)` _macOS_

* `id` Integer

Cancel the bounce of `id`.

### `app.dock.downloadFinished(filePath)` _macOS_

* `filePath` String

Bounces the Downloads stack if the filePath is inside the Downloads folder.

### `app.dock.setBadge(text)` _macOS_

* `text` String

Sets the string to be displayed in the dock’s badging area.

### `app.dock.getBadge()` _macOS_

Returns the badge string of the dock.

### `app.dock.hide()` _macOS_

Hides the dock icon.

### `app.dock.show()` _macOS_

Shows the dock icon.

### `app.dock.isVisible()` _macOS_

Returns whether the dock icon is visible.
The `app.dock.show()` call is asynchronous so this method might not
return true immediately after that call.

### `app.dock.setMenu(menu)` _macOS_

* `menu` [Menu](menu.md)

Sets the application's [dock menu][dock-menu].

### `app.dock.setIcon(image)` _macOS_

* `image` [NativeImage](native-image.md)

Sets the `image` associated with this dock icon.

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[CFBundleURLTypes]: https://developer.apple.com/library/ios/documentation/General/Reference/InfoPlistKeyReference/Articles/CoreFoundationKeys.html#//apple_ref/doc/uid/TP40009249-102207-TPXREF115
[LSCopyDefaultHandlerForURLScheme]: https://developer.apple.com/library/mac/documentation/Carbon/Reference/LaunchServicesReference/#//apple_ref/c/func/LSCopyDefaultHandlerForURLScheme
[handoff]: https://developer.apple.com/library/ios/documentation/UserExperience/Conceptual/Handoff/HandoffFundamentals/HandoffFundamentals.html
[activity-type]: https://developer.apple.com/library/ios/documentation/Foundation/Reference/NSUserActivity_Class/index.html#//apple_ref/occ/instp/NSUserActivity/activityType
[unity-requiremnt]: ../tutorial/desktop-environment-integration.md#unity-launcher-shortcuts-linux
[JumpListBeginListMSDN]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378398(v=vs.85).aspx
