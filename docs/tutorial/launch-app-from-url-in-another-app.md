---
title: Deep Links
description: Set your Electron app as the default handler for a specific protocol.
slug: launch-app-from-url-in-another-app
hide_title: true
---

# Deep Links

## Overview

<!-- ✍ Update this section if you want to provide more details -->

This guide will take you through the process of setting your Electron app as the default
handler for a specific [protocol](../api/protocol.md).

By the end of this tutorial, we will have set our app to intercept and handle
any clicked URLs that start with a specific protocol. In this guide, the protocol
we will use will be "`electron-fiddle://`".

## Examples

### Main Process (main.js)

First, we will import the required modules from `electron`. These modules help
control our application lifecycle and create a native browser window.

```js
const { app, BrowserWindow, shell, dialog } = require('electron')

const path = require('node:path')
```

Next, we will proceed to register our application to handle all "`electron-fiddle://`" protocols.

```js
if (process.defaultApp) {
  if (process.argv.length >= 2) {
    app.setAsDefaultProtocolClient('electron-fiddle', process.execPath, [path.resolve(process.argv[1])])
  }
} else {
  app.setAsDefaultProtocolClient('electron-fiddle')
}
```

We will now define the function in charge of creating our browser window and load our application's `index.html` file.

```js
let mainWindow

const createWindow = () => {
  // Create the browser window.
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  mainWindow.loadFile('index.html')
}

const findDeepLink = (argv) => {
  return argv.find((arg) => arg.startsWith('electron-fiddle://'))
}
```

In this next step, we will create our `BrowserWindow` and tell our application how to handle an event in which an external protocol is clicked.

This code will be different in Windows and Linux compared to macOS. On Windows and Linux, a deep link launched while your app is already running is delivered in the `second-instance` event, while a deep link that launches your app from a cold start is available in `process.argv`. Read more about this [here](../api/app.md#apprequestsingleinstancelockadditionaldata).

#### Windows and Linux code:

```js @ts-type={mainWindow:Electron.BrowserWindow} @ts-type={createWindow:()=>void}
let deeplinkingUrl

const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
  app.quit()
} else {
  app.on('second-instance', (event, commandLine) => {
    // Someone tried to run a second instance, we should focus our window.
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore()
      mainWindow.focus()
    }

    deeplinkingUrl = findDeepLink(commandLine)
    if (deeplinkingUrl) {
      dialog.showErrorBox('Welcome Back', `You arrived from: ${deeplinkingUrl}`)
    }
  })

  // Create mainWindow, load the rest of the app, etc...
  app.whenReady().then(() => {
    createWindow()

    deeplinkingUrl = findDeepLink(process.argv)
    if (deeplinkingUrl) {
      dialog.showErrorBox('Welcome Back', `You arrived from: ${deeplinkingUrl}`)
    }
  })
}
```

#### macOS code:

```js @ts-type={createWindow:()=>void}
// Register `open-url` listener early so it is available for app launch.
app.on('open-url', (event, url) => {
  event.preventDefault()
  dialog.showErrorBox('Welcome Back', `You arrived from: ${url}`)
})

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  createWindow()
})
```

Finally, we will add some additional code to handle when someone closes our application.

```js
// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
```

## Important notes

### Packaging

On macOS and Linux, this feature will only work when your app is packaged. It will not work when
you're launching it in development from the command-line. When you package your app you'll need to
make sure the macOS `Info.plist` and the Linux `.desktop` files for the app are updated to include
the new protocol handler. Some of the Electron tools for bundling and distributing apps handle
this for you.

#### [Electron Forge](https://electronforge.io)

If you're using Electron Forge, adjust `packagerConfig` for macOS support, and the configuration for
the appropriate Linux makers for Linux support, in your [Forge configuration](https://www.electronforge.io/configuration)
_(please note the following example only shows the bare minimum needed to add the configuration changes)_:

```json
{
  "config": {
    "forge": {
      "packagerConfig": {
        "protocols": [
          {
            "name": "Electron Fiddle",
            "schemes": ["electron-fiddle"]
          }
        ]
      },
      "makers": [
        {
          "name": "@electron-forge/maker-deb",
          "config": {
            "mimeType": ["x-scheme-handler/electron-fiddle"]
          }
        }
      ]
    }
  }
}
```

#### [Electron Packager](https://github.com/electron/packager)

For macOS support:

If you're using Electron Packager's API, adding support for protocol handlers is similar to how
Electron Forge is handled, except
`protocols` is part of the Packager options passed to the `packager` function.

```js @ts-nocheck
const packager = require('@electron/packager')

packager({
  // ...other options...
  protocols: [
    {
      name: 'Electron Fiddle',
      schemes: ['electron-fiddle']
    }
  ]

}).then(paths => console.log(`SUCCESS: Created ${paths.join(', ')}`))
  .catch(err => console.error(`ERROR: ${err.message}`))
```

If you're using Electron Packager's CLI, use the `--protocol` and `--protocol-name` flags. For
example:

```shell
npx electron-packager . --protocol=electron-fiddle --protocol-name="Electron Fiddle"
```

## Conclusion

After you start your Electron app, you can enter in a URL in your browser that contains the custom
protocol, for example `"electron-fiddle://open"` and observe that the application will respond and
show an error dialog box.

<!--
    Because Electron examples usually require multiple files (HTML, CSS, JS
    for the main and renderer process, etc.), we use this custom code block
    for Fiddle (https://www.electronjs.org/fiddle).
    Please modify any of the files in the referenced folder to fit your
    example.
    The content in this codeblock will not be rendered in the website so you
    can leave it empty.
-->

```fiddle docs/fiddles/system/protocol-handler/launch-app-from-URL-in-another-app

```

<!-- ✍ Explanation of the code below -->
