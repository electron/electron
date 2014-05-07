# Quick start

## Introduction

Generally atom-shell enables you to create desktop applications with pure
JavaScript by providing a runtime with rich native APIs. You could see it as
an variant of the Node.js runtime which is focused on desktop applications
instead of web server.

It doesn't mean atom-shell is a JavaScript binding to GUI libraries. Instead,
atom-shell uses web pages as GUI, so you could also see it as a minimal Chromium
browser, controlled by JavaScript.

### The browser side

If you had experience with Node.js web applications, you would notice that there
are two types of JavaScript scripts: the server side scripts and the client side
scripts. The server side JavaScript are the scripts that run on the Node.js
runtime, and the client side JavaScript are the ones that run on user's browser.

In atom-shell we have similar concepts, since atom-shell displays GUI by showing
web pages, we would have **scripts that run in the web page**, and also have
**scripts ran by the atom-shell runtime**, which created those web pages.
Like Node.js, we call them **client scripts**, and **browser scripts**.

In traditional Node.js applications, communication between server side and
client side are usually done by web sockets. In atom-shell, we have provided
the [ipc](../api/ipc-renderer.md) module for browser side to client
communication, and the [remote](../api/remote.md) module for easy RPC
support.

### Web page and Node.js

Normal web pages are designed to not touch outside world, which makes them
unsuitable for interacting with native systems. Atom-shell provides Node.js APIs
in web pages so you could access native resources in web pages, just like
[Node-Webkit](https://github.com/rogerwang/node-webkit).

But unlike Node-Webkit, you could not do native GUI related operations in web
pages, instead you need to do them on the browser side by sending messages or
use the easy [remote](../api/remote.md) module.


## Write your first atom-shell app

Generally, an atom-shell app would be like this (see this repo for reference [hello-atom](https://github.com/dougnukem/hello-atom)):

```text
your-app/
├── package.json
├── main.js
└── index.html
```

The format of `package.json` is exactly the same with Node's modules, and the
script specified by the `main` field is the startup script of your app, which
will run under the browser side. An example of your `package.json` is like
this:

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

The `main.js` should create windows and handle system events, and a typical
example is:

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

After you're done writing your app, you could create a distribution by
following the [Application distribution](./application-distribution.md) guide
and then execute the packaged app. You can also just use the downloaded atom-shell
binary to execute your app directly.

On Windows:

```cmd
$ .\atom-shell\atom.exe your-app\
```

On Linux:

```bash
$ ./atom-shell/atom your-app/
```

On Mac OS X:

```bash
$ ./Atom.app/Contents/MacOS/Atom your-app/
```
