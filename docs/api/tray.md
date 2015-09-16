# Tray

A `Tray` represents an icon in an operating system's notification area, it is
usually attached with a context menu.

```javascript
var app = require('app');
var Menu = require('menu');
var Tray = require('tray');

var appIcon = null;
app.on('ready', function(){
  appIcon = new Tray('/path/to/my/icon');
  var contextMenu = Menu.buildFromTemplate([
    { label: 'Item1', type: 'radio' },
    { label: 'Item2', type: 'radio' },
    { label: 'Item3', type: 'radio', checked: true },
    { label: 'Item4', type: 'radio' }
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
* When app indicator is used on Linux, the `clicked` event is ignored.

If you want to keep exact same behaviors on all platforms, you should not
rely on the `clicked` event and always attach a context menu to the tray icon.

## Class: Tray

`Tray` is an [EventEmitter][event-emitter].

### `new Tray(image)`

* `image` [NativeImage](native-image.md)

Creates a new tray icon associated with the `image`.

## Events

The `Tray` module emits the following events:

**Note:** Some events are only available on specific operating systems and are
labeled as such.

### Event: 'clicked'

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

__Note:__ The `bounds` payload is only implemented on OS X and Windows.

### Event: 'right-clicked' _OS X_ _Windows_

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

### Event: 'double-clicked' _OS X_ _Windows_

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

### Event: 'balloon-clicked' _Windows_

Emitted when the tray balloon is clicked.

### Event: 'balloon-closed' _Windows_

Emitted when the tray balloon is closed because of timeout or user manually
closes it.

### Event: 'drop-files' _OS X_

* `event`
* `files` Array - the file path of dropped files.

Emitted when dragged files are dropped in the tray icon.

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

Sets whether the tray icon is highlighted when it is clicked.

### `Tray.displayBalloon(options)` _Windows_

* `options` Object
  * `icon` [NativeImage](native-image.md)
  * `title` String
  * `content` String

Displays a tray balloon.

### `Tray.popUpContextMenu([position])` _OS X_ _Windows_

* `position` Object (optional)- The pop up position.
  * `x` Integer
  * `y` Integer

The `position` is only available on Windows, and it is (0, 0) by default.

### `Tray.setContextMenu(menu)`

* `menu` Menu

Sets the context menu for this icon.

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter
