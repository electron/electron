# Processes

## Main Process

The main process, commonly a file named `main.js`, is the entry point to every
Electron app. It controls the life of the app, from open to close. It also
manages native elements such as the Menu, Menu Bar, Dock, Tray, etc.  The
main process is responsible for creating each new renderer process in the app.
The full Node API is built in.

Every app's main process file is specified in the `main` property in
`package.json`. This is how `electron .` knows what file to execute at startup.

In Chromium, this process is referred to as the "browser process". It is
renamed in Electron to avoid confusion with renderer processes.

## Renderer Process

The renderer process is a browser window in your app. Unlike the main process,
there can be multiple of these and each is run in a separate process.
They can also be hidden.

In normal browsers, web pages usually run in a sandboxed environment and are not
allowed access to native resources. Electron users, however, have the power to
use Node.js APIs in web pages allowing lower level operating system
interactions.

## Differences Between the Main Process and Renderer Process

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
renderer processes. Like [`ipcRenderer`](api/ipc-renderer.md) and
[`ipcMain`](api/ipc-main.md) modules for sending messages, and the
[remote](api/remote.md) module for RPC style communication. There is also
an FAQ entry on [how to share data between web pages][share-data].

### The `process` Global

Like Node.js, all Electron processes have a `process` object which is 
available in both the main and renderer processes. It contains information 
about the currently running process. It inherits from the Node.js `process` 
object but includes additional information like the current versions of 
Chromium, V8, Node.js, etc.

For more info, see the [`process` API documentation](api/process.md).

[share-data]: faq.md#how-to-share-data-between-web-pages

