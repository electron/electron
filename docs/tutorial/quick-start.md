# Quick Start

Electron enables you to create desktop applications with pure JavaScript by
providing a runtime with rich native (operating system) APIs. You could see it
as a variant of the Node.js runtime that is focused on desktop applications
instead of web servers.

This doesn't mean Electron is a JavaScript binding to graphical user interface
(GUI) libraries. Instead, Electron uses web pages as its GUI, so you could also
see it as a minimal Chromium browser, controlled by JavaScript.

### Main Process

In Electron, the process that runs `package.json`'s `main` script is called
__the main process__. The script that runs in the main process can display a GUI
by creating web pages.

### Renderer Process

Since Electron uses Chromium for displaying web pages, Chromium's
multi-process architecture is also used. Each web page in Electron runs in
its own process, which is called __the renderer process__.

In normal browsers, web pages usually run in a sandboxed environment and are not
allowed access to native resources. Electron users, however, have the power to
use Node.js APIs in web pages allowing lower level operating system
interactions.

### Differences Between Main Process and Renderer Process

The main process creates web pages by creating `BrowserWindow` instances. Each
`BrowserWindow` instance runs the web page in its own renderer process. When a
`BrowserWindow` instance is destroyed, the corresponding renderer process
is also terminated.

The main process manages all web pages and their corresponding renderer
processes. Each renderer process is isolated and only cares about the web page
running in it.

In web pages, calling native GUI related APIs is not allowed because managing
native GUI resources in web pages is very dangerous and it is easy to leak
resources. If you want to perform GUI operations in a web page, the renderer
process of the web page must communicate with the main process to request that
the main process perform those operations.

In Electron, we have several ways to communicate between the main process and
renderer processes. Like [`ipcRenderer`](../api/ipc-renderer.md) and
[`ipcMain`](../api/ipc-main.md) modules for sending messages, and the
[remote](../api/remote.md) module for RPC style communication. There is also
an FAQ entry on [how to share data between web pages][share-data].

## Write your First Electron App

Generally, an Electron app is structured like this:

```text
your-app/
├── package.json
├── main.js
└── index.html
```

The format of `package.json` is exactly the same as that of Node's modules, and
the script specified by the `main` field is the startup script of your app,
which will run the main process. An example of your `package.json` might look
like this:

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__Note__: If the `main` field is not present in `package.json`, Electron will
attempt to load an `index.js`.

The `main.js` should create windows and handle system events, a typical
example being:

```javascript
const electron = require('electron');
// Module to control application life.
const app = electron.app;
// Module to create native browser window.
const BrowserWindow = electron.BrowserWindow;

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let mainWindow;

function createWindow () {
  // Create the browser window.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // and load the index.html of the app.
  mainWindow.loadURL(`file://${__dirname}/index.html`);

  // Open the DevTools.
  mainWindow.webContents.openDevTools();

  // Emitted when the window is closed.
  mainWindow.on('closed', () => {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null;
  });
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (mainWindow === null) {
    createWindow();
  }
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
```

Finally the `index.html` is the web page you want to show:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using node <script>document.write(process.versions.node)</script>,
    Chrome <script>document.write(process.versions.chrome)</script>,
    and Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## Run your app

Once you've created your initial `main.js`, `index.html`, and `package.json` files,
you'll probably want to try running your app locally to test it and make sure it's
working as expected.

### electron-prebuilt

[`electron-prebuilt`](https://github.com/electron-userland/electron-prebuilt) is
an `npm` module that contains pre-compiled versions of Electron.

If you've installed it globally with `npm`, then you will only need to run the
following in your app's source directory:

```bash
electron .
```

If you've installed it locally, then run:

```bash
./node_modules/.bin/electron .
```

### Manually Downloaded Electron Binary

If you downloaded Electron manually, you can also use the included
binary to execute your app directly.

#### Windows

```bash
$ .\electron\electron.exe your-app\
```

#### Linux

```bash
$ ./electron/electron your-app/
```

#### OS X

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

`Electron.app` here is part of the Electron's release package, you can download
it from [here](https://github.com/electron/electron/releases).

### Run as a distribution

After you're done writing your app, you can create a distribution by
following the [Application Distribution](./application-distribution.md) guide
and then executing the packaged app.

### Try this Example

Clone and run the code in this tutorial by using the [`atom/electron-quick-start`](https://github.com/electron/electron-quick-start)
repository.

**Note**: Running this requires [Git](https://git-scm.com) and [Node.js](https://nodejs.org/en/download/) (which includes [npm](https://npmjs.org)) on your system.

```bash
# Clone the repository
$ git clone https://github.com/electron/electron-quick-start
# Go into the repository
$ cd electron-quick-start
# Install dependencies and run the app
$ npm install && npm start
```

[share-data]: ../faq/electron-faq.md#how-to-share-data-between-web-pages
