# Quick start

## Introduction

Generally, atom-shell enables you to create desktop applications with pure
JavaScript by providing a runtime with rich native APIs. You could see it as
a variant of the Node.js runtime which is focused on desktop applications
instead of web servers.

It doesn't mean atom-shell is a JavaScript binding to GUI libraries. Instead,
atom-shell uses web pages as its GUI, so you could also see it as a minimal
Chromium browser, controlled by JavaScript.

### The main process

The main atom-shell process displays a GUI by creating web pages. We have
**scripts that run in the atom-shell runtime**, that create **scripts
that run in the web page**.

In atom-shell, we have provided the [ipc](../api/ipc-renderer.md) module for
communication from the main process to the renderer process, and the
[remote](../api/remote.md) module for easy RPC support.

### The renderer process

Normal web pages are designed to not reach outside of the browser, which makes
them unsuitable for interacting with native systems. Atom-shell provides Node.js
APIs in web pages so you can access native resources from web pages, just like
[nw.js](https://github.com/nwjs/nw.js).

But unlike nw.js, you cannot do native GUI related operations in web
pages. Instead you need to do them on the main process by sending messages to
it, or using the easy [remote](../api/remote.md) module.


## Write your first atom-shell app

Generally, an atom-shell app would be structured like this (see the
[hello-atom](https://github.com/dougnukem/hello-atom) repo for reference):

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
  if (process.platform != 'darwin')
    app.quit();
});

// This method will be called when atom-shell has done everything
// initialization and ready for creating browser windows.
app.on('ready', function() {
  // Create the browser window.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // and load the index.html of the app.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

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
    We are using node.js <script>document.write(process.version)</script>
    and atom-shell <script>document.write(process.versions['atom-shell'])</script>.
  </body>
</html>
```

## Run your app

After you're done writing your app, you can create a distribution by
following the [Application distribution](./application-distribution.md) guide
and then execute the packaged app. You can also just use the downloaded
atom-shell binary to execute your app directly.

On Windows:

```cmd
$ .\atom-shell\atom.exe your-app\
```

On Linux:

```bash
$ ./atom-shell/atom your-app/
```

On OS X:

```bash
$ ./Atom.app/Contents/MacOS/Atom your-app/
```

`Atom.app` here is part of the atom-shell's release package, you can download
it from [here](https://github.com/atom/atom-shell/releases).
