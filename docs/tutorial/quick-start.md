<!--
TODO:
* Explain webContents?
* Explain why renderer has no Node.js access.
* Explain preload

-->

# Quick Start

This guide will step you through the process of creating a barebones Hello World app in
Electron, similar to the app in the [`electron/electron-quick-start`][quick-start]
repository.

This app will open a browser window that displays information about which Chromium,
Node.js, and Electron versions the app is using.

## Prerequisites

Before proceeding with Electron, you need to install [Node.js][node-download] and
its package manager, `npm`. We recommend that you install either the latest `LTS`
or `Current` version available.

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
your code does not correspond to the version running on your system.

## Create your application

### Scaffold the project

Electron apps follow the same general structure as other Node.js projects.
Start by creating a folder for your app and initializing its `package.json` config.

```sh npm2yarn
mkdir my-electron-app && cd my-electron-app
npm init
```

The interactive `init` command will prompt you to set some fields in your config.
There are a few rules to follow for the purposes of this tutorial:

* `entry point` should be `main.js`.
* `author` and `description` can be any value, but are necessary for app packaging.

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
<!--TODO(erickzhao): explain why Electron is a dev dependency? -->

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

> Note: Running `npm start` will tell the Electron binary to run on your project's root
> folder. At this stage, Electron will run, but the script will immediately throw
> an error telling you that an app cannot be found in your folder.

### Run the main process

The entry point of any Electron application is its main script. This script controls the
**main process**, which runs in a full Node.js environment and is responsible for
controlling your app's lifecycle, displaying native interfaces, performing privileged
operations, and managing renderer processes (more on that later).

During execution, Electron will look for this script in the [`main`][package-json-main]
field of the app's `package.json` config, which you should have configured during the
[app scaffolding](#scaffold-the-project) step.

To initialize the main script, create a file named `main.js` in the root folder of your
project.

```sh
touch main.js
```

> Note: If you run the `start` script again at this point, your app will no longer throw
> any errors! However, it will do absolutely nothing because you're not defining any
> behaviour in your main script. You'll learn how to do that in the next step.

### Create a browser window

Next, let's add functionality to the main process so it can create browser windows.
In order to do so, we'll need to use two Electron modules:

* The [`app`] module controls your application's event lifecycle
* The [`BrowserWindow`] module to create and control windows.

Because the main process runs Node.js, you can import these as [CommonJS] modules at
the top of your file:

```js
const { app, BrowserWindow } = require('electron')
```

In Electron, each window in your app runs in a separate process called a **renderer**.
Renderers are created through the main process by loading HTML into a `BrowserWindow`
instance.

First, you should create a separate `index.html` file in the root folder of your project:

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

Then, underneath your imports in `main.js`, add a `createWindow()` function that
loads `index.html` into a `BrowserWindow` instance.

```js
function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.loadFile('index.html')
}
```

Browser windows can only be created in Electron after the `app` module's
[`ready`][app-ready] event is fired. You can do this by using the
[`app.whenReady()`][app-when-ready] API, which returns a Promise that resolves once
Electron is fully initialized. Call `createWindow()` after the Promise is
resolved.

```js
app.whenReady().then(() => {
  createWindow()
})
```

> Note: At this point in time, your Electron application should successfully
> create a functional browser window with the Hello World message!

### Manage your window's lifecycle

Although your browser window is working, you'll need some additional boilerplate code
to make it feel more native to each platform. All operating systems have quirks on how
application windows behave, and Electron puts the responsibility on developers to
implement these conventions in their app.

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
continue running even without any windows open. Moreover, clicking the app's dock icon
usually opens a new app window.

To implement this feature, listen for the `app` module's [`activate`][activate]
event, and call your existing `createWindow()` method if no browser windows are open.
Note that the `activate` event is only emitted on macOS, so you don't actually need
to guard this action using `process.platform`.

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

> Note: At this point, your window should be behaving like any native application window
> would. The only functionality left to add is the ability to display the embedded module
> versions of Chromium, Node.js, and Electron itself.

### Bridge Node.js and the renderer

Accessing the version numbers for Electron and its dependencies is trivial to do in the
main process through the Node.js `process.versions` object. However, accessing this
information from within a renderer is difficult because it is running in an entirely
different process with no Node.js access.

<!-- This is where a preload script comes in handy. Your preload script acts as a bridge
between Node.js and your web page. By default, the preload script and your renderer
process live in isolated worlds, meaning. It allows you to expose specific APIs and behaviors
to your web page rather than insecurely exposing the entire Node.js API. -->

### Define a preload script

Your preload script (in our case, the `preload.js` file) acts as a bridge between Node.js and your web page. It allows you to expose specific APIs and behaviors to your web page rather than insecurely exposing the entire Node.js API. In this example we will use the preload script to read version information from the `process` object and update the web page with that info.

```javascript fiddle='docs/fiddles/quick-start'
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

##### What's going on above?

1. On line 1: First you define an event listener that tells you when the web page has loaded
2. On line 2: Second you define a utility function used to set the text of the placeholders in the `index.html`
3. On line 7: Next you loop through the list of components whose version you want to display
4. On line 8: Finally, you call `replaceText` to look up the version placeholders in `index.html` and set their text value to the values from `process.versions`

#### Run your application

```sh
npm start
```

Your running Electron app should look as follows:

![Simplest Electron app](../images/simplest-electron-app.png)

### Package and distribute the application

The simplest and the fastest way to distribute your newly created app is using
[Electron Forge](https://www.electronforge.io).

1. Import Electron Forge to your app folder:

    ```sh
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

1. Create a distributable:

    ```sh
    npm run make

    > my-gsod-electron-app@1.0.0 make /my-electron-app
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

    Electron-forge creates the `out` folder where your package will be located:

    ```plain
    // Example for MacOS
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
