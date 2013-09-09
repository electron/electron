# atom-delegate

The `atom-delegate` returns the delegate object for Chrome Content API. The
atom-shell would call methods of the delegate object when the corresponding
C++ code is called. Developers can override methods of it to control the
underlying behaviour of the browser.

An example of creating a new window when the browser is initialized:

```javascript
var delegate = require('atom-delegate');  // Delegate of Content API.
var BrowserWindow = require('browser-window');  // Module to create native browser window.

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the javascript object is GCed.
var mainWindow = null;

// This method will be called when atom-shell has done everything
// initialization and ready for creating browser windows.
delegate.browserMainParts.preMainMessageLoopRun = function() {
  // Create the browser window,
  mainWindow = new BrowserWindow({ width: 800, height: 600 });
  // and load the index.html of the app.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');
}
```

## atom-delegate.browserMainParts.preMainMessageLoopRun()

Called when atom-shell has done everything initialization and ready for
creating browser windows.
