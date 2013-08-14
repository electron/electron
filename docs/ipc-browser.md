## Synopsis

The `ipc` module allows developers to send asynchronous messages to renderers. To avoid possible dead-locks, it's not allowed to send synchronous messages in browser.

## Event: 'message'

* `processId` Integer
* `routingId` Integer

Emitted when renderer sent a message to the browser.

## Event: 'sync-message'

* `event` Object
* `processId` Integer
* `routingId` Integer

Emitted when renderer sent a synchronous message to the browser. The receiver should store the result in `event.result`.

**Note:** Due to the limitation of `EventEmitter`, returning value in the event handler has no effect, so we have to store the result by using the `event` parameter.

## ipc.send(processId, routingId, [args...])

* `processId` Integer
* `routingId` Integer

Send `args...` to the renderer specified by `processId` and `routingId` and return immediately, the renderer should handle the message by listening to the `message` event.

## ipc.sendChannel(processId, routingId, channel, [args...])

* `processId` Integer
* `routingId` Integer
* `channel` String

This is the same with ipc.send, except that the renderer should listen to the `channel` event. The ipc.send(processId, routingId, args...) can be seen as ipc.sendChannel(processId, routingId, 'message', args...).

**Note:** If the the first argument (e.g. `processId`) is a `BrowserWindow`, `ipc.sendChannel` would automatically get the `processId` and `routingId` from it, so you can send a message to window like this:

```javascript
ipc.sendChannel(browserWindow, 'message', ...);
```