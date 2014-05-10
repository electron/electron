# ipc (browser)

Handles asynchronous and synchronous message sent from web page.

The messages sent from web page would be emitted to this module, the event name
is the `channel` when sending message. To reply a synchronous message, you need
to set `event.returnValue`, to send an asynchronous back to the sender, you can
use `event.sender.send(...)`.

It's also possible to send messages from browser side to web pages, see
[WebContents.send](browser-window.md#webcontentssendchannel-args) for more.

An example of sending and handling messages:

```javascript
// In browser.
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
// In web page.
var ipc = require('ipc');
console.log(ipc.sendSync('synchronous-message', 'ping')); // prints "pong"

ipc.on('asynchronous-reply', function(arg) {
  console.log(arg); // prints "pong"
});
ipc.send('asynchronous-message', 'ping');
```

## Class: Event

### Event.returnValue

Assign to this to return an value to synchronous messages.

### Event.sender

The `WebContents` of the web page that has sent the message.
