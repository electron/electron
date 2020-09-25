# webFrameMain

> Control web pages and iframes.

Process: [Main](../glossary.md#main-process)

## Methods

These methods can be accessed from the `webFrameMain` module:

```javascript
const { webFrameMain } = require('electron')
console.log(webFrameMain)
```

### `webFrameMain.fromId(processId, routingId)`

* `processId` Integer - An `Integer` representing the id of the process which owns the frame.
* `routingId` Integer - An `Integer` representing the unique frame id in the
   current renderer process. Routing IDs can be retrieved from `WebFrameMain`
   instances (`frame.routingId`) and are also passed by frame
   specific `WebContents` navigation events (e.g. `did-frame-navigate`).

Returns `WebFrameMain` - A frame with the given process and routing IDs.

## Class: WebFrameMain

### Instance Methods

#### `frame.executeJavaScript(code, userGesture)`

* `code` String
* `userGesture` Boolean

Returns `Promise<unknown>` - A promise that resolves with the result of the executed
code or is rejected if execution throws or results in a rejected promise.

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

#### `frame.reload()`

Returns `boolean` - Whether the reload was initiated successfully. Only results in `false` when the frame has no history.

### Instance Properties

#### `frame.url` _Readonly_

A `string` representing the current URL of the frame.

#### `frame.top` _Readonly_

A `WebFrameMain | null` representing top frame in the frame hierarchy to which `frame`
belongs.

#### `frame.parent` _Readonly_

A `WebFrameMain | null` representing parent frame of `frame`, the property would be
`null` if `frame` is the top frame in the frame hierarchy.

#### `frame.frames` _Readonly_

A `WebFrameMain[]` collection containing the direct descendents of `frame`.

#### `frame.framesInSubtree` _Readonly_

A `WebFrameMain[]` collection containing every frame in the subtree of `frame`,
including itself. This can be useful when traversing through all frames.

#### `frame.frameTreeNodeId` _Readonly_

An `Integer` representing the id of the frame's internal FrameTreeNode
instance. This id is browser-global and uniquely identifies a frame that hosts
content. The identifier is fixed at the creation of the frame and stays
constant for the lifetime of the frame. When the frame is removed, the id is
not used again.

#### `frame.processId` _Readonly_

An `Integer` representing the id of the process which owns this frame.

#### `frame.routingId` _Readonly_

An `Integer` representing the unique frame id in the current renderer process.
Distinct WebFrame instances that refer to the same underlying frame will have
the same `routingId`.
