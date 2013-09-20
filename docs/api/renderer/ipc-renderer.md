# ipc (renderer)

The `ipc` module provides a few methods so you can send synchronous and
asynchronous messages to the browser, and also receive messages sent from
browser. If you want to make use of modules of browser from renderer, you
might consider using the [remote](remote.md) module.

An example of echoing messages between browser and renderer:

```javascript
// In browser:
var ipc = require('ipc');
ipc.on('message', function(processId, routingId, m) {
  ipc.send(processId, routingId, m);
});
```

```javascript
// In renderer:
var ipc = require('ipc');
ipc.on('message', function(m) {
  console.log('Received message', m);
});
ipc.send('Hello world');
```

An example of sending synchronous message from renderer to browser:

```javascript
// In browser:
var ipc = require('ipc');
ipc.on('browser-data-request', function(event, processId, routingId, message) {
  event.returnValue = 'THIS SOME DATA FROM THE BROWSER';
});
```

```javascript
// In renderer:
var ipc = require('ipc');
console.log(ipc.sendChannelSync('browser-data-request', 'THIS IS FROM THE RENDERER'));
```

## Event: 'message'

Emitted when browser sent a message to this window.

## ipc.send([args...])

Send all arguments to the browser and return immediately, the browser should
handle the message by listening to the `message` event.

## ipc.sendSync([args...])

Send all arguments to the browser synchronously, and returns the result sent
from browser. The browser should handle the message by listening to the
`sync-message` event.

**Note:** Usually developers should never use this API, since sending
synchronous message would block the browser.

## ipc.sendChannel(channel, [args...])

* `channel` String

This is the same with `ipc.send`, except that the browser should listen to the
`channel` event. The `ipc.send(args...)` can be seen as
`ipc.sendChannel('message', args...)`.


## ipc.sendChannelSync(channel, [args...])

* `channel` String

This is the same with `ipc.sendSync`, except that the browser should listen to
the `channel` event. The `ipc.sendSync(args...)` can be seen as
`ipc.sendChannelSync('sync-message', args...)`.

**Note:** Usually developers should never use this API, since sending
synchronous message would block the browser.
