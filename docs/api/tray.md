# tray

A `Tray` represents an icon in operating system's notification area, it is
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
    { label: 'Item3', type: 'radio', clicked: true },
    { label: 'Item4', type: 'radio' },
  ]);
  appIcon.setToolTip('This is my application.');
  appIcon.setContextMenu(contextMenu);
});

```

__Platform limitations:__

* On OS X `clicked` event will be ignored if the tray icon has context menu.
* On Linux app indicator will be used if it is supported, otherwise
  `GtkStatusIcon` will be used instead.
* App indicator will only be showed when it has context menu.
* When app indicator is used on Linux, `clicked` event is ignored.

So if you want to keep exact same behaviors on all platforms, you should not
rely on `clicked` event and always attach a context menu to the tray icon.

## Class: Tray

`Tray` is an [EventEmitter][event-emitter].

### new Tray(image)

* `image` [Image](image.md)

Creates a new tray icon associated with the `image`.

### Event: 'clicked'

Emitted when the tray icon is clicked.

### Tray.setImage(image)

* `image` [Image](image.md)

Sets the `image` associated with this tray icon.

### Tray.setPressedImage(image)

* `image` [Image](image.md)

Sets the `image` associated with this tray icon when pressed.

### Tray.setToolTip(toolTip)

* `toolTip` String

### Tray.setContextMenu(menu)

* `menu` Menu

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter
