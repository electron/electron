# Quick start

## Introduction

Generally, atom-shell enables you to create desktop applications with pure
JavaScript by providing a runtime with rich native APIs. You could see it as
a variant of the io.js runtime which is focused on desktop applications
instead of web servers.

It doesn't mean atom-shell is a JavaScript binding to GUI libraries. Instead,
atom-shell uses web pages as its GUI, so you could also see it as a minimal
Chromium browser, controlled by JavaScript.

### The main process

In atom-shell the process that runs `package.json`'s `main` script is called
__the main process__. The script runs in the main process can display GUI by
creating web pages.

### The renderer process

Since atom-shell uses Chromium for displaying web pages, Chromium's
multi-processes architecture is also used. Each web page in atom-shell runs in
its own process, which is called __the renderer process__.

In normal browsers web pages are usually running in sandboxed environment and
not allowed to access native resources. In atom-shell users are given the power
to use io.js APIs in web pages, so it would be possible to interactive with
low level operating system in web pages with JavaScript.

### Differences between main process and renderer process

The main process creates web pages by creating `BrowserWindow` instances, and
each `BrowserWindow` instance runs the web page in its own renderer process,
when a `BrowserWindow` instance is destroyed, the corresponding renderer process
would also be terminated.

So the main process manages all web pages and their corresponding renderer
processes, and each renderer process is separated from each other and only care
about the web page running in it.

In web pages, it is not allowed to call native GUI related APIs because managing
native GUI resources in web pages is very dangerous and easy to leak resources.
If you want to do GUI operations in web pages, you have to communicate with
the main process to do it there.

In atom-shell, we have provided the [ipc](../api/ipc-renderer.md) module for
communication between main process and renderer process. And there is also a
[remote](../api/remote.md) module for RPC style communication.

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
