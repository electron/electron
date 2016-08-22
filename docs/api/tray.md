# Tray

> Add icons and context menus to the system's notification area.

```javascript
const {app, Menu, Tray} = require('electron')

let tray = null
app.on('ready', () => {
  tray = new Tray('/path/to/my/icon')
  const contextMenu = Menu.buildFromTemplate([
    {label: 'Item1', type: 'radio'},
    {label: 'Item2', type: 'radio'},
    {label: 'Item3', type: 'radio', checked: true},
    {label: 'Item4', type: 'radio'}
  ])
  tray.setToolTip('This is my application.')
  tray.setContextMenu(contextMenu)
})
```

__Platform limitations:__

* On Linux the app indicator will be used if it is supported, otherwise
  `GtkStatusIcon` will be used instead.
* On Linux distributions that only have app indicator support, you have to
  install `libappindicator1` to make the tray icon work.
* App indicator will only be shown when it has a context menu.
* When app indicator is used on Linux, the `click` event is ignored.
* On Linux in order for changes made to individual `MenuItem`s to take effect,
  you have to call `setContextMenu` again. For example:

```javascript
const {Menu, Tray} = require('electron')
const appIcon = new Tray('/path/to/my/icon')
const contextMenu = Menu.buildFromTemplate([
  {label: 'Item1', type: 'radio'},
  {label: 'Item2', type: 'radio'}
])

// Make a change to the context menu
contextMenu.items[2].checked = false

// Call this again for Linux because we modified the context menu
appIcon.setContextMenu(contextMenu)
```
* On Windows it is recommended to use `ICO` icons to get best visual effects.

If you want to keep exact same behaviors on all platforms, you should not
rely on the `click` event and always attach a context menu to the tray icon.

## Class: Tray

`Tray` is an [EventEmitter][event-emitter].

### `new Tray(image)`

* `image` [NativeImage](native-image.md)

Creates a new tray icon associated with the `image`.

### Instance Events

The `Tray` module emits the following events:

#### Event: 'click'

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object _macOS_ _Windows_ - the bounds of tray icon.
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

Emitted when the tray icon is clicked.

#### Event: 'right-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - the bounds of tray icon.
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

Emitted when the tray icon is right clicked.

#### Event: 'double-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - the bounds of tray icon
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

Emitted when the tray icon is double clicked.

#### Event: 'balloon-show' _Windows_

Emitted when the tray balloon shows.

#### Event: 'balloon-click' _Windows_

Emitted when the tray balloon is clicked.

#### Event: 'balloon-closed' _Windows_

Emitted when the tray balloon is closed because of timeout or user manually
closes it.

#### Event: 'drop' _macOS_

Emitted when any dragged items are dropped on the tray icon.

#### Event: 'drop-files' _macOS_

* `event` Event
* `files` Array - the file path of dropped files.

Emitted when dragged files are dropped in the tray icon.

#### Event: 'drop-text' _macOS_

* `event` Event
* `text` String - the dropped text string

Emitted when dragged text is dropped in the tray icon.

#### Event: 'drag-enter' _macOS_

Emitted when a drag operation enters the tray icon.

#### Event: 'drag-leave' _macOS_

Emitted when a drag operation exits the tray icon.

#### Event: 'drag-end' _macOS_

Emitted when a drag operation ends on the tray or ends at another location.

### Instance Methods

The `Tray` class has the following methods:

#### `tray.destroy()`

Destroys the tray icon immediately.

#### `tray.setImage(image)`

* `image` [NativeImage](native-image.md)

Sets the `image` associated with this tray icon.

#### `tray.setPressedImage(image)` _macOS_

* `image` [NativeImage](native-image.md)

Sets the `image` associated with this tray icon when pressed on macOS.

#### `tray.setToolTip(toolTip)`

* `toolTip` String

Sets the hover text for this tray icon.

#### `tray.setTitle(title)` _macOS_

* `title` String

Sets the title displayed aside of the tray icon in the status bar.

#### `tray.setHighlightMode(mode)` _macOS_

* `mode` String highlight mode with one of the following values:
  * `'selection'` - Highlight the tray icon when it is clicked and also when
    its context menu is open. This is the default.
  * `'always'` - Always highlight the tray icon.
  * `'never'` - Never highlight the tray icon.

Sets when the tray's icon background becomes highlighted (in blue).

**Note:** You can use `highlightMode` with a [`BrowserWindow`](browser-window.md)
by toggling between `'never'` and `'always'` modes when the window visibility
changes.

```javascript
const {BrowserWindow, Tray} = require('electron')

const win = new BrowserWindow({width: 800, height: 600})
const tray = new Tray('/path/to/my/icon')

tray.on('click', () => {
  win.isVisible() ? win.hide() : win.show()
})
win.on('show', () => {
  tray.setHighlightMode('always')
})
win.on('hide', () => {
  tray.setHighlightMode('never')
})
```

#### `tray.displayBalloon(options)` _Windows_

* `options` Object
  * `icon` [NativeImage](native-image.md)
  * `title` String
  * `content` String

Displays a tray balloon.

#### `tray.popUpContextMenu([menu, position])` _macOS_ _Windows_

* `menu` Menu (optional)
* `position` Object (optional) - The pop up position.
  * `x` Integer
  * `y` Integer

Pops up the context menu of the tray icon. When `menu` is passed, the `menu` will
be shown instead of the tray icon's context menu.

The `position` is only available on Windows, and it is (0, 0) by default.

#### `tray.setContextMenu(menu)`

* `menu` Menu

Sets the context menu for this icon.

#### `tray.getBounds()` _macOS_ _Windows_

Returns the `bounds` of this tray icon as `Object`.

* `bounds` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter
