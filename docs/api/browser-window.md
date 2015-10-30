# BrowserWindow

The `BrowserWindow` class gives you the ability to create a browser window. For
example:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600, show: false });
win.on('closed', function() {
  win = null;
});

win.loadUrl('https://github.com');
win.show();
```

You can also create a window without chrome by using
[Frameless Window](frameless-window.md) API.

## Class: BrowserWindow

`BrowserWindow` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

It creates a new `BrowserWindow` with native properties as set by the `options`.

### `new BrowserWindow(options)`

`options` Object, properties:

* `width` Integer - Window's width.
* `height` Integer - Window's height.
* `x` Integer - Window's left offset from screen.
* `y` Integer - Window's top offset from screen.
* `use-content-size` Boolean - The `width` and `height` would be used as web
   page's size, which means the actual window's size will include window
   frame's size and be slightly larger.
* `center` Boolean - Show window in the center of the screen.
* `min-width` Integer - Window's minimum width.
* `min-height` Integer - Window's minimum height.
* `max-width` Integer - Window's maximum width.
* `max-height` Integer - Window's maximum height.
* `resizable` Boolean - Whether window is resizable.
* `always-on-top` Boolean - Whether the window should always stay on top of
   other windows.
* `fullscreen` Boolean - Whether the window should show in fullscreen. When
  set to `false` the fullscreen button will be hidden or disabled on OS X.
* `skip-taskbar` Boolean - Whether to show the window in taskbar.
* `kiosk` Boolean - The kiosk mode.
* `title` String - Default window title.
* `icon` [NativeImage](native-image.md) - The window icon, when omitted on
  Windows the executable's icon would be used as window icon.
* `show` Boolean - Whether window should be shown when created.
* `frame` Boolean - Specify `false` to create a
[Frameless Window](frameless-window.md).
* `accept-first-mouse` Boolean - Whether the web view accepts a single
  mouse-down event that simultaneously activates the window.
* `disable-auto-hide-cursor` Boolean - Whether to hide cursor when typing.
* `auto-hide-menu-bar` Boolean - Auto hide the menu bar unless the `Alt`
  key is pressed.
* `enable-larger-than-screen` Boolean - Enable the window to be resized larger
  than screen.
* `background-color` String - Window's background color as Hexadecimal value,
  like `#66CD00` or `#FFF`. This is only implemented on Linux and Windows.
* `dark-theme` Boolean - Forces using dark theme for the window, only works on
  some GTK+3 desktop environments.
* `transparent` Boolean - Makes the window [transparent](frameless-window.md).
* `type` String - Specifies the type of the window, possible types are
  `desktop`, `dock`, `toolbar`, `splash`, `notification`. This only works on
  Linux.
* `standard-window` Boolean - Uses the OS X's standard window instead of the
  textured window. Defaults to `true`.
* `title-bar-style` String, OS X - specifies the style of window title bar.
  This option is supported on OS X 10.10 Yosemite and newer. There are three
  possible values:
  * `default` or not specified results in the standard gray opaque Mac title
  bar.
  * `hidden` results in a hidden title bar and a full size content window, yet
  the title bar still has the standard window controls ("traffic lights") in
  the top left.
  * `hidden-inset` results in a hidden title bar with an alternative look
  where the traffic light buttons are slightly more inset from the window edge.
* `web-preferences` Object - Settings of web page's features, properties:
  * `node-integration` Boolean - Whether node integration is enabled. Default
    is `true`.
  * `preload` String - Specifies a script that will be loaded before other
    scripts run in the page. This script will always have access to node APIs
    no matter whether node integration is turned on for the page, and the path
    of `preload` script has to be absolute path.
  * `partition` String - Sets the session used by the page. If `partition`
    starts with `persist:`, the page will use a persistent session available to
    all pages in the app with the same `partition`. if there is no `persist:`
    prefix, the page will use an in-memory session. By assigning the same
    `partition`, multiple pages can share the same session. If the `partition`
    is unset then default session of the app will be used.
  * `zoom-factor` Number - The default zoom factor of the page, `3.0` represents
    `300%`.
  * `javascript` Boolean
  * `web-security` Boolean - When setting `false`, it will disable the
    same-origin policy (Usually using testing websites by people), and set
    `allow_displaying_insecure_content` and `allow_running_insecure_content` to
    `true` if these two options are not set by user.
  * `allow-displaying-insecure-content` Boolean - Allow an https page to display
    content like images from http URLs.
  * `allow-running-insecure-content` Boolean - Allow a https page to run
    JavaScript, CSS or plugins from http URLs.
  * `images` Boolean
  * `java` Boolean
  * `text-areas-are-resizable` Boolean
  * `webgl` Boolean
  * `webaudio` Boolean
  * `plugins` Boolean - Whether plugins should be enabled.
  * `experimental-features` Boolean
  * `experimental-canvas-features` Boolean
  * `overlay-scrollbars` Boolean
  * `overlay-fullscreen-video` Boolean
  * `shared-worker` Boolean
  * `direct-write` Boolean - Whether the DirectWrite font rendering system on
     Windows is enabled.
  * `page-visibility` Boolean - Page would be forced to be always in visible
     or hidden state once set, instead of reflecting current window's
     visibility. Users can set it to `true` to prevent throttling of DOM
     timers.

## Events

The `BrowserWindow` object emits the following events:

**Note:** Some events are only available on specific operating systems and are labeled as such.

### Event: 'page-title-updated'

Returns:

* `event` Event

Emitted when the document changed its title, calling `event.preventDefault()`
would prevent the native window's title to change.

### Event: 'close'

Returns:

* `event` Event

Emitted when the window is going to be closed. It's emitted before the
`beforeunload` and `unload` event of the DOM. Calling `event.preventDefault()`
will cancel the close.

Usually you would want to use the `beforeunload` handler to decide whether the
window should be closed, which will also be called when the window is
reloaded. In Electron, returning an empty string or `false` would cancel the
close. For example:

```javascript
window.onbeforeunload = function(e) {
  console.log('I do not want to be closed');

  // Unlike usual browsers, in which a string should be returned and the user is
  // prompted to confirm the page unload, Electron gives developers more options.
  // Returning empty string or false would prevent the unloading now.
  // You can also use the dialog API to let the user confirm closing the application.
  e.returnValue = false;
};
```

### Event: 'closed'

Emitted when the window is closed. After you have received this event you should
remove the reference to the window and avoid using it anymore.

### Event: 'unresponsive'

Emitted when the web page becomes unresponsive.

### Event: 'responsive'

Emitted when the unresponsive web page becomes responsive again.

### Event: 'blur'

Emitted when the window loses focus.

### Event: 'focus'

Emitted when the window gains focus.

### Event: 'maximize'

Emitted when window is maximized.

### Event: 'unmaximize'

Emitted when the window exits from maximized state.

### Event: 'minimize'

Emitted when the window is minimized.

### Event: 'restore'

Emitted when the window is restored from minimized state.

### Event: 'resize'

Emitted when the window is getting resized.

### Event: 'move'

Emitted when the window is getting moved to a new position.

__Note__: On OS X this event is just an alias of `moved`.

### Event: 'moved' _OS X_

Emitted once when the window is moved to a new position.

### Event: 'enter-full-screen'

Emitted when the window enters full screen state.

### Event: 'leave-full-screen'

Emitted when the window leaves full screen state.

### Event: 'enter-html-full-screen'

Emitted when the window enters full screen state triggered by html api.

### Event: 'leave-html-full-screen'

Emitted when the window leaves full screen state triggered by html api.

### Event: 'app-command':

Emitted when an [App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)
is invoked. These are typically related to keyboard media keys or browser
commands, as well as the "Back" button built into some mice on Windows.

```js
someWindow.on('app-command', function(e, cmd) {
  // Navigate the window back when the user hits their mouse back button
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack();
  }
});
```

## Methods

The `BrowserWindow` object has the following methods:

### `BrowserWindow.getAllWindows()`

Returns an array of all opened browser windows.

### `BrowserWindow.getFocusedWindow()`

Returns the window that is focused in this application.

### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](web-contents.md)

Find a window according to the `webContents` it owns.

### `BrowserWindow.fromId(id)`

* `id` Integer

Find a window according to its ID.

### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

Adds DevTools extension located at `path`, and returns extension's name.

The extension will be remembered so you only need to call this API once, this
API is not for programming use.

### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

Remove the DevTools extension whose name is `name`.

## Instance Properties

Objects created with `new BrowserWindow` have the following properties:

```javascript
var BrowserWindow = require('browser-window');

// In this example `win` is our instance
var win = new BrowserWindow({ width: 800, height: 600 });

```

### `win.webContents`

The `WebContents` object this window owns, all web page related events and
operations will be done via it.

See the [`webContents` documentation](web-contents.md) for its methods and
events.

### `win.id`

The unique ID of this window.

## Instance Methods

Objects created with `new BrowserWindow` have the following instance methods:

**Note:** Some methods are only available on specific operating systems and are labeled as such.

```javascript
var BrowserWindow = require('browser-window');

// In this example `win` is our instance
var win = new BrowserWindow({ width: 800, height: 600 });

```

### `win.destroy()`

Force closing the window, the `unload` and `beforeunload` event won't be emitted
for the web page, and `close` event will also not be emitted
for this window, but it guarantees the `closed` event will be emitted.

You should only use this method when the renderer process (web page) has
crashed.

### `win.close()`

Try to close the window, this has the same effect with user manually clicking
the close button of the window. The web page may cancel the close though, see
the [close event](#event-close).

### `win.focus()`

Focus on the window.

### `win.isFocused()`

Returns a boolean, whether the window is focused.

### `win.show()`

Shows and gives focus to the window.

### `win.showInactive()`

Shows the window but doesn't focus on it.

### `win.hide()`

Hides the window.

### `win.isVisible()`

Returns a boolean, whether the window is visible to the user.

### `win.maximize()`

Maximizes the window.

### `win.unmaximize()`

Unmaximizes the window.

### `win.isMaximized()`

Returns a boolean, whether the window is maximized.

### `win.minimize()`

Minimizes the window. On some platforms the minimized window will be shown in
the Dock.

### `win.restore()`

Restores the window from minimized state to its previous state.

### `win.isMinimized()`

Returns a boolean, whether the window is minimized.

### `win.setFullScreen(flag)`

* `flag` Boolean

Sets whether the window should be in fullscreen mode.

### `win.isFullScreen()`

Returns a boolean, whether the window is in fullscreen mode.

### `win.setAspectRatio(aspectRatio[, extraSize])` _OS X_

* `aspectRatio` The aspect ratio we want to maintain for some portion of the
content view.
* `extraSize` Object (optional) - The extra size not to be included while
maintaining the aspect ratio. Properties:
  * `width` Integer
  * `height` Integer

This will have a window maintain an aspect ratio. The extra size allows a
developer to have space, specified in pixels, not included within the aspect
ratio calculations. This API already takes into account the difference between a
window's size and its content size.

Consider a normal window with an HD video player and associated controls.
Perhaps there are 15 pixels of controls on the left edge, 25 pixels of controls
on the right edge and 50 pixels of controls below the player. In order to
maintain a 16:9 aspect ratio (standard aspect ratio for HD @1920x1080) within
the player itself we would call this function with arguments of 16/9 and
[ 40, 50 ]. The second argument doesn't care where the extra width and height
are within the content view--only that they exist. Just sum any extra width and
height areas you have within the overall content view.

### `win.setBounds(options)`

`options` Object, properties:

* `x` Integer
* `y` Integer
* `width` Integer
* `height` Integer

Resizes and moves the window to `width`, `height`, `x`, `y`.

### `win.getBounds()`

Returns an object that contains window's width, height, x and y values.

### `win.setSize(width, height)`

* `width` Integer
* `height` Integer

Resizes the window to `width` and `height`.

### `win.getSize()`

Returns an array that contains window's width and height.

### `win.setContentSize(width, height)`

* `width` Integer
* `height` Integer

Resizes the window's client area (e.g. the web page) to `width` and `height`.

### `win.getContentSize()`

Returns an array that contains window's client area's width and height.

### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

Sets the minimum size of window to `width` and `height`.

### `win.getMinimumSize()`

Returns an array that contains window's minimum width and height.

### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

Sets the maximum size of window to `width` and `height`.

### `win.getMaximumSize()`

Returns an array that contains window's maximum width and height.

### `win.setResizable(resizable)`

* `resizable` Boolean

Sets whether the window can be manually resized by user.

### `win.isResizable()`

Returns whether the window can be manually resized by user.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

Sets whether the window should show always on top of other windows. After
setting this, the window is still a normal window, not a toolbox window which
can not be focused on.

### `win.isAlwaysOnTop()`

Returns whether the window is always on top of other windows.

### `win.center()`

Moves window to the center of the screen.

### `win.setPosition(x, y)`

* `x` Integer
* `y` Integer

Moves window to `x` and `y`.

### `win.getPosition()`

Returns an array that contains window's current position.

### `win.setTitle(title)`

* `title` String

Changes the title of native window to `title`.

### `win.getTitle()`

Returns the title of the native window.

**Note:** The title of web page can be different from the title of the native
window.

### `win.flashFrame(flag)`

* `flag` Boolean

Starts or stops flashing the window to attract user's attention.

### `win.setSkipTaskbar(skip)`

* `skip` Boolean

Makes the window not show in the taskbar.

### `win.setKiosk(flag)`

* `flag` Boolean

Enters or leaves the kiosk mode.

### `win.isKiosk()`

Returns whether the window is in kiosk mode.

### `win.hookWindowMessage(message, callback)` _WINDOWS_

* `message` Integer
* `callback` Function

Hooks a windows message. The `callback` is called when
the message is received in the WndProc.

### `win.isWindowMessageHooked(message)` _WINDOWS_

* `message` Integer

Returns `true` or `false` depending on whether the message is hooked.

### `win.unhookWindowMessage(message)` _WINDOWS_

* `message` Integer

Unhook the window message.

### `win.unhookAllWindowMessages()` _WINDOWS_

Unhooks all of the window messages.

### `win.setRepresentedFilename(filename)` _OS X_

* `filename` String

Sets the pathname of the file the window represents, and the icon of the file
will show in window's title bar.

### `win.getRepresentedFilename()` _OS X_

Returns the pathname of the file the window represents.

### `win.setDocumentEdited(edited)` _OS X_

* `edited` Boolean

Specifies whether the window’s document has been edited, and the icon in title
bar will become grey when set to `true`.

### `win.isDocumentEdited()` _OS X_

Whether the window's document has been edited.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (optional)- The area of page to be captured, properties:
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

Captures a snapshot of the page within `rect`. Upon completion `callback` will
be called with `callback(image)`. The `image` is an instance of
[NativeImage](native-image.md) that stores data of the snapshot. Omitting
`rect` will capture the whole visible page.

### `win.print([options])`

Same as `webContents.print([options])`

### `win.printToPDF(options, callback)`

Same as `webContents.printToPDF(options, callback)`

### `win.loadUrl(url[, options])`

Same as `webContents.loadUrl(url[, options])`.

### `win.reload()`

Same as `webContents.reload`.

### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

Sets the `menu` as the window's menu bar, setting it to `null` will remove the
menu bar.

### `win.setProgressBar(progress)`

* `progress` Double

Sets progress value in progress bar. Valid range is [0, 1.0].

Remove progress bar when progress < 0;
Change to indeterminate mode when progress > 1.

On Linux platform, only supports Unity desktop environment, you need to specify
the `*.desktop` file name to `desktopName` field in `package.json`. By default,
it will assume `app.getName().desktop`.

### `win.setOverlayIcon(overlay, description)` _Windows 7+_

* `overlay` [NativeImage](native-image.md) - the icon to display on the bottom
right corner of the taskbar icon. If this parameter is `null`, the overlay is
cleared
* `description` String - a description that will be provided to Accessibility
screen readers

Sets a 16px overlay onto the current taskbar icon, usually used to convey some
sort of application status or to passively notify the user.


### `win.setThumbarButtons(buttons)` _Windows 7+_

`buttons` Array of `button` Objects:

`button` Object, properties:

* `icon` [NativeImage](native-image.md) - The icon showing in thumbnail
  toolbar.
* `tooltip` String (optional) - The text of the button's tooltip.
* `flags` Array (optional) - Control specific states and behaviors
  of the button. By default, it uses `enabled`. It can include following
  Strings:
  * `enabled` - The button is active and available to the user.
  * `disabled` - The button is disabled. It is present, but has a visual
    state indicating it will not respond to user action.
  * `dismissonclick` - When the button is clicked, the taskbar button's
    flyout closes immediately.
  * `nobackground` - Do not draw a button border, use only the image.
  * `hidden` - The button is not shown to the user.
  * `noninteractive` - The button is enabled but not interactive; no
    pressed button state is drawn. This value is intended for instances
    where the button is used in a notification.
* `click` - Function

Add a thumbnail toolbar with a specified set of buttons to the thumbnail image
of a window in a taskbar button layout. Returns a `Boolean` object indicates
whether the thumbnail has been added successfully.

The number of buttons in thumbnail toolbar should be no greater than 7 due to
the limited room. Once you setup the thumbnail toolbar, the toolbar cannot be
removed due to the platform's limitation. But you can call the API with an empty
array to clean the buttons.

### `win.showDefinitionForSelection()` _OS X_

Shows pop-up dictionary that searches the selected word on the page.

### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

Sets whether the window menu bar should hide itself automatically. Once set the
menu bar will only show when users press the single `Alt` key.

If the menu bar is already visible, calling `setAutoHideMenuBar(true)` won't
hide it immediately.

### `win.isMenuBarAutoHide()`

Returns whether menu bar automatically hides itself.

### `win.setMenuBarVisibility(visible)`

* `visible` Boolean

Sets whether the menu bar should be visible. If the menu bar is auto-hide, users
can still bring up the menu bar by pressing the single `Alt` key.

### `win.isMenuBarVisible()`

Returns whether the menu bar is visible.

### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

Sets whether the window should be visible on all workspaces.

**Note:** This API does nothing on Windows.

### `win.isVisibleOnAllWorkspaces()`

Returns whether the window is visible on all workspaces.

**Note:** This API always returns false on Windows.
