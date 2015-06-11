# Quick start

## Introduction

Electron enables you to create desktop applications with pure JavaScript by providing a runtime with rich native APIs. You could see it as a variant of the io.js runtime which is focused on desktop applications instead of web servers.

It doesn't mean Electron is a JavaScript binding to GUI libraries. Instead,
Electron uses web pages as its GUI, so you could also see it as a minimal
Chromium browser, controlled by JavaScript.

### Main process

In Electron, the process that runs `package.json`'s `main` script is called
__the main process__. The script that runs in the main process, can display GUI by
creating web pages.

### Renderer process

Since Electron uses Chromium for displaying web pages, Chromium's
multi-processes architecture is also used. Each web page in Electron runs in
its own process, which is called __the renderer process__.

In normal browsers web pages usually run in a sandboxed environment and are not
allowed access to native resources. Electron users however, have the power to use
io.js APIs in web pages allowing lower level operating system interactions.

### Differences between main process and renderer process

The main process creates web pages by creating `BrowserWindow` instances. Each `BrowserWindow` instance runs the web page in its own renderer process. When a `BrowserWindow` instance is destroyed, the corresponding renderer process
would also be terminated.

The main process manages all web pages and their corresponding renderer
processes, each renderer process is isolated and only cares
about the web page running in it.

In web pages, it is not allowed to call native GUI related APIs because managing
native GUI resources in web pages is very dangerous and easy to leak resources.
If you want to do GUI operations in web pages, you have to communicate with
the main process to do it there.

In Electron, we have provided the [ipc](../api/ipc-renderer.md) module for
communication between main process and renderer process. And there is also a
[remote](../api/remote.md) module for RPC style communication.

## Write your first Electron app

Generally, an Electron app would be structured like this:

```text
your-app/
├── package.json
├── main.js
└── index.html
```

The format of `package.json` is exactly the same as that of Node's modules, and
the script specified by the `main` field is the startup script of your app,
which will run on the main process. An example of your `package.json` might look
like this:

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

The `main.js` should create windows and handle system events, a typical
example being:

```javascript
var app = require('app');  // Module to control application life.
var BrowserWindow = require('browser-window');  // Module to create native browser window.

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

// This method will be called when Electron has done everything
// initialization and ready for creating browser windows.
app.on('ready', function() {
  // Create the browser window.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // and load the index.html of the app.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  // Open the devtools.
  mainWindow.openDevTools();

  // Emitted when the window is closed.
  mainWindow.on('closed', function() {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null;
  });
});
```

Finally the `index.html` is the web page you want to show:

```html
<!DOCTYPE html>
<html>
  <head>
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using io.js <script>document.write(process.version)</script>
    and Electron <script>document.write(process.versions['electron'])</script>.
  </body>
</html>
```

## Run your app

After you're done writing your app, you can create a distribution by
following the [Application distribution](./application-distribution.md) guide
and then execute the packaged app. You can also just use the downloaded
Electron binary to execute your app directly.

On Windows:

```bash
$ .\electron\electron.exe your-app\
```

On Linux:

```bash
$ ./electron/electron your-app/
```

On OS X:

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` here is part of the Electron's release package, you can download
it from [here](https://github.com/atom/electron/releases).
