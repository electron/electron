# Multithreading

With [Web Workers][web-workers], it is possible to run JavaScript in OS-level
threads.

## Multi-threaded Node.js

In Electron, it is supported to use Node.js integration in Web Workers. To do
so the `nodeIntegrationInWorker` option should be set to `true` in
`webPreferences`.

```javascript
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegrationInWorker: true
  }
})
```

The `nodeIntegrationInWorker` can be used independent of `nodeIntegration`, but
`sandbox` must not be set to `true`.

## Available APIs

All built-in modules of Node.js are supported in Web Workers, and `asar`
archives can still be read with Node.js APIs. However non of Electron's built-in
modules can be used in multi-threaded environment.

## Native Node.js modules

Any native Node.js module can be loaded directly in Web Workers, but it is
strongly recommended not to do so. Most existing native modules have been
written assuming single-thread environment, using them in a Web Workers will
lead to crashes and memory corruptions.

Even when using a thread-safe native Node.js module, it should be noticed that
the `process.dlopen` function is not thread safe, so loading a native module
in Web Workers is not thread safe.

The only way to load a native module safely for now, is to make sure the app
loads no native modules after the Web Workers get started.

```javascript
process.dlopen = () => {
  throw new Error('Load native module is not safe')
}
new Worker('script.js')
```

[web-workers]: https://developer.mozilla.org/en/docs/Web/API/Web_Workers_API/Using_web_workers
