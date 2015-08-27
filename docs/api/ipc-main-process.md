# ipc (main process)

The `ipc` module, when used in the main process, handles asynchronous and synchronous messages sent from a renderer process (web page). Messages sent from a renderer will be emitted to this module.

## Sending Messages

It is also possible to send messages from the main process to the renderer
process, see [WebContents.send](browser-window.md#webcontentssendchannel-args)
for more information.

- When sending a message, the event name is the `channel`.
- To reply a synchronous message, you need to set `event.returnValue`.
- To send an asynchronous back to the sender, you can use `event.sender.send(...)`.

An example of sending and handling messages between the render and main
processes:

```javascript
// In main process.
var ipc = require('ipc');
ipc.on('asynchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.sender.send('asynchronous-reply', 'pong');
});

ipc.on('synchronous-message', function(event, arg) {
  console.log(arg);  // prints "ping"
  event.returnValue = 'pong';
});
```

```javascript
// In renderer process (web page).
var ipc = require('ipc');
console.log(ipc.sendSync('synchronous-message', 'ping')); // prints "pong"

ipc.on('asynchronous-reply', function(arg) {
  console.log(arg); // prints "pong"
});
ipc.send('asynchronous-message', 'ping');
```

## Methods

The `ipc` module has the following method:

### `ipc.on(channel, callback)`

* `channel` String - The event name.
* `callback` Function

When the event occurs the `callback` is called with an `event` object and a
message, `arg`.

## IPC Events

The `event` object passed to the `callback` has the following methods:

### `Event.returnValue`

Set this to the value to be returned in a synchronous message.

### `Event.sender`

Returns the `WebContents` that sent the message.

### `Event.sender.send(channel[, message])`

* `channel` String - The event name.
* `message` (optional)

This sends an asynchronous message back to the render process. The `message` can be any value.
