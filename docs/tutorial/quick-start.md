# Quick Start

This guide will step you through the process of creating a barebones Hello World app in
Electron, similar to [`electron/electron-quick-start`][quick-start].

By the end of this tutorial, your app will open a browser window that displays a web page
with information about which Chromium, Node.js, and Electron versions are running.

## Prerequisites

To use Electron, you need to install [Node.js][node-download]. We recommend that you
use the latest `LTS` version available.

> Please install Node.js using pre-built installers for your platform.
> You may encounter incompatibility issues with different development tools otherwise.

To check that Node.js was installed correctly, type the following commands in your
terminal client:

```sh
node -v
npm -v
```

The commands should print the versions of Node.js and npm accordingly.

**Note:** Since Electron embeds Node.js into its binary, the version of Node running
your code is unrelated to the version running on your system.

## Create your application

### Scaffold the project

Electron apps follow the same general structure as other Node.js projects.
Start by creating a folder and initializing an npm package.

```sh npm2yarn
mkdir my-electron-app && cd my-electron-app
npm init
```

The interactive `init` command will prompt you to set some fields in your config.
There are a few rules to follow for the purposes of this tutorial:

* `entry point` should be `main.js`.
* `author` and `description` can be any value, but are necessary for
[app packaging](#package-and-distribute-your-application).

Your `package.json` file should look something like this:

```json
{
  "name": "my-electron-app",
  "version": "1.0.0",
  "description": "Hello World!",
  "main": "main.js",
  "author": "Jane Doe",
  "license": "MIT"
}
```

Then, install the `electron` package into your app's `devDependencies`.

```sh npm2yarn
$ npm install --save-dev electron
```

> Note: If you're encountering any issues with installing Electron, please
> refer to the [Advanced Installation] guide.

Finally, you want to be able to execute Electron. In the [`scripts`][package-scripts]
field of your `package.json` config, add a `start` command like so:

```json
{
  "scripts": {
    "start": "electron ."
  }
}
```

> Note: This script tells Electron to run on your project's root folder. At this stage,
> your app will immediately throw an error telling you that it cannot find an app to run.

### Run the main process

The entry point of any Electron application is its `main` script. This script controls the
**main process**, which runs in a full Node.js environment and is responsible for
controlling your app's lifecycle, displaying native interfaces, performing privileged
operations, and managing renderer processes (more on that later).

During execution, Electron will look for this script in the [`main`][package-json-main]
field of the app's `package.json` config, which you should have configured during the
[app scaffolding](#scaffold-the-project) step.

To initialize the `main` script, create an empty file named `main.js` in the root folder
of your project.

> Note: If you run the `start` script again at this point, your app will no longer throw
> any errors! However, it won't do anything yet because we haven't added any code into
> `main.js` yet.

### Create a web page

Before we can create a window for our application, we need to create the content that
will be loaded into it. In Electron, each window displays web contents that can be loaded
from either from a local HTML file or a remote URL.

For this tutorial, you will be doing the former. Create an `index.html` file in the root
folder of your project:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <!-- https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP -->
    <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'">
    <meta http-equiv="X-Content-Security-Policy" content="default-src 'self'; script-src 'self'">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using Node.js <span id="node-version"></span>,
    Chromium <span id="chrome-version"></span>,
    and Electron <span id="electron-version"></span>.
  </body>
</html>
```

> Note: Looking at this HTML document, you can observe that the version numbers are
> missing from the body text. We'll manually insert them later using JavaScript.

### Opening your web page in a browser window

Now that you have a web page, load it into an application window. To do so, you'll
need two Electron modules:

* The [`app`] module, which controls your application's event lifecycle.
* The [`BrowserWindow`] module, which creates and manages application windows.

Because the main process runs Node.js, you can import these as [CommonJS] modules at
the top of your file:

```js
const { app, BrowserWindow } = require('electron')
```

Then, add a `createWindow()` function that loads `index.html` into a new `BrowserWindow`
instance.

```js
function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.loadFile('index.html')
}
```

Next, call this `createWindow()` function to open your window.

In Electron, browser windows can only be created after the `app` module's
[`ready`][app-ready] event is fired. You can wait for this event by using the
[`app.whenReady()`][app-when-ready] API. Call `createWindow()` after `whenReady()`
resolves its Promise.

```js
app.whenReady().then(() => {
  createWindow()
})
```

> Note: At this point, your Electron application should successfully
> open a window that displays your web page!

### Manage your window's lifecycle

Although you can now open a browser window, you'll need some additional boilerplate code
to make it feel more native to each platform. Application windows behave differently on
each OS, and Electron puts the responsibility on developers to implement these
conventions in their app.

In general, you can use the [`process`] global's [`platform`][node-platform] attribute
to run code specifically for certain operating systems.

#### Quit the app when all windows are closed (Windows & Linux)

On Windows and Linux, exiting all windows generally quits an application entirely.

To implement this, listen for the `app` module's [`'window-all-closed'`][window-all-closed]
event, and call [`app.quit()`][app-quit] if the user is not on macOS (`darwin`).

```js
app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})
```

#### Open a window if none are open (macOS)

Whereas Linux and Windows apps quit when they have no windows open, macOS apps generally
continue running even without any windows open, and activating the app when no windows
are available should open a new one.

To implement this feature, listen for the `app` module's [`activate`][activate]
event, and call your existing `createWindow()` method if no browser windows are open.

Because windows cannot be created before the `ready` event, you should only listen for
`activate` events after your app is initialized. Do this by attaching your event listener
from within your existing `whenReady()` callback.

```js
app.whenReady().then(() => {
  createWindow()

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})
```

> Note: At this point, your window controls should be fully functional!

### Add functionality with a preload script

Now, the last thing to do is print out the version numbers for Electron and its
dependencies onto your web page.

Accessing this information is trivial to do in the main process through Node's
global `process` object. However, you can't just edit the DOM from the main
process because it has no access to the renderer's `document` context.
They're in entirely different processes!

This is where attaching a **preload** script to your renderer comes in handy.
A preload script runs before the renderer process is loaded, and has access to both
renderer globals (e.g. `window` and `document`) and a Node.js environment.

Create a preload script as such:

```js
window.addEventListener('DOMContentLoaded', () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector)
    if (element) element.innerText = text
  }

  for (const type of ['chrome', 'node', 'electron']) {
    replaceText(`${type}-version`, process.versions[type])
  }
})
```

Here, as soon as web page is loaded, we access Node.js' `process.versions`
object and run a basic `replaceText` helper function to insert the version numbers
into the HTML document.

### Run your application

<!-- TODO: summary and recap? -->

```sh npm2yarn
npm start
```

Your Electron app should look as follows:

![Simplest Electron app](../images/simplest-electron-app.png)

## Package and distribute your application

The fastest way to distribute your newly created app is using
[Electron Forge](https://www.electronforge.io).

1. Import Electron Forge to your app folder, and use the `import` script to set up
Forge's scaffolding:

    ```sh npm2yarn
    npm install --save-dev @electron-forge/cli
    npx electron-forge import

    ✔ Checking your system
    ✔ Initializing Git Repository
    ✔ Writing modified package.json file
    ✔ Installing dependencies
    ✔ Writing modified package.json file
    ✔ Fixing .gitignore

    We have ATTEMPTED to convert your app to be in a format that electron-forge understands.

    Thanks for using "electron-forge"!!!
    ```

1. Create a distributable using Forge's `make` command:

    ```sh npm2yarn
    npm run make

    > my-electron-app@1.0.0 make /my-electron-app
    > electron-forge make

    ✔ Checking your system
    ✔ Resolving Forge Config
    We need to package your application before we can make it
    ✔ Preparing to Package Application for arch: x64
    ✔ Preparing native dependencies
    ✔ Packaging Application
    Making for the following targets: zip
    ✔ Making for target: zip - On platform: darwin - For arch: x64
    ```

    Electron Forge creates the `out` folder where your package will be located:

    ```plain
    // Example for macOS
    out/
    ├── out/make/zip/darwin/x64/my-electron-app-darwin-x64-1.0.0.zip
    ├── ...
    └── out/my-electron-app-darwin-x64/my-electron-app.app/Contents/MacOS/my-electron-app
    ```

[node-download]: https://nodejs.org/en/download/

## Learning the basics

This section guides you through the basics of how Electron works under the hood. It aims at strengthening knowledge about Electron and the application created earlier in the Quickstart section.

### Application architecture

Electron consists of three main pillars:

* **Chromium** for displaying web content.
* **Node.js** for working with the local filesystem and the operating system.
* **Custom APIs** for working with often-needed OS native functions.

Developing an application with Electron is like building a Node.js app with a web interface or building web pages with seamless Node.js integration.

#### Main and Renderer Processes

As it was mentioned before, Electron has two types of processes: Main and Renderer.

* The Main process **creates** web pages by creating `BrowserWindow` instances. Each `BrowserWindow` instance runs the web page in its Renderer process. When a `BrowserWindow` instance is destroyed, the corresponding Renderer process gets terminated as well.
* The Main process **manages** all web pages and their corresponding Renderer processes.

----

* The Renderer process **manages** only the corresponding web page. A crash in one Renderer process does not affect other Renderer processes.
* The Renderer process **communicates** with the Main process via IPC to perform GUI operations in a web page. Calling native GUI-related APIs from the Renderer process directly is restricted due to security concerns and potential resource leakage.

----

The communication between processes is possible via Inter-Process Communication (IPC) modules: [`ipcMain`](../api/ipc-main.md) and [`ipcRenderer`](../api/ipc-renderer.md).

#### APIs

##### Electron API

Electron APIs are assigned based on the process type, meaning that some modules can be used from either the Main or Renderer process, and some from both. Electron's API documentation indicates which process each module can be used from.

For example, to access the Electron API in both processes, require its included module:

```js
const electron = require('electron')
```

To create a window, call the `BrowserWindow` class, which is only available in the Main process:

```js
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()
```

To call the Main process from the Renderer, use the IPC module:

```js
// In the Main process
const { ipcMain } = require('electron')

ipcMain.handle('perform-action', (event, ...args) => {
  // ... do actions on behalf of the Renderer
})
```

```js
// In the Renderer process
const { ipcRenderer } = require('electron')

ipcRenderer.invoke('perform-action', ...args)
```

> NOTE: Because Renderer processes may run untrusted code (especially from third parties), it is important to carefully validate the requests that come to the Main process.

##### Node.js API

> NOTE: To access the Node.js API from the Renderer process, you need to set the `nodeIntegration` preference to `true` and the `contextIsolation` preference to `false`.  Please note that access to the Node.js API in any renderer that loads remote content is not recommended for [security reasons](../tutorial/security.md#2-do-not-enable-nodejs-integration-for-remote-content).

Electron exposes full access to Node.js API and its modules both in the Main and the Renderer processes. For example, you can read all the files from the root directory:

```js
const fs = require('fs')

const root = fs.readdirSync('/')

console.log(root)
```

To use a Node.js module, you first need to install it as a dependency:

```sh
npm install --save aws-sdk
```

Then, in your Electron application, require the module:

```js
const S3 = require('aws-sdk/clients/s3')
```

[quick-start]: https://github.com/electron/electron-quick-start
