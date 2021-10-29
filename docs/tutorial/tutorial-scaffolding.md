---
title: 'Scaffolding'
description: 'This guide will step you through the process of creating a barebones Hello World app in Electron, similar to electron/electron-quick-start.'
slug: scaffolding
hide_title: false
---

:::info Tutorial parts
This is part 2 of the Electron tutorial. The other parts are:

1. [Prerequisites]
1. [Scaffolding]
1. [Main and Renderer process communication][main-renderer]
1. [Adding Features][features]
1. [Application Distribution]
1. [Code Signing]
1. [Updating Your Application][updates]

:::

At the end of this part you will have an Electron application with some basic
UI and you will know how to debug it.

## Create your application

Electron apps follow the same general structure as other Node.js projects.
Start by creating a folder and initializing an npm package.

```sh npm2yarn
mkdir my-electron-app && cd my-electron-app
npm init
```

:::warning About Windows and WSL
If you are on a Windows machine, please do not use [Windows Subsystem for Linux][wsl]
when running this tutorial as you will run into issues when trying to execute the application.
:::

The interactive `init` command will prompt you to set some fields in your config.
There are a few rules to follow for the purposes of this tutorial:

- `entry point` should be `main.js`.
- `author` and `description` can be any value, but are necessary for
  [app packaging][application distribution].

Your `package.json` file should look something like this:

```json title='package.json'
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
npm install --save-dev electron
```

Finally, you want to be able to execute Electron. In the [`scripts`][package-scripts]
field of your `package.json` config, add a `start` command like so:

```json {8-10} title='package.json'
{
  "name": "my-electron-app",
  "version": "1.0.0",
  "description": "Hello World!",
  "main": "main.js",
  "author": "Jane Doe",
  "license": "MIT",
  "scripts": {
    "start": "electron ."
  }
}
```

This `start` command will let you open your app in development mode.

```sh npm2yarn
npm start
```

This script tells Electron to run on your project's root folder. At this stage,
your app will immediately throw an error telling you

> "Error launching app"

This is expected as we have not written any code yet!

## Adding the entry point

The entry point of any Electron application is its `main` script. This script controls the
**main process**, which runs in a full Node.js environment and is responsible for
controlling your app's lifecycle, displaying native interfaces, performing privileged
operations, and managing renderer processes (more on that later).

During execution, Electron will look for this script in the [`main`][package-json-main]
field of the app's `package.json` config, which you should have configured in the previous
step.

To initialize the `main` script, create a file named `main.js` in the root folder
of your project with the following contents and run `npm start` again:

```js title='main.js'
console.log(`Hello from Electron 👋`);
```

Your terminal's console should print out `Hello from Electron 👋` after running the script.

Congratulations, you have executed your first Electron application!

:::tip Further reading 📚
If you haven't yet, please read [Electron's process model][process-model] to better understand
how all the pieces work together.
:::

## Creating a window

Before we can create a window for our application, we need to create the content that
will be loaded into it. In Electron, each window displays web contents that can be loaded
from either from a local HTML file or a remote URL.

For this tutorial, you will be doing the former. Create an `index.html` file in the root
folder of your project:

```html title='index.html'
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <!-- https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP -->
    <meta
      http-equiv="Content-Security-Policy"
      content="default-src 'self'; script-src 'self'"
    />
    <meta
      http-equiv="X-Content-Security-Policy"
      content="default-src 'self'; script-src 'self'"
    />
    <title>Hello from Electron renderer!</title>
  </head>
  <body>
    <h1>Hello from Electron renderer!</h1>
    <p>👋</p>
  </body>
</html>
```

Now that you have a web page, load it into an application window. To do so, paste
the following code in your `main.js` file so we can disect it line by line:

```js title='main.js'
const { app, BrowserWindow } = require('electron')

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      sandbox: true
    }
  })

  win.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()
})
```

In the first line, we are importing 2 Electron modules with the CommonJS
`require` syntax:

- The [`app`][app] module, which controls your application's event lifecycle.
- The [`BrowserWindow`][browser-window] module, which creates and manages application
  windows.

```js
const { app, BrowserWindow } = require('electron')
```

:::warning ESM and Electron
[ECMAScript modules](https://nodejs.org/api/esm.html) (i.e. using `import` to load a module) is currently not supported in Electron directly.
You can find more information about the state of ESM in Electron in
[this GitHub issue](https://github.com/electron/electron/issues/21457).
:::

Then, the `createWindow()` function loads `index.html` into a new `BrowserWindow`
instance with a series of options:

```js
const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      sandbox: true
    }
  })

  win.loadFile('index.html')
}
```

:::tip Further reading 📚
You can find all the constructor options available in the [`BrowserWindow` API docs][browser-window].
:::

In Electron, browser windows can only be created after the `app` module's
[`ready`][app-ready] event is fired. You can wait for this event by using the
[`app.whenReady()`][app-when-ready] API. Call `createWindow()` after `whenReady()`
resolves its Promise. This is done in the last 3 lines of the code:

```js
app.whenReady().then(() => {
  createWindow()
})
```

At this point, your Electron application should successfully open a window that displays your
web page! Each window created by your app will run in a separate process called a
**renderer**.

:::tip sandbox
One important thing from the code above is `sandbox: true`. The sandbox limits the harm that malicious
code can cause by limiting access to most system resources. While the sandbox used to be disabled by
default, that changes in Electron 18 where `sandbox: true` will be the default.

The sandbox will appear again in the tutorial, and you can read now more about it in the
[sandbox guide][sandbox].
:::

You have now code being executed in the main and renderer process.
The easiest way to debug the code in your renderer process is by using the built-in developer
tools. You can invoke them with a shortcut (`Ctrl/Command+Shift+I`) or programmatically
via `win.webContents.openDevTools()` in the main process. If you want to use VS Code,
keep reading!

The renderer has access to the web APIs. That means you can use the same JavaScript APIs and
tooling you use for typical front-end development, such as using [`webpack`][webpack] to bundle
and minify your code or [React][react] to manage your user interfaces.

:::tip Further reading 📚
You can find more information about this subject in the [Boilerplates and CLIs].
:::

## Optional: Debugging from VS Code

If you want to debug your application using VS Code instead of using the terminal you have to
keep in mind there are 2 processes to attach at this moment: the main and renderer processes.

We are going to create a configuration file that allow us to debug both at the same time:

1. Create a new folder `.vscode` in the root of your project.
1. Create a new file `launch.json` with the following content:

```json title='.vscode/launch.json'
{
  "version": "0.2.0",
  "compounds": [
    {
      "name": "Main + renderer",
      "configurations": ["Main", "Renderer"],
      "stopAll": true
    }
  ],
  "configurations": [
    {
      "name": "Renderer",
      "port": 9222,
      "request": "attach",
      "type": "pwa-chrome",
      "webRoot": "${workspaceFolder}"
    },
    {
      "name": "Main",
      "type": "pwa-node",
      "request": "launch",
      "cwd": "${workspaceFolder}",
      "runtimeExecutable": "${workspaceFolder}/node_modules/.bin/electron",
      "windows": {
        "runtimeExecutable": "${workspaceFolder}/node_modules/.bin/electron.cmd"
      },
      "args": [".", "--remote-debugging-port=9222"],
      "outputCapture": "std",
      "console": "integratedTerminal"
    }
  ]
}
```

<!-- markdownlint-disable MD029 -->

3. The option `Main + renderer` will appear when you select `Run and Debug`
from the sidebar, allowing you to set breakpoints and inspect all the variables among
other things in both the main and renderer processes.
<!-- markdownlint-enable MD029 -->

What we have done in the `launch.json` file is to create 3 configurations:

- `Main` is used to start the main process and also expose port 9222 for remote debugging
  (`--remote-debugging-port=9222`). This is the port that we will use to attach the debugger
  for the `Renderer`. Because the main process is a Node.js process, the type is set to
  `pwa-node` (`pwa-` is the prefix that tells VS Code to use the latest JavaScript debugger).
- `Renderer` is used to debug the renderer process. Because the main process is the one
  that creates the process, we have to "attach" to it (`"request": "attach"`) instead of
  creating a new one.
  The renderer process is a web one, so the debugger we have to use is `pwa-chrome`.
- `Main + renderer` is a [compound task] that executes the previous ones simulatenously.

:::caution
Because in `Renderer` we are attaching to a process, it might be possible that the first lines of
your code will be skipped as the debugger will not have had enough time to connect before they are
being executed.
You can work around this by refreshing the page or setting a timeout before executing the code
in development mode.
:::

:::tip Further reading 📚
If you want to dig deeper in the debugging area, the following guides provide more information:

- [Application debugging]
- [Using a devtools extension][devtools extension]
- The "Testing and Debugging" section in the sidebar

:::

<!-- Links -->

[activate]: ../api/app.md#event-activate-macos
[advanced-installation]: installation.md
[app]: ../api/app.md
[app-quit]: ../api/app.md#appquit
[app-ready]: ../api/app.md#event-ready
[app-when-ready]: ../api/app.md#appwhenready
[application debugging]: ./application-debugging.md
[boilerplates and clis]: boilerplates-and-clis.md
[browser-window]: ../api/browser-window.md
[commonjs]: https://nodejs.org/docs/latest/api/modules.html#modules_modules_commonjs_modules
[compound task]: https://code.visualstudio.com/Docs/editor/tasks#_compound-tasks
[devtools extension]: ./devtools-extension.md
[node-platform]: https://nodejs.org/api/process.html#process_process_platform
[package-json-main]: https://docs.npmjs.com/cli/v7/configuring-npm/package-json#main
[package-scripts]: https://docs.npmjs.com/cli/v7/using-npm/scripts
[process-model]: process-model.md
[react]: https://reactjs.org
[sandbox]: ./sandbox.md
[webpack]: https://webpack.js.org
[window-all-closed]: ../api/app.md#event-window-all-closed
[wsl]: https://docs.microsoft.com/en-us/windows/wsl/about#what-is-wsl-2

<!-- Tutorial links -->

[prerequisites]: ./tutorial-prerequisites.md
[scaffolding]: ./tutorial-scaffolding.md
[main-renderer]: ./tutorial-main-renderer.md
[features]: ./tutorial-adding-features.md
[application distribution]: ./distribution-overview.md
[code signing]: ./code-signing.md
[updates]: ./updates.md
