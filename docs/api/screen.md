# screen

The `screen` module retrieves information about screen size, displays, cursor
position, etc. You should not use this module until the `ready` event of the
`app` module is emitted.

`screen` is an [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

**Note:** In the renderer / DevTools, `window.screen` is a reserved
DOM property, so writing `var screen = require('screen')` will not work. In our
examples below, we use `electronScreen` as the variable name instead.

An example of creating a window that fills the whole screen:

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

var mainWindow;

app.on('ready', function() {
  var electronScreen = require('screen');
  var size = electronScreen.getPrimaryDisplay().workAreaSize;
  mainWindow = new BrowserWindow({ width: size.width, height: size.height });
});
```

Another example of creating a window in the external display:

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

var mainWindow;

app.on('ready', function() {
  var electronScreen = require('screen');
  var displays = electronScreen.getAllDisplays();
  var externalDisplay = null;
  for (var i in displays) {
    if (displays[i].bounds.x != 0 || displays[i].bounds.y != 0) {
      externalDisplay = displays[i];
      break;
    }
  }

  if (externalDisplay) {
    mainWindow = new BrowserWindow({
      x: externalDisplay.bounds.x + 50,
      y: externalDisplay.bounds.y + 50
    });
  }
});
```

## Events

The `screen` module emits the following events:

### Event: 'display-added'

Returns:

* `event` Event
* `newDisplay` Object

Emitted when `newDisplay` has been added.

### Event: 'display-removed'

Returns:

* `event` Event
* `oldDisplay` Object

Emitted when `oldDisplay` has been removed.

### Event: 'display-metrics-changed'

Returns:

* `event` Event
* `display` Object
* `changedMetrics` Array

Emitted when one or more metrics change in a `display`. The `changedMetrics` is
an array of strings that describe the changes. Possible changes are `bounds`,
`workArea`, `scaleFactor` and `rotation`.

## Methods

The `screen` module has the following methods:

### `screen.getCursorScreenPoint()`

Returns the current absolute position of the mouse pointer.

### `screen.getPrimaryDisplay()`

Returns the primary display.

### `screen.getAllDisplays()`

Returns an array of displays that are currently available.

### `screen.getDisplayNearestPoint(point)`

* `point` Object
  * `x` Integer
  * `y` Integer

Returns the display nearest the specified point.

### `screen.getDisplayMatching(rect)`

* `rect` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

Returns the display that most closely intersects the provided bounds.
