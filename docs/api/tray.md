# tray

A `Tray` represents an icon in operating system's notification area, it is
usually attached with a context menu.

```javascript
var Menu = require('menu');
var Tray = require('tray');

var appIcon = new Tray('/path/to/my/icon');
var contextMenu = Menu.buildFromTemplate([
  { label: 'Item1', type: 'radio' },
  { label: 'Item2', type: 'radio' },
  { label: 'Item3', type: 'radio', clicked: true },
  { label: 'Item4', type: 'radio' },
]);
appIcon.setToolTip('This is my application.');
appIcon.setContextMenu(contextMenu);
```

__Platform limitations:__

* On OS X `clicked` event will be ignored if the tray icon has context menu.

## Class: Tray

`Tray` is an [EventEmitter](event-emitter).

### new Tray(image)

* `image` String

Creates a new tray icon associated with the `image`.

### Event: 'clicked'

Emitted when the tray icon is clicked.

### Tray.setImage(image)

* `image` String

Sets the `image` associated with this tray icon.

### Tray.setPressedImage(image)

* `image` String

Sets the `image` associated with this tray icon when pressed.

### Tray.setToolTip(toolTip)

* `toolTip` String

### Tray.setContextMenu(menu)

* `menu` Menu

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter
