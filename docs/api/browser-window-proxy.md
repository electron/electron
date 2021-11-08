## Class: BrowserWindowProxy

> Manipulate the child browser window

Process: [Renderer](../glossary.md#renderer-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

The `BrowserWindowProxy` object is returned from `window.open` and provides
limited functionality with the child window.

### Instance Methods

The `BrowserWindowProxy` object has the following instance methods:

#### `win.blur()`

Removes focus from the child window.

#### `win.close()`

Forcefully closes the child window without calling its unload event.

#### `win.eval(code)`

* `code` string

Evaluates the code in the child window.

#### `win.focus()`

Focuses the child window (brings the window to front).

#### `win.print()`

Invokes the print dialog on the child window.

#### `win.postMessage(message, targetOrigin)`

* `message` any
* `targetOrigin` string

Sends a message to the child window with the specified origin or `*` for no
origin preference.

In addition to these methods, the child window implements `window.opener` object
with no properties and a single method.

### Instance Properties

The `BrowserWindowProxy` object has the following instance properties:

#### `win.closed`

A `boolean` that is set to true after the child window gets closed.
