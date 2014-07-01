# screen

Gets various info about screen size, displays, cursor position, etc.
```js
var Screen = require('screen');
var BrowserWindow = require('browser-window');
var size = Screen.getPrimaryDisplay().workAreaSize;
mainWindow = new BrowserWindow({ width: size.width, height: size.height });
```

## screen.getCursorScreenPoint()

Returns the current absolute position of the mouse pointer.

## screen.getPrimaryDisplay()

Returns the primary display.
