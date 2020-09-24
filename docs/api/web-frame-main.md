# webFrameMain

> Control web pages and iframes.

Process: [Main](../glossary.md#main-process)

## Methods

The `WebFrame` class has the following instance methods:

### `webFrame.executeJavaScript(code, userGesture)`

* `code` String
* `userGesture` Boolean - Default is `false`.

Returns `Promise<unknown>` - A promise that resolves with the result of the executed
code or is rejected if execution throws or results in a rejected promise.

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

### `webFrame.reload()`

Returns `boolean` - Whether the reload was initiated successfully. Only results in `false` when the frame has no history.

## Properties

### `webFrame.url` _Readonly_

A `string` representing the current URL of the frame.

### `webFrame.top` _Readonly_

A `WebFrame | null` representing top frame in the frame hierarchy to which `webFrame`
belongs.

### `webFrame.parent` _Readonly_

A `WebFrame | null` representing parent frame of `webFrame`, the property would be
`null` if `webFrame` is the top frame in the frame hierarchy.

### `webFrame.frames` _Readonly_

A `WebFrame[]` collection representing the child frames of `webFrame`.

### `webFrame.frameTreeNodeId` _Readonly_

An `Integer` which needs documentation (TODO).

### `webFrame.processId` _Readonly_

An `Integer` which needs documentation (TODO).

### `webFrame.routingId` _Readonly_

An `Integer` representing the unique frame id in the current renderer process.
Distinct WebFrame instances that refer to the same underlying frame will have
the same `routingId`.
