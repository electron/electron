# ServiceWorkerMain

> An instance of a Service Worker representing a version of a script for a given scope.

Process: [Main](../glossary.md#main-process)

## Class: ServiceWorkerMain

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### Instance Methods

#### `serviceWorker.isDestroyed()` _Experimental_

Returns `boolean` - Whether the service worker has been destroyed.

#### `serviceWorker.send(channel, ...args)` _Experimental_

- `channel` string
- `...args` any[]

Send an asynchronous message to the service worker process via `channel`, along with
arguments. Arguments will be serialized with the [Structured Clone Algorithm][SCA],
just like [`postMessage`][], so prototype chains will not be included.
Sending Functions, Promises, Symbols, WeakMaps, or WeakSets will throw an exception.

The service worker process can handle the message by listening to `channel` with the
[`ipcRenderer`](ipc-renderer.md) module.

#### `serviceWorker.startWorker()` _Experimental_

Returns `Promise<void>` - Resolves when the service worker has started.

Starts the service worker or does nothing if already running.

```js
const { app, session } = require('electron')
const { serviceWorkers } = session.defaultSession

// Collect service workers
const versions = Object.values(serviceWorkers.getAllRunning()).map(({ versionId }) => {
  return serviceWorkers.getWorkerFromVersionID(versionId)
})

app.on('browser-window-created', (event, window) => {
  for (const serviceWorker of versions) {
    if (serviceWorker.isDestroyed()) continue

    // Request starting worker if it's not running
    serviceWorker.startWorker().then(() => {
      serviceWorker.send('window-created', { windowId: window.id })
    })
  }
})
```

#### `serviceWorker.startTask()` _Experimental_

Returns `Object`:

- `end` Function - Method to call when the task has ended. If never called, the service won't terminate while otherwise idle.

Initiate a task to keep the service worker alive until ended.

```js
const { session } = require('electron')
const { serviceWorkers } = session.defaultSession

async function fetchData () {}

const versionId = 0
const serviceWorker = serviceWorkers.getWorkerFromVersionID(versionId)

serviceWorker?.ipc.handle('request-data', async () => {
  // Keep service worker alive while fetching data
  const task = serviceWorker.startTask()
  try {
    return await fetchData()
  } finally {
    // Mark task as ended to allow service worker to terminate when idle.
    task.end()
  }
})
```

### Instance Properties

#### `serviceWorker.ipc` _Readonly_ _Experimental_

An [`IpcMainServiceWorker`](ipc-main-service-worker.md) instance scoped to the service worker.

#### `serviceWorker.scope` _Readonly_ _Experimental_

A `string` representing the scope URL of the service worker.

#### `serviceWorker.versionId` _Readonly_ _Experimental_

A `number` representing the ID of the specific version of the service worker script in its scope.

[SCA]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm
[`postMessage`]: https://developer.mozilla.org/en-US/docs/Web/API/Window/postMessage
