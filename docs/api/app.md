# app

> Control your application's event lifecycle.

The following example shows how to quit the application when the last window is
closed:

```javascript
const app = require('electron').app;
app.on('window-all-closed', function() {
  app.quit();
});
```

## Events

The `app` object emits the following events:

### Event: 'will-finish-launching'

Emitted when the application has finished basic startup. On Windows and Linux,
the `will-finish-launching` event is the same as the `ready` event; on OS X,
this event represents the `applicationWillFinishLaunching` notification of
`NSApplication`. You would usually set up listeners for the `open-file` and
`open-url` events here, and start the crash reporter and auto updater.

In most cases, you should just do everything in the `ready` event handler.

### Event: 'ready'

Emitted when Electron has finished initialization.

### Event: 'window-all-closed'

Emitted when all windows have been closed.

This event is only emitted when the application is not going to quit. If the
user pressed `Cmd + Q`, or the developer called `app.quit()`, Electron will
first try to close all the windows and then emit the `will-quit` event, and in
this case the `window-all-closed` event would not be emitted.

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

### Event: 'open-file' _OS X_

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

On Windows, you have to parse `process.argv` (in the main process) to get the filepath.

### Event: 'open-url' _OS X_

Returns:

* `event` Event
* `url` String

Emitted when the user wants to open a URL with the application. The URL scheme
must be registered to be opened by your application.

You should call `event.preventDefault()` if you want to handle this event.

### Event: 'activate' _OS X_

Returns:

* `event` Event
* `hasVisibleWindows` Boolean

Emitted when the application is activated, which usually happens when clicks on
the applications's dock icon.

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

### Event: 'certificate-error'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `error` String - The error code
* `certificate` Object
  * `data` Buffer - PEM encoded data
  * `issuerName` String
* `callback` Function

Emitted when failed to verify the `certificate` for `url`, to trust the
certificate you should prevent the default behavior with
`event.preventDefault()` and call `callback(true)`.

```javascript
app.on('certificate-error', function(event, webContents, url, error, certificate, callback) {
  if (url == "https://github.com") {
    // Verification logic.
    event.preventDefault();
    callback(true);
  } else {
    callback(false);
  }
});
```

### Event: 'select-client-certificate'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `certificateList` [Objects]
  * `data` Buffer - PEM encoded data
  * `issuerName` String - Issuer's Common Name
* `callback` Function

Emitted when a client certificate is requested.

The `url` corresponds to the navigation entry requesting the client certificate
and `callback` needs to be called with an entry filtered from the list. Using
`event.preventDefault()` prevents the application from using the first
certificate from the store.

```javascript
app.on('select-client-certificate', function(event, webContents, url, list, callback) {
  event.preventDefault();
  callback(list[0]);
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
app.on('login', function(event, webContents, request, authInfo, callback) {
  event.preventDefault();
  callback('username', 'secret');
})
```

### Event: 'gpu-process-crashed'

Emitted when the gpu process crashes.

### Event: 'platform-theme-changed' _OS X_

Emitted when the system's Dark Mode theme is toggled.

## Methods

The `app` object has the following methods:

**Note:** Some methods are only available on specific operating systems and are labeled as such.

### `app.quit()`

Try to close all windows. The `before-quit` event will be emitted first. If all
windows are successfully closed, the `will-quit` event will be emitted and by
default the application will terminate.

This method guarantees that all `beforeunload` and `unload` event handlers are
correctly executed. It is possible that a window cancels the quitting by
returning `false` in the `beforeunload` event handler.

### `app.exit(exitCode)`

* `exitCode` Integer

Exits immediately with `exitCode`.

All windows will be closed immediately without asking user and the `before-quit`
and `will-quit` events will not be emitted.

### `app.focus()`

On Linux, focuses on the first visible window. On OS X, makes the application
the active app. On Windows, focuses on the application's first window.

### `app.hide()` _OS X_

Hides all application windows without minimizing them.

### `app.show()` _OS X_

Shows application windows after they were hidden. Does not automatically focus them.

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
  * `~/Library/Application Support` on OS X
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

Returns the current application locale.

**Note:** When distributing your packaged app, you have to also ship the
`locales` folder.

**Note:** On Windows you have to call it after the `ready` events gets emitted.

### `app.addRecentDocument(path)` _OS X_ _Windows_

* `path` String

Adds `path` to the recent documents list.

This list is managed by the OS. On Windows you can visit the list from the task
bar, and on OS X you can visit it from dock menu.

### `app.clearRecentDocuments()` _OS X_ _Windows_

Clears the recent documents list.

### `app.setAsDefaultProtocolClient(protocol)` _OS X_ _Windows_

* `protocol` String - The name of your protocol, without `://`. If you want your
  app to handle `electron://` links, call this method with `electron` as the
  parameter.

This method sets the current executable as the default handler for a protocol
(aka URI scheme). It allows you to integrate your app deeper into the operating
system. Once registered, all links with `your-protocol://` will be openend with
the current executable. The whole link, including protocol, will be passed to
your application as a parameter.

**Note:** On OS X, you can only register protocols that have been added to
your app's `info.plist`, which can not be modified at runtime. You can however
change the file with a simple text editor or script during build time.
Please refer to [Apple's documentation][CFBundleURLTypes] for details.

The API uses the Windows Registry and LSSetDefaultHandlerForURLScheme internally.

### `app.removeAsDefaultProtocolClient(protocol)` _Windows_

* `protocol` String - The name of your protocol, without `://`.

This method checks if the current executable as the default handler for a protocol
(aka URI scheme). If so, it will remove the app as the default handler.

**Note:** On OS X, removing the app will automatically remove the app as the
default protocol handler.

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

### `app.allowNTLMCredentialsForAllDomains(allow)`

* `allow` Boolean

Dynamically sets whether to always send credentials for HTTP NTLM or Negotiate
authentication - normally, Electron will only send NTLM/Kerberos credentials for
URLs that fall under "Local Intranet" sites (i.e. are in the same domain as you).
However, this detection often fails when corporate networks are badly configured,
so this lets you co-opt this behavior and enable it for all URLs.

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

On OS X the system enforces single instance automatically when users try to open
a second instance of your app in Finder, and the `open-file` and `open-url`
events will be emitted for that. However when users start your app in command
line the system's single instance mechanism will be bypassed and you have to
use this method to ensure single instance.

An example of activating the window of primary instance when a second instance
starts:

```javascript
var myWindow = null;

var shouldQuit = app.makeSingleInstance(function(commandLine, workingDirectory) {
  // Someone tried to run a second instance, we should focus our window.
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore();
    myWindow.focus();
  }
});

if (shouldQuit) {
  app.quit();
  return;
}

// Create myWindow, load the rest of the app, etc...
app.on('ready', function() {
});
```

### `app.setAppUserModelId(id)` _Windows_

* `id` String

Changes the [Application User Model ID][app-user-model-id] to `id`.

### `app.isAeroGlassEnabled()` _Windows_

This method returns `true` if [DWM composition](https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx)
(Aero Glass) is enabled, and `false` otherwise. You can use it to determine if
you should create a transparent window or not (transparent windows won't work
correctly when DWM composition is disabled).

Usage example:

```javascript
let browserOptions = {width: 1000, height: 800};

// Make the window transparent only if the platform supports it.
if (process.platform !== 'win32' || app.isAeroGlassEnabled()) {
  browserOptions.transparent = true;
  browserOptions.frame = false;
}

// Create the window.
win = new BrowserWindow(browserOptions);

// Navigate.
if (browserOptions.transparent) {
  win.loadURL('file://' + __dirname + '/index.html');
} else {
  // No transparency, so we load a fallback that uses basic styles.
  win.loadURL('file://' + __dirname + '/fallback.html');
}
```

### `app.isDarkMode()` _OS X_

This method returns `true` if the system is in Dark Mode, and `false` otherwise.

### `app.importCertificate(options, callback)` _LINUX_

* `options` Object
  * `certificate` String - Path for the pkcs12 file.
  * `password` String - Passphrase for the certificate.
* `callback` Function
  * `result` Integer - Result of import.

Imports the certificate in pkcs12 format into the platform certificate store.
`callback` is called with the `result` of import operation, a value of `0`
indicates success while any other value indicates failure according to chromium [net_error_list](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).

### `app.commandLine.appendSwitch(switch[, value])`

Append a switch (with optional `value`) to Chromium's command line.

**Note:** This will not affect `process.argv`, and is mainly used by developers
to control some low-level Chromium behaviors.

### `app.commandLine.appendArgument(value)`

Append an argument to Chromium's command line. The argument will be quoted
correctly.

**Note:** This will not affect `process.argv`.

### `app.dock.bounce([type])` _OS X_

* `type` String (optional) - Can be `critical` or `informational`. The default is
 `informational`

When `critical` is passed, the dock icon will bounce until either the
application becomes active or the request is canceled.

When `informational` is passed, the dock icon will bounce for one second.
However, the request remains active until either the application becomes active
or the request is canceled.

Returns an ID representing the request.

### `app.dock.cancelBounce(id)` _OS X_

* `id` Integer

Cancel the bounce of `id`.

### `app.dock.setBadge(text)` _OS X_

* `text` String

Sets the string to be displayed in the dock’s badging area.

### `app.dock.getBadge()` _OS X_

Returns the badge string of the dock.

### `app.dock.hide()` _OS X_

Hides the dock icon.

### `app.dock.show()` _OS X_

Shows the dock icon.

### `app.dock.setMenu(menu)` _OS X_

* `menu` [Menu](menu.md)

Sets the application's [dock menu][dock-menu].

### `app.dock.setIcon(image)` _OS X_

* `image` [NativeImage](native-image.md)

Sets the `image` associated with this dock icon.

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[CFBundleURLTypes]: https://developer.apple.com/library/ios/documentation/General/Reference/InfoPlistKeyReference/Articles/CoreFoundationKeys.html#//apple_ref/doc/uid/TP40009249-102207-TPXREF115
