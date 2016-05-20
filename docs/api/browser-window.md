# BrowserWindow

> Create and control browser windows.

```javascript
// In the main process.
const {BrowserWindow} = require('electron');

// Or in the renderer process.
const {BrowserWindow} = require('electron').remote;

let win = new BrowserWindow({width: 800, height: 600, show: false});
win.on('closed', () => {
  win = null;
});

win.loadURL('https://github.com');
win.show();
```

You can also create a window without chrome by using
[Frameless Window](frameless-window.md) API.

## Class: BrowserWindow

`BrowserWindow` is an
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

It creates a new `BrowserWindow` with native properties as set by the `options`.

### `new BrowserWindow([options])`

* `options` Object
  * `width` Integer - Window's width in pixels. Default is `800`.
  * `height` Integer - Window's height in pixels. Default is `600`.
  * `x` Integer - Window's left offset from screen. Default is to center the
    window.
  * `y` Integer - Window's top offset from screen. Default is to center the
    window.
  * `useContentSize` Boolean - The `width` and `height` would be used as web
    page's size, which means the actual window's size will include window
    frame's size and be slightly larger. Default is `false`.
  * `center` Boolean - Show window in the center of the screen.
  * `minWidth` Integer - Window's minimum width. Default is `0`.
  * `minHeight` Integer - Window's minimum height. Default is `0`.
  * `maxWidth` Integer - Window's maximum width. Default is no limit.
  * `maxHeight` Integer - Window's maximum height. Default is no limit.
  * `resizable` Boolean - Whether window is resizable. Default is `true`.
  * `movable` Boolean - Whether window is movable. This is not implemented
    on Linux. Default is `true`.
  * `minimizable` Boolean - Whether window is minimizable. This is not
    implemented on Linux. Default is `true`.
  * `maximizable` Boolean - Whether window is maximizable. This is not
    implemented on Linux. Default is `true`.
  * `closable` Boolean - Whether window is closable. This is not implemented
    on Linux. Default is `true`.
  * `alwaysOnTop` Boolean - Whether the window should always stay on top of
    other windows. Default is `false`.
  * `fullscreen` Boolean - Whether the window should show in fullscreen. When
    explicitly set to `false` the fullscreen button will be hidden or disabled
    on OS X. Default is `false`.
  * `fullscreenable` Boolean - Whether the maximize/zoom button on OS X should
    toggle full screen mode or maximize window. Default is `true`.
  * `skipTaskbar` Boolean - Whether to show the window in taskbar. Default is
    `false`.
  * `kiosk` Boolean - The kiosk mode. Default is `false`.
  * `title` String - Default window title. Default is `"Electron"`.
  * `icon` [NativeImage](native-image.md) - The window icon. On Windows it is
    recommended to use `ICO` icons to get best visual effects, you can also
    leave it undefined so the executable's icon will be used.
  * `show` Boolean - Whether window should be shown when created. Default is
    `true`.
  * `frame` Boolean - Specify `false` to create a
    [Frameless Window](frameless-window.md). Default is `true`.
  * `acceptFirstMouse` Boolean - Whether the web view accepts a single
    mouse-down event that simultaneously activates the window. Default is
    `false`.
  * `disableAutoHideCursor` Boolean - Whether to hide cursor when typing.
    Default is `false`.
  * `autoHideMenuBar` Boolean - Auto hide the menu bar unless the `Alt`
    key is pressed. Default is `false`.
  * `enableLargerThanScreen` Boolean - Enable the window to be resized larger
    than screen. Default is `false`.
  * `backgroundColor` String - Window's background color as Hexadecimal value,
    like `#66CD00` or `#FFF` or `#80FFFFFF` (alpha is supported). Default is
    `#FFF` (white).
  * `hasShadow` Boolean - Whether window should have a shadow. This is only
    implemented on OS X. Default is `true`.
  * `darkTheme` Boolean - Forces using dark theme for the window, only works on
    some GTK+3 desktop environments. Default is `false`.
  * `transparent` Boolean - Makes the window [transparent](frameless-window.md).
    Default is `false`.
  * `type` String - The type of window, default is normal window. See more about
    this below.
  * `titleBarStyle` String - The style of window title bar. See more about this
    below.
  * `webPreferences` Object - Settings of web page's features. See more about
    this below.

When setting minimum or maximum window size with `minWidth`/`maxWidth`/
`minHeight`/`maxHeight`, it only constrains the users, it won't prevent you from
passing a size that does not follow size constraints to `setBounds`/`setSize` or
to the constructor of `BrowserWindow`.

The possible values and behaviors of `type` option are platform dependent,
supported values are:

* On Linux, possible types are `desktop`, `dock`, `toolbar`, `splash`,
  `notification`.
* On OS X, possible types are `desktop`, `textured`.
  * The `textured` type adds metal gradient appearance
    (`NSTexturedBackgroundWindowMask`).
  * The `desktop` type places the window at the desktop background window level
    (`kCGDesktopWindowLevel - 1`). Note that desktop window will not receive
    focus, keyboard or mouse events, but you can use `globalShortcut` to receive
    input sparingly.

The `titleBarStyle` option is only supported on OS X 10.10 Yosemite and newer.
Possible values are:

* `default` or not specified, results in the standard gray opaque Mac title
  bar.
* `hidden` results in a hidden title bar and a full size content window, yet
  the title bar still has the standard window controls ("traffic lights") in
  the top left.
* `hidden-inset` results in a hidden title bar with an alternative look
  where the traffic light buttons are slightly more inset from the window edge.

The `webPreferences` option is an object that can have following properties:

* `nodeIntegration` Boolean - Whether node integration is enabled. Default
  is `true`.
* `preload` String - Specifies a script that will be loaded before other
  scripts run in the page. This script will always have access to node APIs
  no matter whether node integration is turned on or off. The value should
  be the absolute file path to the script.
  When node integration is turned off, the preload script can reintroduce
  Node global symbols back to the global scope. See example
  [here](process.md#event-loaded).
* `session` [Session](session.md#class-session) - Sets the session used by the
  page. Instead of passing the Session object directly, you can also choose to
  use the `partition` option instead, which accepts a partition string. When
  both `session` and `partition` are provided, `session` would be preferred.
  Default is the default session.
* `partition` String - Sets the session used by the page according to the
  session's partition string. If `partition` starts with `persist:`, the page
  will use a persistent session available to all pages in the app with the
  same `partition`. if there is no `persist:` prefix, the page will use an
  in-memory session. By assigning the same `partition`, multiple pages can share
  the same session. Default is the default session.
* `zoomFactor` Number - The default zoom factor of the page, `3.0` represents
  `300%`. Default is `1.0`.
* `javascript` Boolean - Enables JavaScript support. Default is `true`.
* `webSecurity` Boolean - When setting `false`, it will disable the
  same-origin policy (Usually using testing websites by people), and set
  `allowDisplayingInsecureContent` and `allowRunningInsecureContent` to
  `true` if these two options are not set by user. Default is `true`.
* `allowDisplayingInsecureContent` Boolean - Allow an https page to display
  content like images from http URLs. Default is `false`.
* `allowRunningInsecureContent` Boolean - Allow a https page to run
  JavaScript, CSS or plugins from http URLs. Default is `false`.
* `images` Boolean - Enables image support. Default is `true`.
* `textAreasAreResizable` Boolean - Make TextArea elements resizable. Default
  is `true`.
* `webgl` Boolean - Enables WebGL support. Default is `true`.
* `webaudio` Boolean - Enables WebAudio support. Default is `true`.
* `plugins` Boolean - Whether plugins should be enabled. Default is `false`.
* `experimentalFeatures` Boolean - Enables Chromium's experimental features.
  Default is `false`.
* `experimentalCanvasFeatures` Boolean - Enables Chromium's experimental
  canvas features. Default is `false`.
* `directWrite` Boolean - Enables DirectWrite font rendering system on
  Windows. Default is `true`.
* `scrollBounce` Boolean - Enables scroll bounce (rubber banding) effect on
  OS X. Default is `false`.
* `blinkFeatures` String - A list of feature strings separated by `,`, like
  `CSSVariables,KeyboardEventKey`. The full list of supported feature strings
  can be found in the [setFeatureEnabledFromString][blink-feature-string]
  function.
* `defaultFontFamily` Object - Sets the default font for the font-family.
  * `standard` String - Defaults to `Times New Roman`.
  * `serif` String - Defaults to `Times New Roman`.
  * `sansSerif` String - Defaults to `Arial`.
  * `monospace` String - Defaults to `Courier New`.
* `defaultFontSize` Integer - Defaults to `16`.
* `defaultMonospaceFontSize` Integer - Defaults to `13`.
* `minimumFontSize` Integer - Defaults to `0`.
* `defaultEncoding` String - Defaults to `ISO-8859-1`.
* `backgroundThrottling` Boolean - Whether to throttle animations and timers
  when the page becomes background. Defaults to `true`.

## Events

The `BrowserWindow` object emits the following events:

**Note:** Some events are only available on specific operating systems and are
labeled as such.

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
window.onbeforeunload = (e) => {
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

### Event: 'show'

Emitted when the window is shown.

### Event: 'hide'

Emitted when the window is hidden.

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

### Event: 'app-command' _Windows_

Returns:

* `event` Event
* `command` String

Emitted when an [App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)
is invoked. These are typically related to keyboard media keys or browser
commands, as well as the "Back" button built into some mice on Windows.

Commands are lowercased with underscores replaced with hyphens and the
`APPCOMMAND_` prefix stripped off.
e.g. `APPCOMMAND_BROWSER_BACKWARD` is emitted as `browser-backward`.

```javascript
someWindow.on('app-command', (e, cmd) => {
  // Navigate the window back when the user hits their mouse back button
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack();
  }
});
```

### Event: 'scroll-touch-begin' _OS X_

Emitted when scroll wheel event phase has begun.

### Event: 'scroll-touch-end' _OS X_

Emitted when scroll wheel event phase has ended.

### Event: 'swipe' _OS X_

Returns:

* `event` Event
* `direction` String

Emitted on 3-finger swipe. Possible directions are `up`, `right`, `down`, `left`.

## Methods

The `BrowserWindow` object has the following methods:

### `BrowserWindow.getAllWindows()`

Returns an array of all opened browser windows.

### `BrowserWindow.getFocusedWindow()`

Returns the window that is focused in this application, otherwise returns `null`.

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
// In this example `win` is our instance
let win = new BrowserWindow({width: 800, height: 600});
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

**Note:** Some methods are only available on specific operating systems and are
labeled as such.

### `win.destroy()`

Force closing the window, the `unload` and `beforeunload` event won't be emitted
for the web page, and `close` event will also not be emitted
for this window, but it guarantees the `closed` event will be emitted.

### `win.close()`

Try to close the window, this has the same effect with user manually clicking
the close button of the window. The web page may cancel the close though, see
the [close event](#event-close).

### `win.focus()`

Focus on the window.

### `win.blur()`

Remove focus on the window.

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
maintaining the aspect ratio.
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

### `win.setBounds(options[, animate])`

* `options` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `animate` Boolean (optional) _OS X_

Resizes and moves the window to `width`, `height`, `x`, `y`.

### `win.getBounds()`

Returns an object that contains window's width, height, x and y values.

### `win.setSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _OS X_

Resizes the window to `width` and `height`.

### `win.getSize()`

Returns an array that contains window's width and height.

### `win.setContentSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _OS X_

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

### `win.setMovable(movable)` _OS X_ _Windows_

* `movable` Boolean

Sets whether the window can be moved by user. On Linux does nothing.

### `win.isMovable()` _OS X_ _Windows_

Returns whether the window can be moved by user. On Linux always returns
`true`.

### `win.setMinimizable(minimizable)` _OS X_ _Windows_

* `minimizable` Boolean

Sets whether the window can be manually minimized by user. On Linux does
nothing.

### `win.isMinimizable()` _OS X_ _Windows_

Returns whether the window can be manually minimized by user. On Linux always
returns `true`.

### `win.setMaximizable(maximizable)` _OS X_ _Windows_

* `maximizable` Boolean

Sets whether the window can be manually maximized by user. On Linux does
nothing.

### `win.isMaximizable()` _OS X_ _Windows_

Returns whether the window can be manually maximized by user. On Linux always
returns `true`.

### `win.setFullScreenable(fullscreenable)`

* `fullscreenable` Boolean

Sets whether the maximize/zoom window button toggles fullscreen mode or
maximizes the window.

### `win.isFullScreenable()`

Returns whether the maximize/zoom window button toggles fullscreen mode or
maximizes the window.

### `win.setClosable(closable)` _OS X_ _Windows_

* `closable` Boolean

Sets whether the window can be manually closed by user. On Linux does nothing.

### `win.isClosable()` _OS X_ _Windows_

Returns whether the window can be manually closed by user. On Linux always
returns `true`.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

Sets whether the window should show always on top of other windows. After
setting this, the window is still a normal window, not a toolbox window which
can not be focused on.

### `win.isAlwaysOnTop()`

Returns whether the window is always on top of other windows.

### `win.center()`

Moves window to the center of the screen.

### `win.setPosition(x, y[, animate])`

* `x` Integer
* `y` Integer
* `animate` Boolean (optional) _OS X_

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

### `win.setSheetOffset(offset)` _OS X_

Changes the attachment point for sheets on Mac OS X. By default, sheets are
attached just below the window frame, but you may want to display them beneath
a HTML-rendered toolbar. For example:

```javascript
let toolbarRect = document.getElementById('toolbar').getBoundingClientRect();
win.setSheetOffset(toolbarRect.height);
```

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

### `win.getNativeWindowHandle()`

Returns the platform-specific handle of the window as `Buffer`.

The native type of the handle is `HWND` on Windows, `NSView*` on OS X, and
`Window` (`unsigned long`) on Linux.

### `win.hookWindowMessage(message, callback)` _Windows_

* `message` Integer
* `callback` Function

Hooks a windows message. The `callback` is called when
the message is received in the WndProc.

### `win.isWindowMessageHooked(message)` _Windows_

* `message` Integer

Returns `true` or `false` depending on whether the message is hooked.

### `win.unhookWindowMessage(message)` _Windows_

* `message` Integer

Unhook the window message.

### `win.unhookAllWindowMessages()` _Windows_

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
bar will become gray when set to `true`.

### `win.isDocumentEdited()` _OS X_

Whether the window's document has been edited.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (optional) - The area of page to be captured
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

### `win.loadURL(url[, options])`

Same as `webContents.loadURL(url[, options])`.

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

Sets a 16 x 16 pixel overlay onto the current taskbar icon, usually used to
convey some sort of application status or to passively notify the user.

### `win.setHasShadow(hasShadow)` _OS X_

* `hasShadow` (Boolean)

Sets whether the window should have a shadow. On Windows and Linux does
nothing.

### `win.hasShadow()` _OS X_

Returns whether the window has a shadow. On Windows and Linux always returns
`true`.

### `win.setThumbarButtons(buttons)` _Windows 7+_

* `buttons` Array

Add a thumbnail toolbar with a specified set of buttons to the thumbnail image
of a window in a taskbar button layout. Returns a `Boolean` object indicates
whether the thumbnail has been added successfully.

The number of buttons in thumbnail toolbar should be no greater than 7 due to
the limited room. Once you setup the thumbnail toolbar, the toolbar cannot be
removed due to the platform's limitation. But you can call the API with an empty
array to clean the buttons.

The `buttons` is an array of `Button` objects:

* `Button` Object
  * `icon` [NativeImage](native-image.md) - The icon showing in thumbnail
    toolbar.
  * `click` Function
  * `tooltip` String (optional) - The text of the button's tooltip.
  * `flags` Array (optional) - Control specific states and behaviors of the
    button. By default, it is `['enabled']`.

The `flags` is an array that can include following `String`s:

* `enabled` - The button is active and available to the user.
* `disabled` - The button is disabled. It is present, but has a visual state
  indicating it will not respond to user action.
* `dismissonclick` - When the button is clicked, the thumbnail window closes
  immediately.
* `nobackground` - Do not draw a button border, use only the image.
* `hidden` - The button is not shown to the user.
* `noninteractive` - The button is enabled but not interactive; no pressed
  button state is drawn. This value is intended for instances where the button
  is used in a notification.

### `win.showDefinitionForSelection()` _OS X_

Shows pop-up dictionary that searches the selected word on the page.

### `win.setIcon(icon)` _Windows_ _Linux_

* `icon` [NativeImage](native-image.md)

Changes window icon.

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

### `win.setIgnoreMouseEvents(ignore)` _OS X_

* `ignore` Boolean

Ignore all moused events that happened in the window.

[blink-feature-string]: https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/blink/platform/RuntimeEnabledFeatures.cpp&sq=package:chromium&type=cs&l=576
