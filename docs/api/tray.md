# Tray

> Add icons and context menus to the system's notification area.

```javascript
const {app, Menu, Tray} = require('electron');

let appIcon = null;
app.on('ready', () => {
  appIcon = new Tray('/path/to/my/icon');
  const contextMenu = Menu.buildFromTemplate([
    {label: 'Item1', type: 'radio'},
    {label: 'Item2', type: 'radio'},
    {label: 'Item3', type: 'radio', checked: true},
    {label: 'Item4', type: 'radio'}
  ]);
  appIcon.setToolTip('This is my application.');
  appIcon.setContextMenu(contextMenu);
});
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
contextMenu.items[2].checked = false;
appIcon.setContextMenu(contextMenu);
```
* On Windows it is recommended to use `ICO` icons to get best visual effects.

If you want to keep exact same behaviors on all platforms, you should not
rely on the `click` event and always attach a context menu to the tray icon.

## Class: Tray

`Tray` is an [EventEmitter][event-emitter].

### `new Tray(image)`

* `image` [NativeImage](native-image.md)

Creates a new tray icon associated with the `image`.

## Events

The `Tray` module emits the following events:

**Note:** Some events are only available on specific operating systems and are
labeled as such.

### Event: 'click'

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

Emitted when the tray icon is clicked.

**Note:** The `bounds` payload is only implemented on OS X and Windows.

### Event: 'right-click' _OS X_ _Windows_

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

### Event: 'double-click' _OS X_ _Windows_

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

### Event: 'balloon-show' _Windows_

Emitted when the tray balloon shows.

### Event: 'balloon-click' _Windows_

Emitted when the tray balloon is clicked.

### Event: 'balloon-closed' _Windows_

Emitted when the tray balloon is closed because of timeout or user manually
closes it.

### Event: 'drop' _OS X_

Emitted when any dragged items are dropped on the tray icon.

### Event: 'drop-files' _OS X_

* `event`
* `files` Array - the file path of dropped files.

Emitted when dragged files are dropped in the tray icon.

### Event: 'drag-enter' _OS X_

Emitted when a drag operation enters the tray icon.

### Event: 'drag-leave' _OS X_

Emitted when a drag operation exits the tray icon.

### Event: 'drag-end' _OS X_

Emitted when a drag operation ends on the tray or ends at another location.

## Methods

The `Tray` module has the following methods:

**Note:** Some methods are only available on specific operating systems and are
labeled as such.

### `Tray.destroy()`

Destroys the tray icon immediately.

### `Tray.setImage(image)`

* `image` [NativeImage](native-image.md)

Sets the `image` associated with this tray icon.

### `Tray.setPressedImage(image)` _OS X_

* `image` [NativeImage](native-image.md)

Sets the `image` associated with this tray icon when pressed on OS X.

### `Tray.setToolTip(toolTip)`

* `toolTip` String

Sets the hover text for this tray icon.

### `Tray.setTitle(title)` _OS X_

* `title` String

Sets the title displayed aside of the tray icon in the status bar.

### `Tray.setHighlightMode(highlight)` _OS X_

* `highlight` Boolean

Sets whether the tray icon's background becomes highlighted (in blue)
when the tray icon is clicked. Defaults to true.

### `Tray.displayBalloon(options)` _Windows_

* `options` Object
  * `icon` [NativeImage](native-image.md)
  * `title` String
  * `content` String

Displays a tray balloon.

### `Tray.popUpContextMenu([menu, position])` _OS X_ _Windows_

* `menu` Menu (optional)
* `position` Object (optional) - The pop up position.
  * `x` Integer
  * `y` Integer

Pops up the context menu of the tray icon. When `menu` is passed, the `menu` will
be shown instead of the tray icon's context menu.

The `position` is only available on Windows, and it is (0, 0) by default.

### `Tray.setContextMenu(menu)`

* `menu` Menu

Sets the context menu for this icon.

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter
