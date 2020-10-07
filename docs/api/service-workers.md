## Class: ServiceWorkers

> Query and receive events from a sessions active service workers.

Process: [Main](../glossary.md#main-process)

Instances of the `ServiceWorkers` class are accessed by using `serviceWorkers` property of
a `Session`.

For example:

```javascript
const { session } = require('electron')

// Get all service workers.
console.log(session.defaultSession.serviceWorkers.getAllRunning())

// Handle logs and get service worker info
session.defaultSession.serviceWorkers.on('console-message', (event, messageDetails) => {
  console.log(
    'Got service worker message',
    messageDetails,
    'from',
    session.defaultSession.serviceWorkers.getFromVersionID(messageDetails.versionId)
  )
})
```

### Instance Events

The following events are available on instances of `ServiceWorkers`:

#### Event: 'console-message'

Returns:

* `event` Event
* `messageDetails` Object - Information about the console message
  * `message` string - The actual console message
  * `versionId` number - The version ID of the service worker that sent the log message
  * `source` string - The type of source for this message.  Can be `javascript`, `xml`, `network`, `console-api`, `storage`, `app-cache`, `rendering`, `security`, `deprecation`, `worker`, `violation`, `intervention`, `recommendation` or `other`.
  * `level` number - The log level, from 0 to 3. In order it matches `verbose`, `info`, `warning` and `error`.
  * `sourceUrl` string - The URL the message came from
  * `linenumber` number - The line number of the source that triggered this console message

Emitted when a service worker logs something to the console.

### Instance Methods

The following methods are available on instances of `ServiceWorkers`:

#### `serviceWorkers.getAllRunning()`

Returns `Record<number, ServiceWorkerInfo>` - A [ServiceWorkerInfo](structures/service-worker-info.md) object where the keys are the service worker version ID and the values are the information about that service worker.

#### `serviceWorkers.getFromVersionID(versionId)`

* `versionId` number

Returns [`ServiceWorkerInfo`](structures/service-worker-info.md) - Information about this service worker

If the service worker does not exist or is not running this method will throw an exception.
