## Class: IpcMainServiceWorker

> Communicate asynchronously from the main process to service workers.

Process: [Main](../glossary.md#main-process)

### Instance Methods

#### `ipcMainServiceWorker.on(channel, listener)`

* `channel` string
* `listener` Function
  * `event` [IpcMainServiceWorkerEvent][ipc-main-service-worker-event]
  * `...args` any[]

Listens to `channel`, when a new message arrives `listener` would be called with
`listener(event, args...)`.

#### `ipcMainServiceWorker.once(channel, listener)`

* `channel` string
* `listener` Function
  * `event` [IpcMainServiceWorkerEvent][ipc-main-service-worker-event]
  * `...args` any[]

Adds a one time `listener` function for the event. This `listener` is invoked
only the next time a message is sent to `channel`, after which it is removed.

#### `ipcMainServiceWorker.removeListener(channel, listener)`

* `channel` string
* `listener` Function
  * `...args` any[]

Removes the specified `listener` from the listener array for the specified
`channel`.

#### `ipcMainServiceWorker.removeAllListeners([channel])`

* `channel` string (optional)

Removes listeners of the specified `channel`.

#### `ipcMainServiceWorker.handle(channel, listener)`

* `channel` string
* `listener` Function\<Promise\<any\> | any\>
  * `event` [IpcMainServiceWorkerInvokeEvent][ipc-main-service-worker-invoke-event]
  * `...args` any[]

#### `ipcMainServiceWorker.handleOnce(channel, listener)`

* `channel` string
* `listener` Function\<Promise\<any\> | any\>
  * `event` [IpcMainServiceWorkerInvokeEvent][ipc-main-service-worker-invoke-event]
  * `...args` any[]

Handles a single `invoke`able IPC message, then removes the listener. See
`ipcMainServiceWorker.handle(channel, listener)`.

#### `ipcMainServiceWorker.removeHandler(channel)`

* `channel` string

Removes any handler for `channel`, if present.

[ipc-main-service-worker-event]:../api/structures/ipc-main-service-worker-event.md
[ipc-main-service-worker-invoke-event]:../api/structures/ipc-main-service-worker-invoke-event.md
