# UtilityProcess

`UtilityProcess` is the equivalent of [`child_process.fork`][] API from Node.js
but instead uses [Services API][] from Chromium to create child processes with
Node.js and Message ports enabled.

`UtilityProcess` is an [EventEmitter][event-emitter].

## Class: UtilityProcess

> Child process with Node.js integration spawned by Chromium.

Process: [Main](../glossary.md#main-process)<br />

### `new UtilityProcess(modulePath[, args][, options])`

* `modulePath` string - Absolute path to the script that should run as entrypoint in the child process.
* `args` string[] (optional) - List of string arguments that will be available as `process.argv`
  in the child process.
* `options` Object (optional)
  * `env` Object - Environment key-value pairs. Default is `process.env`.
  * `execArgv` string[] - List of string arguments passed to the executable. Default is `process.execArgv`.
  * `serviceName` string - Name of the process that will appear in `name` property of
    [`child-process-gone` event of `app`](app.md#event-child-process-gone).
    Default is `node.mojom.NodeService`.
  * `allowLoadingUnsignedLibraries` boolean (optional) _macOS_ - With this flag, the utility process will be
    launched via the `Electron Helper (Plugin).app` helper executable on macOS, which can be
    codesigned with `com.apple.security.cs.disable-library-validation` and
    `com.apple.security.cs.allow-unsigned-executable-memory` entitlements. This will allow the utility process
    to load unsigned libraries. Unless you specifically need this capability, it is best to leave this disabled.
    Default is `false`.

### Instance Methods

#### `child.postMessage(message, [transfer])`

* `message` any
* `transfer` MessagePortMain[] (optional)

Send a message to the child process, optionally transferring ownership of
zero or more [`MessagePortMain`][] objects.

For example:

```js
// Main process
const { port1, port2 } = new MessageChannelMain()
const child = new UtilityProcess(path.join(__dirname, 'test.js'))
child.postMessage({ message: 'hello' }, [port1])

// Child process
process.parentPort.once('message', (e) => {
  const [port] = e.ports
  // ...
})
```

#### `child.kill([signal])`

* `signal` (string | Integer) (optional)

Returns `boolean`

Sends a signal to the child process. If no argument is given,
the process will be sent the 'SIGTERM' signal. This function returns true
if kill succeeds, and false otherwise.

### Instance Properties

#### `child.pid`

A `Integer` representing the process identifier (PID) of the child process.
If the child process fails to spawn due to errors, then the value is `undefined`.

### Instance Events

#### Event: 'spawn'

Emitted once the child process has spawned successfully.

#### Event: 'exit'

Returns:

* `event` Event
* `code` number

Emitted when the child process has a normal termination.
For other abnormal exit cases listen to the [`child-process-gone` event of `app`](app.md#event-child-process-gone).

[`child_process.fork`]: https://nodejs.org/dist/latest-v16.x/docs/api/child_process.html#child_processforkmodulepath-args-options
[Services API]: https://chromium.googlesource.com/chromium/src/+/master/docs/mojo_and_services.md
