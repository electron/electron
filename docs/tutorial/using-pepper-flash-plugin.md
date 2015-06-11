# Using Pepper Flash Plugin

Pepper flash plugin is now supported. To use pepper flash plugin in Electron, you should manually specify the location of pepper flash plugin and then enable it in your application.

## Prepare a copy of flash plugin

On OS X and Linux, the detail of pepper flash plugin can be found by navigating `chrome://plugins` in Chrome browser. Its location and version are useful for electron's pepper flash support. You can also copy it to anywhere else.

## Add Electron switch

You can directly add `--ppapi-flash-path` and `ppapi-flash-version` to electron commandline or by `app.commandLine.appendSwitch` method before app ready event. Also, add the `plugins` switch of `browser-window`. For example,

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

// Report crashes to our server.
require('crash-reporter').start();

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the javascript object is GCed.
var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// Specify flash path.
// On Windows, it might be /path/to/pepflashplayer.dll
// On Mac, /path/to/PepperFlashPlayer.plugin
// On Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    'web-preferences': {
      'plugins': true
    }
  });
  mainWindow.loadUrl('file://' + __dirname + '/index.html');
  // Something else
});
```

## Enable flash plugin in a `<webview>` tag
Add `plugins` attribute to `<webview>` tag.
```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
