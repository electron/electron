# Quick start

## Introduction

Generally, atom-shell enables you to create desktop applications with pure
JavaScript by providing a runtime with rich native APIs, you could see it as
an variant of node.js runtime that focused on desktop applications instead of
web server.

But it doesn't mean atom-shell is a JavaScript binding to GUI libraries, instead
atom-shell uses web pages as GUI, so you could also see it as a minimal Chromium
browser, controlled by JavaScript.

### The browser side

If you had experience with node.js web applications, you would notice that there
are types of JavaScript scripts: the server side scripts and the client side
scripts. The server side JavaScript, is the scrips that run on the node.js
runtime, and the client side JavaScript, is the ones that run on user's browser.

In atom-shell we have similar concepts, since atom-shell displays GUI by showing
web pages, we would have scripts that run in the web page, and also have scripts
ran by the atom-shell runtime, which created those web pages. Like node.js, we
call the former ones client client scripts, and the latter one browser side
scripts.

In traditional node.js applications, communication between server side and
client side are usually done by web sockets. In atom-shell, we have provided
the [ipc](../api/renderer/ipc-renderer.md) module for browser side to client
communication, and the [remote](../api/renderer/remote.md) module for easy RPC
support.

### Web page and node.js

Normal web pages are designed to not touch outside world, which makes them not
suitable for interacting with native systems, atom-shell provides node.js APIs
in web pages so you could access native resources in web pages, just like
[node-webkit](https://github.com/rogerwang/node-webkit).

But unlike node-webkit, you could not do native GUI related operations in web
pages, instead you need to do them on the browser side by sending messages or
use the easy [remote](../api/renderer/remote.md) module.

## Write your first atom-shell app

### The architecture of an app

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

### Package your app in atom-shell

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
