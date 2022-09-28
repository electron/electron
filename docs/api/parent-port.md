# parentPort

> Interface for communication with parent process.

Process: [Utility](../glossary.md#utility-process)

`parentPort` is an [EventEmitter][event-emitter].
_This object is not exported from the `'electron'` module. It is only available as a property of the process object in the Electron API._

```js
// Main process
const child = new UtilityProcess(path.join(__dirname, 'test.js'))
child.postMessage({ message: 'hello' })
child.on('message', (data) => {
  console.log(data) // hello world!
})

// Child process
process.parentPort.on('message', (e) => {
  process.parentPort.postMessage(`${e.data} world!`)
})
```

## Events

The `parentPort` object emits the following events:

### Event: 'message'

Returns:

* `messageEvent` Object
  * `data` any
  * `ports` MessagePortMain[]

Emitted when the process receives a message.

## Methods

### `parentPort.postMessage(message)`

* `message` any

Sends a message from the process to its parent.
