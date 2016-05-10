# ipcMain _Main Process_

> Communicate asynchronously from the main process to renderer processes.

The `ipcMain` module is an instance of the
[EventEmitter](https://nodejs.org/api/events.html) class. When used in the main
process, it handles asynchronous and synchronous messages sent from a renderer
process (web page). Messages sent from a renderer will be emitted to this
module.

## Sending Messages

It is also possible to send messages from the main process to the renderer
process, see [webContents.send][web-contents-send] for more information.

* When sending a message, the event name is the `channel`.
* To reply a synchronous message, you need to set `event.returnValue`.
* To send an asynchronous back to the sender, you can use
  `event.sender.send(...)`.

An example of sending and handling messages between the render and main
processes:

```javascript
// In main process.
const ipcMain = require('electron').ipcMain;
ipcMain.on('asynchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.sender.send('asynchronous-reply', 'pong');
});

ipcMain.on('synchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.returnValue = 'pong';
});
```

```javascript
// In renderer process (web page).
const ipcRenderer = require('electron').ipcRenderer;
console.log(ipcRenderer.sendSync('synchronous-message', 'ping')); // prints "pong"

ipcRenderer.on('asynchronous-reply', function(event, arg) {
  console.log(arg); // prints "pong"
});
ipcRenderer.send('asynchronous-message', 'ping');
```

## Listening for Messages

The `ipcMain` module has the following method to listen for events:

### `ipcMain.on(channel, listener)`

* `channel` String
* `listener` Function

Listens to `channel`, when a new message arrives `listener` would be called with
`listener(event, args...)`.

### `ipcMain.once(channel, listener)`

* `channel` String
* `listener` Function

Adds a one time `listener` function for the event. This `listener` is invoked
only the next time a message is sent to `channel`, after which it is removed.

### `ipcMain.removeListener(channel, listener)`

* `channel` String
* `listener` Function

Removes the specified `listener` from the listener array for the specified
`channel`.

### `ipcMain.removeAllListeners([channel])`

* `channel` String (optional)

Removes all listeners, or those of the specified `channel`.

## Event object

The `event` object passed to the `callback` has the following methods:

### `event.returnValue`

Set this to the value to be returned in a synchronous message.

### `event.sender`

Returns the `webContents` that sent the message, you can call
`event.sender.send` to reply to the asynchronous message, see
[webContents.send][web-contents-send] for more information.

[web-contents-send]: web-contents.md#webcontentssendchannel-arg1-arg2-
