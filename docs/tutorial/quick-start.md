# Quick Start Guide

## Quickstart

Electron is a framework that enables you to create desktop applications with JavaScript, HTML, and CSS. These applications can then be packaged to run directly on macOS, Windows, or Linux, or distributed via the Mac App Store or the Microsoft Store.

Typically, you create a desktop application for an operating system (OS) using each operating system's specific native application frameworks. Electron makes it possible to write your application once using technologies that you already know.

### Prerequisites

Before proceeding with Electron you need to install [Node.js][node-download].
We recommend that you install either the latest `LTS` or `Current` version available.

> Please install Node.js using pre-built installers for your platform.
> You may encounter incompatibility issues with different development tools otherwise.

To check that Node.js was installed correctly, type the following commands in your terminal client:

```sh
node -v
npm -v
```

The commands should print the versions of Node.js and npm accordingly.
If both commands succeeded, you are ready to install Electron.

### Create a basic application

From a development perspective, an Electron application is essentially a Node.js application. This means that the starting point of your Electron application will be a `package.json` file like in any other Node.js application. A minimal Electron application has the following structure:

```plaintext
my-electron-app/
├── package.json
├── main.js
├── preload.js
└── index.html
```

Let's create a basic application based on the structure above.

#### Install Electron

Create a folder for your project and install Electron there:

```sh
mkdir my-electron-app && cd my-electron-app
npm init -y
npm i --save-dev electron
```

#### Create the main script file

The main script specifies the entry point of your Electron application (in our case, the `main.js` file) that will run the Main process. Typically, the script that runs in the Main process controls the lifecycle of the application, displays the graphical user interface and its elements, performs native operating system interactions, and creates Renderer processes within web pages. An Electron application can have only one Main process.

The main script may look as follows:

```javascript fiddle='docs/fiddles/quick-start'
const { app, BrowserWindow } = require('electron')
const path = require('path')

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  win.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow()
    }
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})
```

##### What is going on above?

1. Line 1: First, you import the `app` and `BrowserWindow` modules of the `electron` package to be able to manage your application's lifecycle events, as well as create and control browser windows.
2. Line 2: Second, you import the `path` package which provides utility functions for file paths.
3. Line 4: After that, you define a function that creates a [new browser window](../api/browser-window.md#new-browserwindowoptions) with a preload script, loads `index.html` file into this window (line 13, we will discuss the file later).
4. Line 16: You create a new browser window by invoking the `createWindow` function once the Electron application [is initialized](../api/app.md#appwhenready).
5. Line 18: You add a new listener that creates a new browser window only if when the application has no visible windows after being activated. For example, after launching the application for the first time, or re-launching the already running application.
6. Line 25: You add a new listener that tries to quit the application when it no longer has any open windows. This listener is a no-op on macOS due to the operating system's [window management behavior](https://support.apple.com/en-ca/guide/mac-help/mchlp2469/mac).

#### Create a web page

This is the web page you want to display once the application is initialized. This web page represents the Renderer process. You can create multiple browser windows, where each window uses its own independent Renderer. You can optionally grant access to additional Node.js APIs by exposing them from your preload script.

The `index.html` page looks as follows:

```html fiddle='docs/fiddles/quick-start'
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Hello World!</title>
    <meta http-equiv="Content-Security-Policy" content="script-src 'self' 'unsafe-inline';" />
</head>
<body style="background: white;">
    <h1>Hello World!</h1>
    <p>
        We are using Node.js <span id="node-version"></span>,
        Chromium <span id="chrome-version"></span>,
        and Electron <span id="electron-version"></span>.
    </p>
</body>
</html>
```

#### Define a preload script

Your preload script acts as a bridge between Node.js and your web page. It allows you to expose specific APIs and behaviors to your web page rather than insecurely exposing the entire Node.js API. In this example we will use the preload script to read version information from the `process` object and update the web page with that info.

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

#### Modify your package.json file

Your Electron application uses the `package.json` file as the main entry point (as any other Node.js application). The main script of your application is `main.js`, so modify the `package.json` file accordingly:

```json
{
    "name": "my-electron-app",
    "version": "0.1.0",
    "author": "your name",
    "description": "My Electron app",
    "main": "main.js"
}
```

> NOTE: If the `main` field is omitted, Electron will attempt to load an `index.js` file from the directory containing `package.json`.

> NOTE: The `author` and `description` fields are required for packaging, otherwise error will occur when running `npm run make`.

By default, the `npm start` command will run the main script with Node.js. To run the script with Electron, you need to change it as such:

```json
{
    "name": "my-electron-app",
    "version": "0.1.0",
    "author": "your name",
    "description": "My Electron app",
    "main": "main.js",
    "scripts": {
        "start": "electron ."
    }
}
```

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
