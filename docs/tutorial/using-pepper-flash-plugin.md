# Using Pepper Flash Plugin

Electron now supports the Pepper Flash plugin. To use the Pepper Flash plugin in
Electron, you should manually specify the location of the Pepper Flash plugin
and then enable it in your application.

## Prepare a Copy of Flash Plugin

On OS X and Linux, the details of the Pepper Flash plugin can be found by
navigating to `chrome://plugins` in the Chrome browser. Its location and version
are useful for Electron's Pepper Flash support. You can also copy it to another
location.

_**Attention:** On windows, Pepper Flash plugin is win32 and it won't work with Electron x64 version. 
<br>Get win32 version from [Electron Releases](https://github.com/electron/electron/releases)_

## Add Electron Switch

You can directly add `--ppapi-flash-path` and `ppapi-flash-version` to the
Electron command line or by using the `app.commandLine.appendSwitch` method
before the app ready event. Also, add the `plugins` switch of `browser-window`.
For example:

```javascript
// Specify flash path.
// On Windows, it might be /path/to/pepflashplayer.dll or just pepflashplayer.dll if it resides main.js
// On OS X, /path/to/PepperFlashPlayer.plugin
// On Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Optional: Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    // web-preferences is deprecated. Use webPreferences instead.
    'webPreferences': {
      'plugins': true
    }
  });
  mainWindow.loadURL('file://' + __dirname + '/index.html');
  // Something else
});
```

_**Attention:** You can check if Flash dll was loaded by running `navigator.plugins` on the Console (although you can't know if the plugin's path is correct)_

## Enable Flash Plugin in a `<webview>` Tag

Add `plugins` attribute to `<webview>` tag.

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```
