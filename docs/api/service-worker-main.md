# ServiceWorkerMain

> An instance of a Service Worker representing a version of a script for a given scope.

Process: [Main](../glossary.md#main-process)

## Class: ServiceWorkerMain

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

### Instance Methods

#### `serviceWorker.isDestroyed()` _Experimental_

Returns `boolean` - Whether the service worker has been destroyed.

#### `serviceWorker.startTask()` _Experimental_

Returns `Object`:

- `end` Function - Method to call when the task has ended. If never called, the service won't terminate while otherwise idle.

Initiate a task to keep the service worker alive until ended.

### Instance Properties

#### `serviceWorker.scope` _Readonly_ _Experimental_

A `string` representing the scope URL of the service worker.

#### `serviceWorker.versionId` _Readonly_ _Experimental_

A `number` representing the ID of the specific version of the service worker script in its scope.
