# Using Pepper Flash Plugin

Electron supports the Pepper Flash plugin. To use the Pepper Flash plugin in
Electron, you should manually specify the location of the Pepper Flash plugin
and then enable it in your application.

## Prepare a Copy of Flash Plugin

On macOS and Linux, the details of the Pepper Flash plugin can be found by
navigating to `chrome://plugins` in the Chrome browser. Its location and version
are useful for Electron's Pepper Flash support. You can also copy it to another
location.

## Add Electron Switch

You can directly add `--ppapi-flash-path` and `--ppapi-flash-version` to the
Electron command line or by using the `app.commandLine.appendSwitch` method
before the app ready event. Also, turn on `plugins` option of `BrowserWindow`.

For example:

```javascript
const {app, BrowserWindow} = require('electron')
const path = require('path')

// Specify flash path, supposing it is placed in the same directory with main.js.
let pluginName
switch (process.platform) {
  case 'win32':
    pluginName = 'pepflashplayer.dll'
    break
  case 'darwin':
    pluginName = 'PepperFlashPlayer.plugin'
    break
  case 'linux':
    pluginName = 'libpepflashplayer.so'
    break
}
app.commandLine.appendSwitch('ppapi-flash-path', path.join(__dirname, pluginName))

// Optional: Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169')

app.on('ready', () => {
  let win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      plugins: true
    }
  })
  win.loadURL(`file://${__dirname}/index.html`)
  // Something else
})
```

You can also try loading the system wide Pepper Flash plugin instead of shipping
the plugins yourself, its path can be received by calling
`app.getPath('pepperFlashSystemPlugin')`.

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

On Windows the path passed to `--ppapi-flash-path` has to use `\` as path
delimiter, using POSIX-style paths will not work.
