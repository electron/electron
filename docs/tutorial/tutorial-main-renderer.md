---
title: 'Main and Renderer Process Communication'
description: 'This guide will step you through the process of creating a barebones Hello World app in Electron, similar to electron/electron-quick-start.'
slug: node-access
hide_title: false
---

:::info Tutorial parts
This is part 3 of the Electron tutorial. The other parts are:

1. [Prerequisites]
1. [Scaffolding]
1. [Main and Renderer process communication][main-renderer]
1. [Adding Features][features]
1. [Application Distribution]
1. [Code Signing]
1. [Updating Your Application][updates]

:::

At the end of this part you will have an Electron application that uses
a preload script to safely expose features from the main process
into the renderer one.

Electron's main process is a Node.js process that has full OS access.
On top of [Electron's modules][modules], you can also access [Node.js ones][node-api],
as well as any compatible code downloaded via `npm`.

If you are not familiar with Node.js, we recommend you to first
read [this guide][node-guide] before continuing.

## Using `contextBridge` and a preload script

:::tip Further reading 📚
This is your last reminder to look at [Electron's Process Model][process-model] to
better understand all of this!
:::

In Electron, the main process is the one that has access to Node.js, and the
renderer process to the web APIs. This means it is not possible to access the
Node.js APIs directly from the renderer process, nor the DOM from the main process.
They're in entirely different processes with different APIs!

On top of that, we are using `sandbox: true` (the default starting on Electron 18)
when creating our renderer process.
The [sandbox] limits the harm that malicious code can cause by limiting access to
most system resources. With sandbox enabled, your web content is completely isolated.
To add features to your renderer process that require "privileged access", you have
to use a [`preload` script][preload-script] in conjunction with the
[`contextBridge` API][contextbridge].

We are going to create a `preload` script that exposes the versions of Chrome, Node, and
Electron into the renderer. To do this, add a new file `preload.js` with the
following:

```js title="preload.js"
const { contextBridge } = require('electron')

contextBridge.exposeInMainWorld('versions', {
  node: () => process.versions.node,
  chrome: () => process.versions.chrome,
  electron: () => process.versions.electron
  // we can also expose variables, not just functions
})
```

:::tip Further reading 📚
To know more about this security feature, we recommend you to read the
[Context Isolation guide][context-isolation].
:::

The above code accesses the Node.js `process.versions` object and exposes some of them in
the renderer process in a [global](https://developer.mozilla.org/en-US/docs/Glossary/Global_object) object called `versions`.

To attach this script to your renderer process, pass in the path to your preload script
to the `webPreferences.preload` option in the `BrowserWindow` constructor:

```js {9} title="main.js"
const { app, BrowserWindow } = require('electron')
const path = require('path')

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      sandbox: true
    }
  })

  win.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow()
})
```

:::tip
There are two Node.js concepts that are used here:

- The [`__dirname`][dirname] string points to the path of the currently executing script
  (in this case, your project's root folder).
- The [`path.join`][path-join] API joins multiple path segments together, creating a
  combined path string that works across all platforms.

If you are not familiar with Node.js, we recommend you to read [this guide][node-guide].
:::

At this point, the renderer context has access to `versions`, so let's make that information
available to the user. Create a new file `renderer.js` that contains the following code:

```js title="renderer.js"
const information = document.getElementById('info')
information.innerText = `This app is using Chrome (v${versions.chrome()}), Node.js (v${versions.node()}), and Electron (v${versions.electron()})`
```

And modify `index.html` to match the following:

```html {18,20} title="index.html"
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
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
    <p id="info"></p>
  </body>
  <script src="./renderer.js"></script>
</html>
```

We have created a paragraph with `info` as its id, and included `renderer.js`.

:::warning
The reason why we do not write the JavaScript directly in the HTML between `<script></script>`
is because we have a Content Security Policy (CSP) that will prevent its execution.
To know more you can visit [MDN's CSP documentation][mdn-csp].
:::

After following the above steps, you should have a fully functional Electron application that
has a message similar to the following one (probably with different versions):

> This app is using Chrome (v94.0.4606.81), Node.js (v16.5.0), and Electron (v15.2.0)

And the code should be similar to this:

```fiddle docs/latest/fiddles/tutorial-main-renderer

```

<!-- TODO (@erickzhao): Write the IPC part -->

<!-- Links -->

[advanced-installation]: ./installation.md
[application debugging]: ./application-debugging.md
[app]: ../api/app.md
[app-ready]: ../api/app.md#event-ready
[app-when-ready]: ../api/app.md#appwhenready
[browser-window]: ../api/browser-window.md
[commonjs]: https://nodejs.org/docs/latest/api/modules.html#modules_modules_commonjs_modules
[compound task]: https://code.visualstudio.com/Docs/editor/tasks#_compound-tasks
[contextbridge]: ../api/context-bridge.md
[context-isolation]: ./context-isolation.md
[devtools-extension]: ./devtools-extension.md
[dirname]: https://nodejs.org/api/modules.html#modules_dirname
[mdn-csp]: https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP
[modules]: ../api/app.md
[node-api]: https://nodejs.org/dist/latest/docs/api/
[node-guide]: https://nodejs.dev/learn
[package-json-main]: https://docs.npmjs.com/cli/v7/configuring-npm/package-json#main
[package-scripts]: https://docs.npmjs.com/cli/v7/using-npm/scripts
[path-join]: https://nodejs.org/api/path.html#path_path_join_paths
[preload-script]: ./sandbox.md#preload-scripts
[process-model]: ./process-model.md
[react]: https://reactjs.org
[sandbox]: ./sandbox.md
[webpack]: https://webpack.js.org

<!-- Tutorial links -->

[prerequisites]: ./tutorial-prerequisites.md
[scaffolding]: ./tutorial-scaffolding.md
[main-renderer]: ./tutorial-main-renderer.md
[features]: ./tutorial-adding-features.md
[application distribution]: ./distribution-overview.md
[code signing]: ./code-signing.md
[updates]: ./updates.md
