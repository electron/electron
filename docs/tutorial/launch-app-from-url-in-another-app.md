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
```

In this next step, we will create our  `BrowserWindow` and tell our application how to handle an event in which an external protocol is clicked.

This code will be different in Windows and Linux compared to MacOS. This is due to both platforms emitting the `second-instance` event rather than the `open-url` event and Windows requiring additional code in order to open the contents of the protocol link within the same Electron instance. Read more about this [here](../api/app.md#apprequestsingleinstancelockadditionaldata).

#### Windows and Linux code:

```js
// Check if we're the primary instance
const gotTheLock = app.requestSingleInstanceLock()

if (!gotTheLock) {
  app.quit()
} else {
  // Handle second-instance events (app already running)
  app.on('second-instance', (event, commandLine, workingDirectory) => {
    // Someone tried to run a second instance, we should focus our window.
    if (mainWindow) {
      if (mainWindow.isMinimized()) mainWindow.restore()
      mainWindow.focus()
    }
    // The last element in commandLine array is the deep link URL
    dialog.showErrorBox('Welcome Back', `You arrived from: ${commandLine.pop()}`)
  })

  // Create window and handle initial deep link (app was closed)
  app.whenReady().then(() => {
    createWindow()
    
    // On Windows/Linux, check for deep link in initial launch arguments
    // This handles the case when the app is launched via a deep link
    if (process.platform !== 'darwin') {
      const deepLink = process.argv.find(arg => arg.startsWith('electron-fiddle://'))
      if (deepLink) {
        dialog.showErrorBox('Initial Launch', `You arrived from: ${deepLink}`)
      }
    }
  })
}
```

#### MacOS code:

```js
// Handle deep links on macOS - MUST be registered synchronously!
// Async registration may miss the initial deep link event
app.on('open-url', (event, url) => {
  event.preventDefault()
  dialog.showErrorBox('Welcome Back', `You arrived from: ${url}`)
})

// Only create window after setting up the protocol handler
app.whenReady().then(() => {
  createWindow()
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
## Troubleshooting

### App doesn't respond to deep links on first launch
- **Windows/Linux**: Ensure you're checking `process.argv` for the deep link URL when the app starts
- **macOS**: Make sure the `open-url` event listener is registered synchronously, not in an async function

### Protocol handling only works in packaged app
On macOS and Linux, protocol handling requires a packaged application. It won't work when running from the command line during development.

### Related Issues
For more information, see:
- [How to get protocol URL argument when Windows app opens for the first time?](https://github.com/electron/electron/issues/33032)

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
