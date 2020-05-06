# BrowserWindow

> Create and control browser windows.

Process: [Main](../glossary.md#main-process)

```javascript
// In the main process.
const { BrowserWindow } = require('electron')

// Or use `remote` from the renderer process.
// const { BrowserWindow } = require('electron').remote

let win = new BrowserWindow({ width: 800, height: 600 })
win.on('closed', () => {
  win = null
})

// Load a remote URL
win.loadURL('https://github.com')

// Or load a local HTML file
win.loadURL(`file://${__dirname}/app/index.html`)
```

## Frameless window

To create a window without chrome, or a transparent window in arbitrary shape,
you can use the [Frameless Window](frameless-window.md) API.

## Showing window gracefully

When loading a page in the window directly, users may see the page load incrementally, which is not a good experience for a native app. To make the window display
without visual flash, there are two solutions for different situations.

## Using `ready-to-show` event

While loading the page, the `ready-to-show` event will be emitted when the renderer
process has rendered the page for the first time if the window has not been shown yet. Showing
the window after this event will have no visual flash:

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow({ show: false })
win.once('ready-to-show', () => {
  win.show()
})
```

This event is usually emitted after the `did-finish-load` event, but for
pages with many remote resources, it may be emitted before the `did-finish-load`
event.

Please note that using this event implies that the renderer will be considered "visible" and
paint even though `show` is false.  This event will never fire if you use `paintWhenInitiallyHidden: false`

## Setting `backgroundColor`

For a complex app, the `ready-to-show` event could be emitted too late, making
the app feel slow. In this case, it is recommended to show the window
immediately, and use a `backgroundColor` close to your app's background:

```javascript
const { BrowserWindow } = require('electron')

let win = new BrowserWindow({ backgroundColor: '#2e2c29' })
win.loadURL('https://github.com')
```

Note that even for apps that use `ready-to-show` event, it is still recommended
to set `backgroundColor` to make app feel more native.

## Parent and child windows

By using `parent` option, you can create child windows:

```javascript
const { BrowserWindow } = require('electron')

let top = new BrowserWindow()
let child = new BrowserWindow({ parent: top })
child.show()
top.show()
```

The `child` window will always show on top of the `top` window.

## Modal windows

A modal window is a child window that disables parent window, to create a modal
window, you have to set both `parent` and `modal` options:

```javascript
const { BrowserWindow } = require('electron')

let child = new BrowserWindow({ parent: top, modal: true, show: false })
child.loadURL('https://github.com')
child.once('ready-to-show', () => {
  child.show()
})
```

## Page visibility

The [Page Visibility API][page-visibility-api] works as follows:

* On all platforms, the visibility state tracks whether the window is
  hidden/minimized or not.
* Additionally, on macOS, the visibility state also tracks the window
  occlusion state. If the window is occluded (i.e. fully covered) by another
  window, the visibility state will be `hidden`. On other platforms, the
  visibility state will be `hidden` only when the window is minimized or
  explicitly hidden with `win.hide()`.
* If a `BrowserWindow` is created with `show: false`, the initial visibility
  state will be `visible` despite the window actually being hidden.
* If `backgroundThrottling` is disabled, the visibility state will remain
  `visible` even if the window is minimized, occluded, or hidden.

It is recommended that you pause expensive operations when the visibility
state is `hidden` in order to minimize power consumption.

## Platform notices

* On macOS modal windows will be displayed as sheets attached to the parent window.
* On macOS the child windows will keep the relative position to parent window
  when parent window moves, while on Windows and Linux child windows will not
  move.
* On Linux the type of modal windows will be changed to `dialog`.
* On Linux many desktop environments do not support hiding a modal window.

## Class: BrowserWindow

> Create and control browser windows.

Process: [Main](../glossary.md#main-process)

`BrowserWindow` is an [EventEmitter][event-emitter].

It creates a new `BrowserWindow` with native properties as set by the `options`.

### `new BrowserWindow([options])`

* `options` Object (optional)
  * `width` Integer (optional) - Window's width in pixels. Default is `800`.
  * `height` Integer (optional) - Window's height in pixels. Default is `600`.
  * `x` Integer (optional) - (**required** if y is used) Window's left offset from screen.
    Default is to center the window.
  * `y` Integer (optional) - (**required** if x is used) Window's top offset from screen.
    Default is to center the window.
  * `useContentSize` Boolean (optional) - The `width` and `height` would be used as web
    page's size, which means the actual window's size will include window
    frame's size and be slightly larger. Default is `false`.
  * `center` Boolean (optional) - Show window in the center of the screen.
  * `minWidth` Integer (optional) - Window's minimum width. Default is `0`.
  * `minHeight` Integer (optional) - Window's minimum height. Default is `0`.
  * `maxWidth` Integer (optional) - Window's maximum width. Default is no limit.
  * `maxHeight` Integer (optional) - Window's maximum height. Default is no limit.
  * `resizable` Boolean (optional) - Whether window is resizable. Default is `true`.
  * `movable` Boolean (optional) - Whether window is movable. This is not implemented
    on Linux. Default is `true`.
  * `minimizable` Boolean (optional) - Whether window is minimizable. This is not
    implemented on Linux. Default is `true`.
  * `maximizable` Boolean (optional) - Whether window is maximizable. This is not
    implemented on Linux. Default is `true`.
  * `closable` Boolean (optional) - Whether window is closable. This is not implemented
    on Linux. Default is `true`.
  * `focusable` Boolean (optional) - Whether the window can be focused. Default is
    `true`. On Windows setting `focusable: false` also implies setting
    `skipTaskbar: true`. On Linux setting `focusable: false` makes the window
    stop interacting with wm, so the window will always stay on top in all
    workspaces.
  * `alwaysOnTop` Boolean (optional) - Whether the window should always stay on top of
    other windows. Default is `false`.
  * `fullscreen` Boolean (optional) - Whether the window should show in fullscreen. When
    explicitly set to `false` the fullscreen button will be hidden or disabled
    on macOS. Default is `false`.
  * `fullscreenable` Boolean (optional) - Whether the window can be put into fullscreen
    mode. On macOS, also whether the maximize/zoom button should toggle full
    screen mode or maximize window. Default is `true`.
  * `simpleFullscreen` Boolean (optional) - Use pre-Lion fullscreen on macOS. Default is `false`.
  * `skipTaskbar` Boolean (optional) - Whether to show the window in taskbar. Default is
    `false`.
  * `kiosk` Boolean (optional) - The kiosk mode. Default is `false`.
  * `title` String (optional) - Default window title. Default is `"Electron"`. If the HTML tag `<title>` is defined in the HTML file loaded by `loadURL()`, this property will be ignored.
  * `icon` ([NativeImage](native-image.md) | String) (optional) - The window icon. On Windows it is
    recommended to use `ICO` icons to get best visual effects, you can also
    leave it undefined so the executable's icon will be used.
  * `show` Boolean (optional) - Whether window should be shown when created. Default is
    `true`.
  * `paintWhenInitiallyHidden` Boolean (optional) - Whether the renderer should be active when `show` is `false` and it has just been created.  In order for `document.visibilityState` to work correctly on first load with `show: false` you should set this to `false`.  Setting this to `false` will cause the `ready-to-show` event to not fire.  Default is `true`.
  * `frame` Boolean (optional) - Specify `false` to create a
    [Frameless Window](frameless-window.md). Default is `true`.
  * `parent` BrowserWindow (optional) - Specify parent window. Default is `null`.
  * `modal` Boolean (optional) - Whether this is a modal window. This only works when the
    window is a child window. Default is `false`.
  * `acceptFirstMouse` Boolean (optional) - Whether the web view accepts a single
    mouse-down event that simultaneously activates the window. Default is
    `false`.
  * `disableAutoHideCursor` Boolean (optional) - Whether to hide cursor when typing.
    Default is `false`.
  * `autoHideMenuBar` Boolean (optional) - Auto hide the menu bar unless the `Alt`
    key is pressed. Default is `false`.
  * `enableLargerThanScreen` Boolean (optional) - Enable the window to be resized larger
    than screen. Only relevant for macOS, as other OSes allow
    larger-than-screen windows by default. Default is `false`.
  * `backgroundColor` String (optional) - Window's background color as a hexadecimal value,
    like `#66CD00` or `#FFF` or `#80FFFFFF` (alpha in #AARRGGBB format is supported if
    `transparent` is set to `true`). Default is `#FFF` (white).
  * `hasShadow` Boolean (optional) - Whether window should have a shadow. This is only
    implemented on macOS. Default is `true`.
  * `opacity` Number (optional) - Set the initial opacity of the window, between 0.0 (fully
    transparent) and 1.0 (fully opaque). This is only implemented on Windows and macOS.
  * `darkTheme` Boolean (optional) - Forces using dark theme for the window, only works on
    some GTK+3 desktop environments. Default is `false`.
  * `transparent` Boolean (optional) - Makes the window [transparent](frameless-window.md#transparent-window).
    Default is `false`. On Windows, does not work unless the window is frameless.
  * `type` String (optional) - The type of window, default is normal window. See more about
    this below.
  * `titleBarStyle` String (optional) - The style of window title bar.
    Default is `default`. Possible values are:
    * `default` - Results in the standard gray opaque Mac title
      bar.
    * `hidden` - Results in a hidden title bar and a full size content window, yet
      the title bar still has the standard window controls ("traffic lights") in
      the top left.
    * `hiddenInset` - Results in a hidden title bar with an alternative look
      where the traffic light buttons are slightly more inset from the window edge.
    * `customButtonsOnHover` Boolean (optional) - Draw custom close,
      and minimize buttons on macOS frameless windows. These buttons will not display
      unless hovered over in the top left of the window. These custom buttons prevent
      issues with mouse events that occur with the standard window toolbar buttons.
      **Note:** This option is currently experimental.
  * `fullscreenWindowTitle` Boolean (optional) - Shows the title in the
    title bar in full screen mode on macOS for all `titleBarStyle` options.
    Default is `false`.
  * `thickFrame` Boolean (optional) - Use `WS_THICKFRAME` style for frameless windows on
    Windows, which adds standard window frame. Setting it to `false` will remove
    window shadow and window animations. Default is `true`.
  * `vibrancy` String (optional) - Add a type of vibrancy effect to the window, only on
    macOS. Can be `appearance-based`, `light`, `dark`, `titlebar`, `selection`,
    `menu`, `popover`, `sidebar`, `medium-light`, `ultra-dark`, `header`, `sheet`, `window`, `hud`, `fullscreen-ui`, `tooltip`, `content`, `under-window`, or `under-page`.  Please note that using `frame: false` in combination with a vibrancy value requires that you use a non-default `titleBarStyle` as well. Also note that `appearance-based`, `light`, `dark`, `medium-light`, and `ultra-dark` have been deprecated and will be removed in an upcoming version of macOS.
  * `zoomToPageWidth` Boolean (optional) - Controls the behavior on macOS when
    option-clicking the green stoplight button on the toolbar or by clicking the
    Window > Zoom menu item. If `true`, the window will grow to the preferred
    width of the web page when zoomed, `false` will cause it to zoom to the
    width of the screen. This will also affect the behavior when calling
    `maximize()` directly. Default is `false`.
  * `tabbingIdentifier` String (optional) - Tab group name, allows opening the
    window as a native tab on macOS 10.12+. Windows with the same tabbing
    identifier will be grouped together. This also adds a native new tab button
    to your window's tab bar and allows your `app` and window to receive the
    `new-window-for-tab` event.
  * `webPreferences` Object (optional) - Settings of web page's features.
    * `devTools` Boolean (optional) - Whether to enable DevTools. If it is set to `false`, can not use `BrowserWindow.webContents.openDevTools()` to open DevTools. Default is `true`.
    * `nodeIntegration` Boolean (optional) - Whether node integration is enabled.
      Default is `false`.
    * `nodeIntegrationInWorker` Boolean (optional) - Whether node integration is
      enabled in web workers. Default is `false`. More about this can be found
      in [Multithreading](../tutorial/multithreading.md).
    * `nodeIntegrationInSubFrames` Boolean (optional) - Experimental option for
      enabling Node.js support in sub-frames such as iframes and child windows. All your preloads will load for
      every iframe, you can use `process.isMainFrame` to determine if you are
      in the main frame or not.
    * `preload` String (optional) - Specifies a script that will be loaded before other
      scripts run in the page. This script will always have access to node APIs
      no matter whether node integration is turned on or off. The value should
      be the absolute file path to the script.
      When node integration is turned off, the preload script can reintroduce
      Node global symbols back to the global scope. See example
      [here](process.md#event-loaded).
    * `sandbox` Boolean (optional) - If set, this will sandbox the renderer
      associated with the window, making it compatible with the Chromium
      OS-level sandbox and disabling the Node.js engine. This is not the same as
      the `nodeIntegration` option and the APIs available to the preload script
      are more limited. Read more about the option [here](sandbox-option.md).
    * `enableRemoteModule` Boolean (optional) - Whether to enable the [`remote`](remote.md) module.
      Default is `true`.
    * `session` [Session](session.md#class-session) (optional) - Sets the session used by the
      page. Instead of passing the Session object directly, you can also choose to
      use the `partition` option instead, which accepts a partition string. When
      both `session` and `partition` are provided, `session` will be preferred.
      Default is the default session.
    * `partition` String (optional) - Sets the session used by the page according to the
      session's partition string. If `partition` starts with `persist:`, the page
      will use a persistent session available to all pages in the app with the
      same `partition`. If there is no `persist:` prefix, the page will use an
      in-memory session. By assigning the same `partition`, multiple pages can share
      the same session. Default is the default session.
    * `affinity` String (optional) - When specified, web pages with the same
      `affinity` will run in the same renderer process. Note that due to reusing
      the renderer process, certain `webPreferences` options will also be shared
      between the web pages even when you specified different values for them,
      including but not limited to `preload`, `sandbox` and `nodeIntegration`.
      So it is suggested to use exact same `webPreferences` for web pages with
      the same `affinity`. _This property is experimental_
    * `zoomFactor` Number (optional) - The default zoom factor of the page, `3.0` represents
      `300%`. Default is `1.0`.
    * `javascript` Boolean (optional) - Enables JavaScript support. Default is `true`.
    * `webSecurity` Boolean (optional) - When `false`, it will disable the
      same-origin policy (usually using testing websites by people), and set
      `allowRunningInsecureContent` to `true` if this options has not been set
      by user. Default is `true`.
    * `allowRunningInsecureContent` Boolean (optional) - Allow an https page to run
      JavaScript, CSS or plugins from http URLs. Default is `false`.
    * `images` Boolean (optional) - Enables image support. Default is `true`.
    * `textAreasAreResizable` Boolean (optional) - Make TextArea elements resizable. Default
      is `true`.
    * `webgl` Boolean (optional) - Enables WebGL support. Default is `true`.
    * `plugins` Boolean (optional) - Whether plugins should be enabled. Default is `false`.
    * `experimentalFeatures` Boolean (optional) - Enables Chromium's experimental features.
      Default is `false`.
    * `scrollBounce` Boolean (optional) - Enables scroll bounce (rubber banding) effect on
      macOS. Default is `false`.
    * `enableBlinkFeatures` String (optional) - A list of feature strings separated by `,`, like
      `CSSVariables,KeyboardEventKey` to enable. The full list of supported feature
      strings can be found in the [RuntimeEnabledFeatures.json5][runtime-enabled-features]
      file.
    * `disableBlinkFeatures` String (optional) - A list of feature strings separated by `,`,
      like `CSSVariables,KeyboardEventKey` to disable. The full list of supported
      feature strings can be found in the
      [RuntimeEnabledFeatures.json5][runtime-enabled-features] file.
    * `defaultFontFamily` Object (optional) - Sets the default font for the font-family.
      * `standard` String (optional) - Defaults to `Times New Roman`.
      * `serif` String (optional) - Defaults to `Times New Roman`.
      * `sansSerif` String (optional) - Defaults to `Arial`.
      * `monospace` String (optional) - Defaults to `Courier New`.
      * `cursive` String (optional) - Defaults to `Script`.
      * `fantasy` String (optional) - Defaults to `Impact`.
    * `defaultFontSize` Integer (optional) - Defaults to `16`.
    * `defaultMonospaceFontSize` Integer (optional) - Defaults to `13`.
    * `minimumFontSize` Integer (optional) - Defaults to `0`.
    * `defaultEncoding` String (optional) - Defaults to `ISO-8859-1`.
    * `backgroundThrottling` Boolean (optional) - Whether to throttle animations and timers
      when the page becomes background. This also affects the
      [Page Visibility API](#page-visibility). Defaults to `true`.
    * `offscreen` Boolean (optional) - Whether to enable offscreen rendering for the browser
      window. Defaults to `false`. See the
      [offscreen rendering tutorial](../tutorial/offscreen-rendering.md) for
      more details.
    * `contextIsolation` Boolean (optional) - Whether to run Electron APIs and
      the specified `preload` script in a separate JavaScript context. Defaults
      to `false`. The context that the `preload` script runs in will still
      have full access to the `document` and `window` globals but it will use
      its own set of JavaScript builtins (`Array`, `Object`, `JSON`, etc.)
      and will be isolated from any changes made to the global environment
      by the loaded page. The Electron API will only be available in the
      `preload` script and not the loaded page. This option should be used when
      loading potentially untrusted remote content to ensure the loaded content
      cannot tamper with the `preload` script and any Electron APIs being used.
      This option uses the same technique used by [Chrome Content Scripts][chrome-content-scripts].
      You can access this context in the dev tools by selecting the
      'Electron Isolated Context' entry in the combo box at the top of the
      Console tab.
    * `nativeWindowOpen` Boolean (optional) - Whether to use native
      `window.open()`. Defaults to `false`. Child windows will always have node
      integration disabled unless `nodeIntegrationInSubFrames` is true. **Note:** This option is currently
      experimental.
    * `webviewTag` Boolean (optional) - Whether to enable the [`<webview>` tag](webview-tag.md).
      Defaults to `false`. **Note:** The
      `preload` script configured for the `<webview>` will have node integration
      enabled when it is executed so you should ensure remote/untrusted content
      is not able to create a `<webview>` tag with a possibly malicious `preload`
      script. You can use the `will-attach-webview` event on [webContents](web-contents.md)
      to strip away the `preload` script and to validate or alter the
      `<webview>`'s initial settings.
    * `additionalArguments` String[] (optional) - A list of strings that will be appended
      to `process.argv` in the renderer process of this app.  Useful for passing small
      bits of data down to renderer process preload scripts.
    * `safeDialogs` Boolean (optional) - Whether to enable browser style
      consecutive dialog protection. Default is `false`.
    * `safeDialogsMessage` String (optional) - The message to display when
      consecutive dialog protection is triggered. If not defined the default
      message would be used, note that currently the default message is in
      English and not localized.
    * `navigateOnDragDrop` Boolean (optional) - Whether dragging and dropping a
      file or link onto the page causes a navigation. Default is `false`.
    * `autoplayPolicy` String (optional) - Autoplay policy to apply to
      content in the window, can be `no-user-gesture-required`,
      `user-gesture-required`, `document-user-activation-required`. Defaults to
      `no-user-gesture-required`.
    * `disableHtmlFullscreenWindowResize` Boolean (optional) - Whether to
      prevent the window from resizing when entering HTML Fullscreen. Default
      is `false`.
    * `enableWebSQL` Boolean (optional) - Whether to enable the [WebSQL api](https://www.w3.org/TR/webdatabase/).
      Default is `true`.

When setting minimum or maximum window size with `minWidth`/`maxWidth`/
`minHeight`/`maxHeight`, it only constrains the users. It won't prevent you from
passing a size that does not follow size constraints to `setBounds`/`setSize` or
to the constructor of `BrowserWindow`.

The possible values and behaviors of the `type` option are platform dependent.
Possible values are:

* On Linux, possible types are `desktop`, `dock`, `toolbar`, `splash`,
  `notification`.
* On macOS, possible types are `desktop`, `textured`.
  * The `textured` type adds metal gradient appearance
    (`NSTexturedBackgroundWindowMask`).
  * The `desktop` type places the window at the desktop background window level
    (`kCGDesktopWindowLevel - 1`). Note that desktop window will not receive
    focus, keyboard or mouse events, but you can use `globalShortcut` to receive
    input sparingly.
* On Windows, possible type is `toolbar`.

### Instance Events

Objects created with `new BrowserWindow` emit the following events:

**Note:** Some events are only available on specific operating systems and are
labeled as such.

#### Event: 'page-title-updated'

Returns:

* `event` Event
* `title` String
* `explicitSet` Boolean

Emitted when the document changed its title, calling `event.preventDefault()`
will prevent the native window's title from changing.
`explicitSet` is false when title is synthesized from file URL.

#### Event: 'close'

Returns:

* `event` Event

Emitted when the window is going to be closed. It's emitted before the
`beforeunload` and `unload` event of the DOM. Calling `event.preventDefault()`
will cancel the close.

Usually you would want to use the `beforeunload` handler to decide whether the
window should be closed, which will also be called when the window is
reloaded. In Electron, returning any value other than `undefined` would cancel the
close. For example:

```javascript
window.onbeforeunload = (e) => {
  console.log('I do not want to be closed')

  // Unlike usual browsers that a message box will be prompted to users, returning
  // a non-void value will silently cancel the close.
  // It is recommended to use the dialog API to let the user confirm closing the
  // application.
  e.returnValue = false // equivalent to `return false` but not recommended
}
```
_**Note**: There is a subtle difference between the behaviors of `window.onbeforeunload = handler` and `window.addEventListener('beforeunload', handler)`. It is recommended to always set the `event.returnValue` explicitly, instead of only returning a value, as the former works more consistently within Electron._

#### Event: 'closed'

Emitted when the window is closed. After you have received this event you should
remove the reference to the window and avoid using it any more.

#### Event: 'session-end' _Windows_

Emitted when window session is going to end due to force shutdown or machine restart
or session log off.

#### Event: 'unresponsive'

Emitted when the web page becomes unresponsive.

#### Event: 'responsive'

Emitted when the unresponsive web page becomes responsive again.

#### Event: 'blur'

Emitted when the window loses focus.

#### Event: 'focus'

Emitted when the window gains focus.

#### Event: 'show'

Emitted when the window is shown.

#### Event: 'hide'

Emitted when the window is hidden.

#### Event: 'ready-to-show'

Emitted when the web page has been rendered (while not being shown) and window can be displayed without
a visual flash.

Please note that using this event implies that the renderer will be considered "visible" and
paint even though `show` is false.  This event will never fire if you use `paintWhenInitiallyHidden: false`

#### Event: 'maximize'

Emitted when window is maximized.

#### Event: 'unmaximize'

Emitted when the window exits from a maximized state.

#### Event: 'minimize'

Emitted when the window is minimized.

#### Event: 'restore'

Emitted when the window is restored from a minimized state.

#### Event: 'will-resize' _macOS_ _Windows_

Returns:

* `event` Event
* `newBounds` [Rectangle](structures/rectangle.md) - Size the window is being resized to.

Emitted before the window is resized. Calling `event.preventDefault()` will prevent the window from being resized.

Note that this is only emitted when the window is being resized manually. Resizing the window with `setBounds`/`setSize` will not emit this event.

#### Event: 'resize'

Emitted after the window has been resized.

#### Event: 'will-move' _Windows_

Returns:

* `event` Event
* `newBounds` [Rectangle](structures/rectangle.md) - Location the window is being moved to.

Emitted before the window is moved. Calling `event.preventDefault()` will prevent the window from being moved.

Note that this is only emitted when the window is being resized manually. Resizing the window with `setBounds`/`setSize` will not emit this event.

#### Event: 'move'

Emitted when the window is being moved to a new position.

__Note__: On macOS this event is an alias of `moved`.

#### Event: 'moved' _macOS_

Emitted once when the window is moved to a new position.

#### Event: 'enter-full-screen'

Emitted when the window enters a full-screen state.

#### Event: 'leave-full-screen'

Emitted when the window leaves a full-screen state.

#### Event: 'enter-html-full-screen'

Emitted when the window enters a full-screen state triggered by HTML API.

#### Event: 'leave-html-full-screen'

Emitted when the window leaves a full-screen state triggered by HTML API.

#### Event: 'always-on-top-changed'

Returns:

* `event` Event
* `isAlwaysOnTop` Boolean

Emitted when the window is set or unset to show always on top of other windows.

#### Event: 'app-command' _Windows_ _Linux_

Returns:

* `event` Event
* `command` String

Emitted when an [App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)
is invoked. These are typically related to keyboard media keys or browser
commands, as well as the "Back" button built into some mice on Windows.

Commands are lowercased, underscores are replaced with hyphens, and the
`APPCOMMAND_` prefix is stripped off.
e.g. `APPCOMMAND_BROWSER_BACKWARD` is emitted as `browser-backward`.

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow()
win.on('app-command', (e, cmd) => {
  // Navigate the window back when the user hits their mouse back button
  if (cmd === 'browser-backward' && win.webContents.canGoBack()) {
    win.webContents.goBack()
  }
})
```

The following app commands are explicitly supported on Linux:

* `browser-backward`
* `browser-forward`

#### Event: 'scroll-touch-begin' _macOS_

Emitted when scroll wheel event phase has begun.

#### Event: 'scroll-touch-end' _macOS_

Emitted when scroll wheel event phase has ended.

#### Event: 'scroll-touch-edge' _macOS_

Emitted when scroll wheel event phase filed upon reaching the edge of element.

#### Event: 'swipe' _macOS_

Returns:

* `event` Event
* `direction` String

Emitted on 3-finger swipe. Possible directions are `up`, `right`, `down`, `left`.

The method underlying this event is built to handle older macOS-style trackpad swiping,
where the content on the screen doesn't move with the swipe. Most macOS trackpads are not
configured to allow this kind of swiping anymore, so in order for it to emit properly the
'Swipe between pages' preference in `System Preferences > Trackpad > More Gestures` must be
set to 'Swipe with two or three fingers'.

#### Event: 'rotate-gesture' _macOS_

Returns:

* `event` Event
* `rotation` Float

Emitted on trackpad rotation gesture. Continually emitted until rotation gesture is
ended. The `rotation` value on each emission is the angle in degrees rotated since
the last emission. The last emitted event upon a rotation gesture will always be of
value `0`. Counter-clockwise rotation values are positive, while clockwise ones are
negative.

#### Event: 'sheet-begin' _macOS_

Emitted when the window opens a sheet.

#### Event: 'sheet-end' _macOS_

Emitted when the window has closed a sheet.

#### Event: 'new-window-for-tab' _macOS_

Emitted when the native new tab button is clicked.

### Static Methods

The `BrowserWindow` class has the following static methods:

#### `BrowserWindow.getAllWindows()`

Returns `BrowserWindow[]` - An array of all opened browser windows.

#### `BrowserWindow.getFocusedWindow()`

Returns `BrowserWindow | null` - The window that is focused in this application, otherwise returns `null`.

#### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](web-contents.md)

Returns `BrowserWindow` - The window that owns the given `webContents`.

#### `BrowserWindow.fromBrowserView(browserView)`

* `browserView` [BrowserView](browser-view.md)

Returns `BrowserWindow | null` - The window that owns the given `browserView`. If the given view is not attached to any window, returns `null`.

#### `BrowserWindow.fromId(id)`

* `id` Integer

Returns `BrowserWindow` - The window with the given `id`.

#### `BrowserWindow.addExtension(path)`

* `path` String

Adds Chrome extension located at `path`, and returns extension's name.

The method will also not return if the extension's manifest is missing or incomplete.

**Note:** This API cannot be called before the `ready` event of the `app` module
is emitted.

#### `BrowserWindow.removeExtension(name)`

* `name` String

Remove a Chrome extension by name.

**Note:** This API cannot be called before the `ready` event of the `app` module
is emitted.

#### `BrowserWindow.getExtensions()`

Returns `Record<String, ExtensionInfo>` - The keys are the extension names and each value is
an Object containing `name` and `version` properties.

**Note:** This API cannot be called before the `ready` event of the `app` module
is emitted.

#### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

Adds DevTools extension located at `path`, and returns extension's name.

The extension will be remembered so you only need to call this API once, this
API is not for programming use. If you try to add an extension that has already
been loaded, this method will not return and instead log a warning to the
console.

The method will also not return if the extension's manifest is missing or incomplete.

**Note:** This API cannot be called before the `ready` event of the `app` module
is emitted.

#### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

Remove a DevTools extension by name.

**Note:** This API cannot be called before the `ready` event of the `app` module
is emitted.

#### `BrowserWindow.getDevToolsExtensions()`

Returns `Record<string, ExtensionInfo>` - The keys are the extension names and each value is
an Object containing `name` and `version` properties.

To check if a DevTools extension is installed you can run the following:

```javascript
const { BrowserWindow } = require('electron')

let installed = BrowserWindow.getDevToolsExtensions().hasOwnProperty('devtron')
console.log(installed)
```

**Note:** This API cannot be called before the `ready` event of the `app` module
is emitted.

### Instance Properties

Objects created with `new BrowserWindow` have the following properties:

```javascript
const { BrowserWindow } = require('electron')
// In this example `win` is our instance
let win = new BrowserWindow({ width: 800, height: 600 })
win.loadURL('https://github.com')
```

#### `win.webContents` _Readonly_

A `WebContents` object this window owns. All web page related events and
operations will be done via it.

See the [`webContents` documentation](web-contents.md) for its methods and
events.

#### `win.id` _Readonly_

A `Integer` property representing the unique ID of the window.

#### `win.autoHideMenuBar`

A `Boolean` property that determines whether the window menu bar should hide itself automatically. Once set, the menu bar will only show when users press the single `Alt` key.

If the menu bar is already visible, setting this property to `true` won't
hide it immediately.

#### `win.minimizable`

A `Boolean` property that determines whether the window can be manually minimized by user.

On Linux the setter is a no-op, although the getter returns `true`.

#### `win.maximizable`

A `Boolean` property that determines whether the window can be manually maximized by user.

On Linux the setter is a no-op, although the getter returns `true`.

#### `win.fullScreenable`

A `Boolean` property that determines whether the maximize/zoom window button toggles fullscreen mode or
maximizes the window.

#### `win.resizable`

A `Boolean` property that determines whether the window can be manually resized by user.

#### `win.closable`

A `Boolean` property that determines whether the window can be manually closed by user.

On Linux the setter is a no-op, although the getter returns `true`.

#### `win.movable`

A `Boolean` property that determines Whether the window can be moved by user.

On Linux the setter is a no-op, although the getter returns `true`.

#### `win.excludedFromShownWindowsMenu` _macOS_

A `Boolean` property that determines whether the window is excluded from the application’s Windows menu. `false` by default.

```js
const win = new BrowserWindow({ height: 600, width: 600 })

const template = [
  {
    role: 'windowmenu'
  }
]

win.excludedFromShownWindowsMenu = true

const menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)
```

### Instance Methods

Objects created with `new BrowserWindow` have the following instance methods:

**Note:** Some methods are only available on specific operating systems and are
labeled as such.

#### `win.destroy()`

Force closing the window, the `unload` and `beforeunload` event won't be emitted
for the web page, and `close` event will also not be emitted
for this window, but it guarantees the `closed` event will be emitted.

#### `win.close()`

Try to close the window. This has the same effect as a user manually clicking
the close button of the window. The web page may cancel the close though. See
the [close event](#event-close).

#### `win.focus()`

Focuses on the window.

#### `win.blur()`

Removes focus from the window.

#### `win.isFocused()`

Returns `Boolean` - Whether the window is focused.

#### `win.isDestroyed()`

Returns `Boolean` - Whether the window is destroyed.

#### `win.show()`

Shows and gives focus to the window.

#### `win.showInactive()`

Shows the window but doesn't focus on it.

#### `win.hide()`

Hides the window.

#### `win.isVisible()`

Returns `Boolean` - Whether the window is visible to the user.

#### `win.isModal()`

Returns `Boolean` - Whether current window is a modal window.

#### `win.maximize()`

Maximizes the window. This will also show (but not focus) the window if it
isn't being displayed already.

#### `win.unmaximize()`

Unmaximizes the window.

#### `win.isMaximized()`

Returns `Boolean` - Whether the window is maximized.

#### `win.minimize()`

Minimizes the window. On some platforms the minimized window will be shown in
the Dock.

#### `win.restore()`

Restores the window from minimized state to its previous state.

#### `win.isMinimized()`

Returns `Boolean` - Whether the window is minimized.

#### `win.setFullScreen(flag)`

* `flag` Boolean

Sets whether the window should be in fullscreen mode.

#### `win.isFullScreen()`

Returns `Boolean` - Whether the window is in fullscreen mode.

#### `win.setSimpleFullScreen(flag)` _macOS_

* `flag` Boolean

Enters or leaves simple fullscreen mode.

Simple fullscreen mode emulates the native fullscreen behavior found in versions of Mac OS X prior to Lion (10.7).

#### `win.isSimpleFullScreen()` _macOS_

Returns `Boolean` - Whether the window is in simple (pre-Lion) fullscreen mode.

#### `win.isNormal()`

Returns `Boolean` - Whether the window is in normal state (not maximized, not minimized, not in fullscreen mode).

#### `win.setAspectRatio(aspectRatio[, extraSize])` _macOS_

* `aspectRatio` Float - The aspect ratio to maintain for some portion of the
content view.
* `extraSize` [Size](structures/size.md) (optional) - The extra size not to be included while
maintaining the aspect ratio.

This will make a window maintain an aspect ratio. The extra size allows a
developer to have space, specified in pixels, not included within the aspect
ratio calculations. This API already takes into account the difference between a
window's size and its content size.

Consider a normal window with an HD video player and associated controls.
Perhaps there are 15 pixels of controls on the left edge, 25 pixels of controls
on the right edge and 50 pixels of controls below the player. In order to
maintain a 16:9 aspect ratio (standard aspect ratio for HD @1920x1080) within
the player itself we would call this function with arguments of 16/9 and
[ 40, 50 ]. The second argument doesn't care where the extra width and height
are within the content view--only that they exist. Sum any extra width and
height areas you have within the overall content view.

Calling this function with a value of `0` will remove any previously set aspect
ratios.

#### `win.setBackgroundColor(backgroundColor)`

* `backgroundColor` String - Window's background color as a hexadecimal value,
  like `#66CD00` or `#FFF` or `#80FFFFFF` (alpha is supported if `transparent`
  is `true`). Default is `#FFF` (white).

Sets the background color of the window. See [Setting
`backgroundColor`](#setting-backgroundcolor).

#### `win.previewFile(path[, displayName])` _macOS_

* `path` String - The absolute path to the file to preview with QuickLook. This
  is important as Quick Look uses the file name and file extension on the path
  to determine the content type of the file to open.
* `displayName` String (optional) - The name of the file to display on the
  Quick Look modal view. This is purely visual and does not affect the content
  type of the file. Defaults to `path`.

Uses [Quick Look][quick-look] to preview a file at a given path.

#### `win.closeFilePreview()` _macOS_

Closes the currently open [Quick Look][quick-look] panel.

#### `win.setBounds(bounds[, animate])`

* `bounds` Partial<[Rectangle](structures/rectangle.md)>
* `animate` Boolean (optional) _macOS_

Resizes and moves the window to the supplied bounds. Any properties that are not supplied will default to their current values.

```javascript
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()

// set all bounds properties
win.setBounds({ x: 440, y: 225, width: 800, height: 600 })

// set a single bounds property
win.setBounds({ width: 100 })

// { x: 440, y: 225, width: 100, height: 600 }
console.log(win.getBounds())
```

#### `win.getBounds()`

Returns [`Rectangle`](structures/rectangle.md) - The `bounds` of the window as `Object`.

#### `win.setContentBounds(bounds[, animate])`

* `bounds` [Rectangle](structures/rectangle.md)
* `animate` Boolean (optional) _macOS_

Resizes and moves the window's client area (e.g. the web page) to
the supplied bounds.

#### `win.getContentBounds()`

Returns [`Rectangle`](structures/rectangle.md) - The `bounds` of the window's client area as `Object`.

#### `win.getNormalBounds()`

Returns [`Rectangle`](structures/rectangle.md) - Contains the window bounds of the normal state

**Note:** whatever the current state of the window : maximized, minimized or in fullscreen, this function always returns the position and size of the window in normal state. In normal state, getBounds and getNormalBounds returns the same [`Rectangle`](structures/rectangle.md).

#### `win.setEnabled(enable)`

* `enable` Boolean

Disable or enable the window.

#### `win.isEnabled()`

Returns Boolean - whether the window is enabled.

#### `win.setSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _macOS_

Resizes the window to `width` and `height`. If `width` or `height` are below any set minimum size constraints the window will snap to its minimum size.

#### `win.getSize()`

Returns `Integer[]` - Contains the window's width and height.

#### `win.setContentSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _macOS_

Resizes the window's client area (e.g. the web page) to `width` and `height`.

#### `win.getContentSize()`

Returns `Integer[]` - Contains the window's client area's width and height.

#### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

Sets the minimum size of window to `width` and `height`.

#### `win.getMinimumSize()`

Returns `Integer[]` - Contains the window's minimum width and height.

#### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

Sets the maximum size of window to `width` and `height`.

#### `win.getMaximumSize()`

Returns `Integer[]` - Contains the window's maximum width and height.

#### `win.setResizable(resizable)`

* `resizable` Boolean

Sets whether the window can be manually resized by user.

**[Deprecated](modernization/property-updates.md)**

#### `win.isResizable()`

Returns `Boolean` - Whether the window can be manually resized by user.

**[Deprecated](modernization/property-updates.md)**

#### `win.setMovable(movable)` _macOS_ _Windows_

* `movable` Boolean

Sets whether the window can be moved by user. On Linux does nothing.

**[Deprecated](modernization/property-updates.md)**

#### `win.isMovable()` _macOS_ _Windows_

Returns `Boolean` - Whether the window can be moved by user.

On Linux always returns `true`.

**[Deprecated](modernization/property-updates.md)**

#### `win.setMinimizable(minimizable)` _macOS_ _Windows_

* `minimizable` Boolean

Sets whether the window can be manually minimized by user. On Linux does
nothing.

**[Deprecated](modernization/property-updates.md)**

#### `win.isMinimizable()` _macOS_ _Windows_

Returns `Boolean` - Whether the window can be manually minimized by user

On Linux always returns `true`.

**[Deprecated](modernization/property-updates.md)**

#### `win.setMaximizable(maximizable)` _macOS_ _Windows_

* `maximizable` Boolean

Sets whether the window can be manually maximized by user. On Linux does
nothing.

**[Deprecated](modernization/property-updates.md)**

#### `win.isMaximizable()` _macOS_ _Windows_

Returns `Boolean` - Whether the window can be manually maximized by user.

On Linux always returns `true`.

**[Deprecated](modernization/property-updates.md)**

#### `win.setFullScreenable(fullscreenable)`

* `fullscreenable` Boolean

Sets whether the maximize/zoom window button toggles fullscreen mode or
maximizes the window.

**[Deprecated](modernization/property-updates.md)**

#### `win.isFullScreenable()`

Returns `Boolean` - Whether the maximize/zoom window button toggles fullscreen mode or
maximizes the window.

**[Deprecated](modernization/property-updates.md)**

#### `win.setClosable(closable)` _macOS_ _Windows_

* `closable` Boolean

Sets whether the window can be manually closed by user. On Linux does nothing.

**[Deprecated](modernization/property-updates.md)**

#### `win.isClosable()` _macOS_ _Windows_

Returns `Boolean` - Whether the window can be manually closed by user.

On Linux always returns `true`.

**[Deprecated](modernization/property-updates.md)**

#### `win.setAlwaysOnTop(flag[, level][, relativeLevel])`

* `flag` Boolean
* `level` String (optional) _macOS_ _Windows_ - Values include `normal`,
  `floating`, `torn-off-menu`, `modal-panel`, `main-menu`, `status`,
  `pop-up-menu`, `screen-saver`, and ~~`dock`~~ (Deprecated). The default is
  `floating` when `flag` is true. The `level` is reset to `normal` when the
  flag is false. Note that from `floating` to `status` included, the window is
  placed below the Dock on macOS and below the taskbar on Windows. From
  `pop-up-menu` to a higher it is shown above the Dock on macOS and above the
  taskbar on Windows. See the [macOS docs][window-levels] for more details.
* `relativeLevel` Integer (optional) _macOS_ - The number of layers higher to set
  this window relative to the given `level`. The default is `0`. Note that Apple
  discourages setting levels higher than 1 above `screen-saver`.

Sets whether the window should show always on top of other windows. After
setting this, the window is still a normal window, not a toolbox window which
can not be focused on.

#### `win.isAlwaysOnTop()`

Returns `Boolean` - Whether the window is always on top of other windows.

#### `win.moveTop()`

Moves window to top(z-order) regardless of focus

#### `win.center()`

Moves window to the center of the screen.

#### `win.setPosition(x, y[, animate])`

* `x` Integer
* `y` Integer
* `animate` Boolean (optional) _macOS_

Moves window to `x` and `y`.

#### `win.getPosition()`

Returns `Integer[]` - Contains the window's current position.

#### `win.setTitle(title)`

* `title` String

Changes the title of native window to `title`.

#### `win.getTitle()`

Returns `String` - The title of the native window.

**Note:** The title of the web page can be different from the title of the native
window.

#### `win.setSheetOffset(offsetY[, offsetX])` _macOS_

* `offsetY` Float
* `offsetX` Float (optional)

Changes the attachment point for sheets on macOS. By default, sheets are
attached just below the window frame, but you may want to display them beneath
a HTML-rendered toolbar. For example:

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow()

let toolbarRect = document.getElementById('toolbar').getBoundingClientRect()
win.setSheetOffset(toolbarRect.height)
```

#### `win.flashFrame(flag)`

* `flag` Boolean

Starts or stops flashing the window to attract user's attention.

#### `win.setSkipTaskbar(skip)`

* `skip` Boolean

Makes the window not show in the taskbar.

#### `win.setKiosk(flag)`

* `flag` Boolean

Enters or leaves the kiosk mode.

#### `win.isKiosk()`

Returns `Boolean` - Whether the window is in kiosk mode.

#### `win.getNativeWindowHandle()`

Returns `Buffer` - The platform-specific handle of the window.

The native type of the handle is `HWND` on Windows, `NSView*` on macOS, and
`Window` (`unsigned long`) on Linux.

#### `win.hookWindowMessage(message, callback)` _Windows_

* `message` Integer
* `callback` Function

Hooks a windows message. The `callback` is called when
the message is received in the WndProc.

#### `win.isWindowMessageHooked(message)` _Windows_

* `message` Integer

Returns `Boolean` - `true` or `false` depending on whether the message is hooked.

#### `win.unhookWindowMessage(message)` _Windows_

* `message` Integer

Unhook the window message.

#### `win.unhookAllWindowMessages()` _Windows_

Unhooks all of the window messages.

#### `win.setRepresentedFilename(filename)` _macOS_

* `filename` String

Sets the pathname of the file the window represents, and the icon of the file
will show in window's title bar.

#### `win.getRepresentedFilename()` _macOS_

Returns `String` - The pathname of the file the window represents.

#### `win.setDocumentEdited(edited)` _macOS_

* `edited` Boolean

Specifies whether the window’s document has been edited, and the icon in title
bar will become gray when set to `true`.

#### `win.isDocumentEdited()` _macOS_

Returns `Boolean` - Whether the window's document has been edited.

#### `win.focusOnWebView()`

#### `win.blurWebView()`

#### `win.capturePage([rect])`

* `rect` [Rectangle](structures/rectangle.md) (optional) - The bounds to capture

Returns `Promise<NativeImage>` - Resolves with a [NativeImage](native-image.md)

Captures a snapshot of the page within `rect`. Omitting `rect` will capture the whole visible page.

#### `win.loadURL(url[, options])`

* `url` String
* `options` Object (optional)
  * `httpReferrer` (String | [Referrer](structures/referrer.md)) (optional) - An HTTP Referrer URL.
  * `userAgent` String (optional) - A user agent originating the request.
  * `extraHeaders` String (optional) - Extra headers separated by "\n"
  * `postData` ([UploadRawData[]](structures/upload-raw-data.md) | [UploadFile[]](structures/upload-file.md) | [UploadBlob[]](structures/upload-blob.md)) (optional)
  * `baseURLForDataURL` String (optional) - Base URL (with trailing path separator) for files to be loaded by the data URL. This is needed only if the specified `url` is a data URL and needs to load other files.

Returns `Promise<void>` - the promise will resolve when the page has finished loading
(see [`did-finish-load`](web-contents.md#event-did-finish-load)), and rejects
if the page fails to load (see [`did-fail-load`](web-contents.md#event-did-fail-load)).

Same as [`webContents.loadURL(url[, options])`](web-contents.md#contentsloadurlurl-options).

The `url` can be a remote address (e.g. `http://`) or a path to a local
HTML file using the `file://` protocol.

To ensure that file URLs are properly formatted, it is recommended to use
Node's [`url.format`](https://nodejs.org/api/url.html#url_url_format_urlobject)
method:

```javascript
let url = require('url').format({
  protocol: 'file',
  slashes: true,
  pathname: require('path').join(__dirname, 'index.html')
})

win.loadURL(url)
```

You can load a URL using a `POST` request with URL-encoded data by doing
the following:

```javascript
win.loadURL('http://localhost:8000/post', {
  postData: [{
    type: 'rawData',
    bytes: Buffer.from('hello=world')
  }],
  extraHeaders: 'Content-Type: application/x-www-form-urlencoded'
})
```

#### `win.loadFile(filePath[, options])`

* `filePath` String
* `options` Object (optional)
  * `query` Record<String, String> (optional) - Passed to `url.format()`.
  * `search` String (optional) - Passed to `url.format()`.
  * `hash` String (optional) - Passed to `url.format()`.

Returns `Promise<void>` - the promise will resolve when the page has finished loading
(see [`did-finish-load`](web-contents.md#event-did-finish-load)), and rejects
if the page fails to load (see [`did-fail-load`](web-contents.md#event-did-fail-load)).

Same as `webContents.loadFile`, `filePath` should be a path to an HTML
file relative to the root of your application.  See the `webContents` docs
for more information.

#### `win.reload()`

Same as `webContents.reload`.

#### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu | null

Sets the `menu` as the window's menu bar.

#### `win.removeMenu()` _Linux_ _Windows_

Remove the window's menu bar.

#### `win.setProgressBar(progress[, options])`

* `progress` Double
* `options` Object (optional)
  * `mode` String _Windows_ - Mode for the progress bar. Can be `none`, `normal`, `indeterminate`, `error` or `paused`.

Sets progress value in progress bar. Valid range is [0, 1.0].

Remove progress bar when progress < 0;
Change to indeterminate mode when progress > 1.

On Linux platform, only supports Unity desktop environment, you need to specify
the `*.desktop` file name to `desktopName` field in `package.json`. By default,
it will assume `{app.name}.desktop`.

On Windows, a mode can be passed. Accepted values are `none`, `normal`,
`indeterminate`, `error`, and `paused`. If you call `setProgressBar` without a
mode set (but with a value within the valid range), `normal` will be assumed.

#### `win.setOverlayIcon(overlay, description)` _Windows_

* `overlay` [NativeImage](native-image.md) | null - the icon to display on the bottom
right corner of the taskbar icon. If this parameter is `null`, the overlay is
cleared
* `description` String - a description that will be provided to Accessibility
screen readers

Sets a 16 x 16 pixel overlay onto the current taskbar icon, usually used to
convey some sort of application status or to passively notify the user.

#### `win.setHasShadow(hasShadow)`

* `hasShadow` Boolean

Sets whether the window should have a shadow.

#### `win.hasShadow()`

Returns `Boolean` - Whether the window has a shadow.

#### `win.setOpacity(opacity)` _Windows_ _macOS_

* `opacity` Number - between 0.0 (fully transparent) and 1.0 (fully opaque)

Sets the opacity of the window. On Linux, does nothing. Out of bound number
values are clamped to the [0, 1] range.

#### `win.getOpacity()`

Returns `Number` - between 0.0 (fully transparent) and 1.0 (fully opaque). On
Linux, always returns 1.

#### `win.setShape(rects)` _Windows_ _Linux_ _Experimental_

* `rects` [Rectangle[]](structures/rectangle.md) - Sets a shape on the window.
  Passing an empty list reverts the window to being rectangular.

Setting a window shape determines the area within the window where the system
permits drawing and user interaction. Outside of the given region, no pixels
will be drawn and no mouse events will be registered. Mouse events outside of
the region will not be received by that window, but will fall through to
whatever is behind the window.

#### `win.setThumbarButtons(buttons)` _Windows_

* `buttons` [ThumbarButton[]](structures/thumbar-button.md)

Returns `Boolean` - Whether the buttons were added successfully

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
  * `flags` String[] (optional) - Control specific states and behaviors of the
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

#### `win.setThumbnailClip(region)` _Windows_

* `region` [Rectangle](structures/rectangle.md) - Region of the window

Sets the region of the window to show as the thumbnail image displayed when
hovering over the window in the taskbar. You can reset the thumbnail to be
the entire window by specifying an empty region:
`{ x: 0, y: 0, width: 0, height: 0 }`.

#### `win.setThumbnailToolTip(toolTip)` _Windows_

* `toolTip` String

Sets the toolTip that is displayed when hovering over the window thumbnail
in the taskbar.

#### `win.setAppDetails(options)` _Windows_

* `options` Object
  * `appId` String (optional) - Window's [App User Model ID](https://msdn.microsoft.com/en-us/library/windows/desktop/dd391569(v=vs.85).aspx).
    It has to be set, otherwise the other options will have no effect.
  * `appIconPath` String (optional) - Window's [Relaunch Icon](https://msdn.microsoft.com/en-us/library/windows/desktop/dd391573(v=vs.85).aspx).
  * `appIconIndex` Integer (optional) - Index of the icon in `appIconPath`.
    Ignored when `appIconPath` is not set. Default is `0`.
  * `relaunchCommand` String (optional) - Window's [Relaunch Command](https://msdn.microsoft.com/en-us/library/windows/desktop/dd391571(v=vs.85).aspx).
  * `relaunchDisplayName` String (optional) - Window's [Relaunch Display Name](https://msdn.microsoft.com/en-us/library/windows/desktop/dd391572(v=vs.85).aspx).

Sets the properties for the window's taskbar button.

**Note:** `relaunchCommand` and `relaunchDisplayName` must always be set
together. If one of those properties is not set, then neither will be used.

#### `win.showDefinitionForSelection()` _macOS_

Same as `webContents.showDefinitionForSelection()`.

#### `win.setIcon(icon)` _Windows_ _Linux_

* `icon` [NativeImage](native-image.md) | String

Changes window icon.

#### `win.setWindowButtonVisibility(visible)` _macOS_

* `visible` Boolean

Sets whether the window traffic light buttons should be visible.

This cannot be called when `titleBarStyle` is set to `customButtonsOnHover`.

#### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

Sets whether the window menu bar should hide itself automatically. Once set the
menu bar will only show when users press the single `Alt` key.

If the menu bar is already visible, calling `setAutoHideMenuBar(true)` won't
hide it immediately.

**[Deprecated](modernization/property-updates.md)**

#### `win.isMenuBarAutoHide()`

Returns `Boolean` - Whether menu bar automatically hides itself.

**[Deprecated](modernization/property-updates.md)**

#### `win.setMenuBarVisibility(visible)` _Windows_ _Linux_

* `visible` Boolean

Sets whether the menu bar should be visible. If the menu bar is auto-hide, users
can still bring up the menu bar by pressing the single `Alt` key.

#### `win.isMenuBarVisible()`

Returns `Boolean` - Whether the menu bar is visible.

#### `win.setVisibleOnAllWorkspaces(visible[, options])`

* `visible` Boolean
* `options` Object (optional)
  * `visibleOnFullScreen` Boolean (optional) _macOS_ - Sets whether
    the window should be visible above fullscreen windows

Sets whether the window should be visible on all workspaces.

**Note:** This API does nothing on Windows.

#### `win.isVisibleOnAllWorkspaces()`

Returns `Boolean` - Whether the window is visible on all workspaces.

**Note:** This API always returns false on Windows.

#### `win.setIgnoreMouseEvents(ignore[, options])`

* `ignore` Boolean
* `options` Object (optional)
  * `forward` Boolean (optional) _macOS_ _Windows_ - If true, forwards mouse move
    messages to Chromium, enabling mouse related events such as `mouseleave`.
    Only used when `ignore` is true. If `ignore` is false, forwarding is always
    disabled regardless of this value.

Makes the window ignore all mouse events.

All mouse events happened in this window will be passed to the window below
this window, but if this window has focus, it will still receive keyboard
events.

#### `win.setContentProtection(enable)` _macOS_ _Windows_

* `enable` Boolean

Prevents the window contents from being captured by other apps.

On macOS it sets the NSWindow's sharingType to NSWindowSharingNone.
On Windows it calls SetWindowDisplayAffinity with `WDA_MONITOR`.

#### `win.setFocusable(focusable)` _macOS_ _Windows_

* `focusable` Boolean

Changes whether the window can be focused.

On macOS it does not remove the focus from the window.

#### `win.setParentWindow(parent)`

* `parent` BrowserWindow | null

Sets `parent` as current window's parent window, passing `null` will turn
current window into a top-level window.

#### `win.getParentWindow()`

Returns `BrowserWindow` - The parent window.

#### `win.getChildWindows()`

Returns `BrowserWindow[]` - All child windows.

#### `win.setAutoHideCursor(autoHide)` _macOS_

* `autoHide` Boolean

Controls whether to hide cursor when typing.

#### `win.selectPreviousTab()` _macOS_

Selects the previous tab when native tabs are enabled and there are other
tabs in the window.

#### `win.selectNextTab()` _macOS_

Selects the next tab when native tabs are enabled and there are other
tabs in the window.

#### `win.mergeAllWindows()` _macOS_

Merges all windows into one window with multiple tabs when native tabs
are enabled and there is more than one open window.

#### `win.moveTabToNewWindow()` _macOS_

Moves the current tab into a new window if native tabs are enabled and
there is more than one tab in the current window.

#### `win.toggleTabBar()` _macOS_

Toggles the visibility of the tab bar if native tabs are enabled and
there is only one tab in the current window.

#### `win.addTabbedWindow(browserWindow)` _macOS_

* `browserWindow` BrowserWindow

Adds a window as a tab on this window, after the tab for the window instance.

#### `win.setVibrancy(type)` _macOS_

* `type` String | null - Can be `appearance-based`, `light`, `dark`, `titlebar`,
  `selection`, `menu`, `popover`, `sidebar`, `medium-light`, `ultra-dark`, `header`, `sheet`, `window`, `hud`, `fullscreen-ui`, `tooltip`, `content`, `under-window`, or `under-page`. See
  the [macOS documentation][vibrancy-docs] for more details.

Adds a vibrancy effect to the browser window. Passing `null` or an empty string
will remove the vibrancy effect on the window.

Note that `appearance-based`, `light`, `dark`, `medium-light`, and `ultra-dark` have been
deprecated and will be removed in an upcoming version of macOS.

#### `win.setTouchBar(touchBar)` _macOS_ _Experimental_

* `touchBar` TouchBar | null

Sets the touchBar layout for the current window. Specifying `null` or
`undefined` clears the touch bar. This method only has an effect if the
machine has a touch bar and is running on macOS 10.12.1+.

**Note:** The TouchBar API is currently experimental and may change or be
removed in future Electron releases.

#### `win.setBrowserView(browserView)` _Experimental_

* `browserView` [BrowserView](browser-view.md) | null - Attach browserView to win.
If there is some other browserViews was attached they will be removed from
this window.

#### `win.getBrowserView()` _Experimental_

Returns `BrowserView | null` - an BrowserView what is attached. Returns `null`
if none is attached. Throw error if multiple BrowserViews is attached.

#### `win.addBrowserView(browserView)` _Experimental_

* `browserView` [BrowserView](browser-view.md)

Replacement API for setBrowserView supporting work with multi browser views.

#### `win.removeBrowserView(browserView)` _Experimental_

* `browserView` [BrowserView](browser-view.md)

#### `win.getBrowserViews()` _Experimental_

Returns `BrowserView[]` - an array of all BrowserViews that have been attached
with `addBrowserView` or `setBrowserView`.

**Note:** The BrowserView API is currently experimental and may change or be
removed in future Electron releases.

[runtime-enabled-features]: https://cs.chromium.org/chromium/src/third_party/blink/renderer/platform/runtime_enabled_features.json5?l=70
[page-visibility-api]: https://developer.mozilla.org/en-US/docs/Web/API/Page_Visibility_API
[quick-look]: https://en.wikipedia.org/wiki/Quick_Look
[vibrancy-docs]: https://developer.apple.com/documentation/appkit/nsvisualeffectview?preferredLanguage=objc
[window-levels]: https://developer.apple.com/documentation/appkit/nswindow/level
[chrome-content-scripts]: https://developer.chrome.com/extensions/content_scripts#execution-environment
[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
