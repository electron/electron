# Using Pepper Flash Plugin

Electron supports the Pepper Flash plugin. To use the Pepper Flash plugin in
Electron, you should manually specify the location of the Pepper Flash plugin
and then enable it in your application.

## Prepare a Copy of Flash Plugin

On OS X and Linux, the details of the Pepper Flash plugin can be found by
navigating to `chrome://plugins` in the Chrome browser. Its location and version
are useful for Electron's Pepper Flash support. You can also copy it to another
location.

## Add Electron Switch

You can directly add `--ppapi-flash-path` and `ppapi-flash-version` to the
Electron command line or by using the `app.commandLine.appendSwitch` method
before the app ready event. Also, turn on `plugins` option of `BrowserWindow`.
For example:

```javascript
// Specify flash path.
// On Windows, it might be /path/to/pepflashplayer.dll or just pepflashplayer.dll if it resides main.js
// On OS X, /path/to/PepperFlashPlayer.plugin
// On Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Optional: Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', () => {
  win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      plugins: true
    }
  });
  win.loadURL(`file://${__dirname}/index.html`);
  // Something else
});
```

The path to the system wide Pepper Flash plugin can also be obtained by calling `app.getPath('pepperFlashSystemPlugin')`.

## Enable Flash Plugin in a `<webview>` Tag

Add `plugins` attribute to `<webview>` tag.

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```

## Troubleshooting

You can check if Pepper Flash plugin was loaded by inspecting
`navigator.plugins` in the console of devtools (although you can't know if the
plugin's path is correct).

The architecture of Pepper Flash plugin has to match Electron's one. On Windows,
a common error is to use 32bit version of Flash plugin against 64bit version of
Electron.
