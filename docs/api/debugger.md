## Class: Debugger

> An alternate transport for Chrome's remote debugging protocol.

Process: [Main](../tutorial/quick-start.md#main-process)

Chrome Developer Tools has a [special binding][rdp] available at JavaScript
runtime that allows interacting with pages and instrumenting them.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

try {
  win.webContents.debugger.attach('1.1')
} catch (err) {
  console.log('Debugger attach failed : ', err)
}

win.webContents.debugger.on('detach', (event, reason) => {
  console.log('Debugger detached due to : ', reason)
})

win.webContents.debugger.on('message', (event, method, params) => {
  if (method === 'Network.requestWillBeSent') {
    if (params.request.url === 'https://www.github.com') {
      win.webContents.debugger.detach()
    }
  }
})

win.webContents.debugger.sendCommand('Network.enable')
```

### Instance Methods

#### `debugger.attach([protocolVersion])`

* `protocolVersion` String (optional) - Requested debugging protocol version.

Attaches the debugger to the `webContents`.

#### `debugger.isAttached()`

Returns `Boolean` - Whether a debugger is attached to the `webContents`.

#### `debugger.detach()`

Detaches the debugger from the `webContents`.

#### `debugger.sendCommand(method[, commandParams, callback])`

* `method` String - Method name, should be one of the methods defined by the
   remote debugging protocol.
* `commandParams` Object (optional) - JSON object with request parameters.
* `callback` Function (optional) - Response
  * `error` Object - Error message indicating the failure of the command.
  * `result` Any - Response defined by the 'returns' attribute of
     the command description in the remote debugging protocol.

Send given command to the debugging target.

### Instance Events

#### Event: 'detach'

* `event` Event
* `reason` String - Reason for detaching debugger.

Emitted when debugging session is terminated. This happens either when
`webContents` is closed or devtools is invoked for the attached `webContents`.

#### Event: 'message'

* `event` Event
* `method` String - Method name.
* `params` Object - Event parameters defined by the 'parameters'
   attribute in the remote debugging protocol.

Emitted whenever debugging target issues instrumentation event.

[rdp]: https://developer.chrome.com/devtools/docs/debugger-protocol
[`webContents.findInPage`]: web-contents.md#contentsfindinpagetext-options
