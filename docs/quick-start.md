# Quick start

## Introduction

Generally, atom-shell lets you create a web-based desktop application in pure
javascript. Unlike CEF, which requires you to use C++ to write underlying
code, or node-webkit, which only allows you to write everything in the web
page, atom-shell gives you the power to use javascript to control the browser
side.

## Browser and renderer

Atom-shell is built upon Chromium's Content API, so it has the same
multi-processes architecture with the Chrome browser. In summary, things about
UI are done in the browser process, and each web page instance would start a
new renderer process.

In atom-shell, you can just put everything in a simpler way: when you are
executing javascript in browser side, you can control the application's life,
create UI widget, deal with system events, and create windows which contain
web pages; while on the renderer side, you can only control the web page you
are showing, if you want something more like creating a new window, you should
use IPC API to tell the browser to do that.

## The architecture of an app

Generally, an app of atom-shell should contains at least following files:

```text
app/
├── package.json
├── main.js
└── index.html
```

The format of `package.json` is exactly the same with node's modules, and the
script specified by the `main` field is the startup script of your app, which
will run under the browser side. An example of your `package.json` is like
this:

```json
{
  "name"    : "atom",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

The `main.js` will be executed, and in which you should do the initialization
work. To give the developers more power, atom-shell works by exposing
necessary Content APIs in javascript, so developers can precisely control
every piece of the app. An example of `main.js` is:

```javascript
var app = require('app');  // Module to control application life.
var Window = require('window');  // Module to create native browser window.

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the javascript object is GCed.
var mainWindow = null;

// Quit when all windows are closed.
app.on('window-all-closed', function() {
  app.terminate();
});

// This method will be called when atom-shell has done everything
// initialization and ready for creating browser windows.
app.on('ready', function() {
  // Create the browser window,
  mainWindow = new Window({ width: 800, height: 600 });
  // and load the index.html of the app.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  // Catch the event when web page in the window changes its title.
  mainWindow.on('page-title-updated', function(event, title) {
    // Prevent the default behaviour of 'page-title-updated' event.
    event.preventDefault();

    // Add a prefix for the window's original title.
    this.setTitle('Atom Shell - ' + title);
  });

  // Hook to when the window is closed.
  mainWindow.on('closed', function() {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null;
  });
});
```

Finally the `index.html` is the web page you want to show, in fact you
actually don't need to provide it, you can just make the window load url of a
remote page.

## Package your app in atom-shell

To make atom-shell run your app, you should name the folder of your app as
`app`, and put it under `Atom.app/Contents/Resources/`, like this:

```text
Atom.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

Then atom-shell will automatically read your `package.json`. If there is no
`Atom.app/Contents/Resources/app/`, atom-shell will load the default empty
app, which is `Atom.app/Contents/Resources/browser/default_app/`.

## IPC between browser and renderer

Atom-shell provides a set of javascript APIs for developers to communicate
between browser and renderers. There are two types of message: asynchronous
messages and synchronous messages, the former one is quite similar with node's
IPC APIs, while the latter one is mainly used for implement the RPC API.
Details can be found in the `ipc` module reference.
