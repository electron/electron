# Writing Your First Electron App

Electron enables you to create desktop applications with pure JavaScript by
providing a runtime with rich native (operating system) APIs. You could see it
as a variant of the Node.js runtime that is focused on desktop applications
instead of web servers.

This doesn't mean Electron is a JavaScript binding to graphical user interface
(GUI) libraries. Instead, Electron uses web pages as its GUI, so you could also
see it as a minimal Chromium browser, controlled by JavaScript.

**Note**: This example is also available as a repository you can
[download and run immediately](#trying-this-example).

As far as development is concerned, an Electron application is essentially a
Node.js application. The starting point is a `package.json` that is identical
to that of a Node.js module. A most basic Electron app would have the following
folder structure:

```plaintext
your-app/
├── package.json
├── main.js
└── index.html
```

Create a new empty folder for your new Electron application. Open up your
command line client and run `npm init` from that very folder.

```sh
npm init
```

npm will guide you through creating a basic `package.json` file. The script
specified by the `main` field is the startup script of your app, which will
run the main process. An example of your `package.json` might look like this:

```json
{
  "name": "your-app",
  "version": "0.1.0",
  "main": "main.js"
}
```

__Note__: If the `main` field is not present in `package.json`, Electron will
attempt to load an `index.js` (as Node.js does). If this was actually
a simple Node application, you would add a `start` script that instructs `node`
to execute the current package:

```json
{
  "name": "your-app",
  "version": "0.1.0",
  "main": "main.js",
  "scripts": {
    "start": "node ."
  }
}
```

Turning this Node application into an Electron application is quite simple - we
merely replace the `node` runtime with the `electron` runtime.

```json
{
  "name": "your-app",
  "version": "0.1.0",
  "main": "main.js",
  "scripts": {
    "start": "electron ."
  }
}
```

## Installing Electron

At this point, you'll need to install `electron` itself. The recommended way
of doing so is to install it as a development dependency in your app, which
allows you to work on multiple apps with different Electron versions. To do so,
run the following command from your app's directory:

```sh
npm install --save-dev electron
```

Other means for installing Electron exist. Please consult the
[installation guide](installation.md) to learn about use with proxies, mirrors,
and custom caches.

## Electron Development in a Nutshell

Electron apps are developed in JavaScript using the same principles and methods
found in Node.js development. All APIs and features found in Electron are
accessible through the `electron` module, which can be required like any other
Node.js module:

```javascript
const electron = require('electron')
```

The `electron` module exposes features in namespaces. As examples, the lifecycle
of the application is managed through `electron.app`, windows can be created
using the `electron.BrowserWindow` class. A simple `main.js` file might wait
for the application to be ready and open a window:

```javascript
const { app, BrowserWindow } = require('electron')

function createWindow () {
  // Create the browser window.
  let win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  })

  // and load the index.html of the app.
  win.loadFile('index.html')
}

app.on('ready', createWindow)
```

The `main.js` should create windows and handle all the system events your
application might encounter. A more complete version of the above example
might open developer tools, handle the window being closed, or re-create
windows on macOS if the user clicks on the app's icon in the dock.

```javascript
const { app, BrowserWindow } = require('electron')

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let win

function createWindow () {
  // Create the browser window.
  win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  })

  // and load the index.html of the app.
  win.loadFile('index.html')

  // Open the DevTools.
  win.webContents.openDevTools()

  // Emitted when the window is closed.
  win.on('closed', () => {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    win = null
  })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  // On macOS it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  // On macOS it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (win === null) {
    createWindow()
  }
})

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
    <!-- https://electronjs.org/docs/tutorial/security#csp-meta-tag -->
    <meta http-equiv="Content-Security-Policy" content="script-src 'self' 'unsafe-inline';" />
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using node <script>document.write(process.versions.node)</script>,
    Chrome <script>document.write(process.versions.chrome)</script>,
    and Electron <script>document.write(process.versions.electron)</script>.
  </body>
</html>
```

## Running Your App

Once you've created your initial `main.js`, `index.html`, and `package.json`
files, you can try your app by running `npm start` from your application's
directory.

## Trying this Example

Clone and run the code in this tutorial by using the
[`electron/electron-quick-start`][quick-start] repository.

**Note**: Running this requires [Git](https://git-scm.com) and [npm](https://www.npmjs.com/).

```sh
# Clone the repository
$ git clone https://github.com/electron/electron-quick-start
# Go into the repository
$ cd electron-quick-start
# Install dependencies
$ npm install
# Run the app
$ npm start
```

For a list of boilerplates and tools to kick-start your development process,
see the [Boilerplates and CLIs documentation][boilerplates].

[share-data]: ../faq.md#how-to-share-data-between-web-pages
[quick-start]: https://github.com/electron/electron-quick-start
[boilerplates]: ./boilerplates-and-clis.md
