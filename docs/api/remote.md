# remote

> Use main process modules from the renderer process.

The `remote` module provides a simple way to do inter-process communication
(IPC) between the renderer process (web page) and the main process.

In Electron, GUI-related modules (such as `dialog`, `menu` etc.) are only
available in the main process, not in the renderer process. In order to use them
from the renderer process, the `ipc` module is necessary to send inter-process
messages to the main process. With the `remote` module, you can invoke methods
of the main process object without explicitly sending inter-process messages,
similar to Java's [RMI][rmi]. An example of creating a browser window from a
renderer process:

```javascript
const {BrowserWindow} = require('electron').remote;

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadURL('https://github.com');
```

**Note:** for the reverse (access the renderer process from the main process),
you can use [webContents.executeJavascript](web-contents.md#webcontentsexecutejavascriptcode-usergesture).

## Remote Objects

Each object (including functions) returned by the `remote` module represents an
object in the main process (we call it a remote object or remote function).
When you invoke methods of a remote object, call a remote function, or create
a new object with the remote constructor (function), you are actually sending
synchronous inter-process messages.

In the example above, both `BrowserWindow` and `win` were remote objects and
`new BrowserWindow` didn't create a `BrowserWindow` object in the renderer
process. Instead, it created a `BrowserWindow` object in the main process and
returned the corresponding remote object in the renderer process, namely the
`win` object.

Please note that only [enumerable properties](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Enumerability_and_ownership_of_properties) are accessible via remote.

## Lifetime of Remote Objects

Electron makes sure that as long as the remote object in the renderer process
lives (in other words, has not been garbage collected), the corresponding object
in the main process will not be released. When the remote object has been
garbage collected, the corresponding object in the main process will be
dereferenced.

If the remote object is leaked in the renderer process (e.g. stored in a map but
never freed), the corresponding object in the main process will also be leaked,
so you should be very careful not to leak remote objects.

Primary value types like strings and numbers, however, are sent by copy.

## Passing callbacks to the main process

Code in the main process can accept callbacks from the renderer - for instance
the `remote` module - but you should be extremely careful when using this
feature.

First, in order to avoid deadlocks, the callbacks passed to the main process
are called asynchronously. You should not expect the main process to
get the return value of the passed callbacks.

For instance you can't use a function from the renderer process in an
`Array.map` called in the main process:

```javascript
// main process mapNumbers.js
exports.withRendererCallback = (mapper) => {
  return [1,2,3].map(mapper);
};

exports.withLocalCallback = () => {
  return exports.mapNumbers(x => x + 1);
};
```

```javascript
// renderer process
const mapNumbers = require('remote').require('./mapNumbers');

const withRendererCb = mapNumbers.withRendererCallback(x => x + 1);

const withLocalCb = mapNumbers.withLocalCallback();

console.log(withRendererCb, withLocalCb); // [true, true, true], [2, 3, 4]
```

As you can see, the renderer callback's synchronous return value was not as
expected, and didn't match the return value of an identical callback that lives
in the main process.

Second, the callbacks passed to the main process will persist until the
main process garbage-collects them.

For example, the following code seems innocent at first glance. It installs a
callback for the `close` event on a remote object:

```javascript
remote.getCurrentWindow().on('close', () => {
  // blabla...
});
```

But remember the callback is referenced by the main process until you
explicitly uninstall it. If you do not, each time you reload your window the
callback will be installed again, leaking one callback for each restart.

To make things worse, since the context of previously installed callbacks has
been released, exceptions will be raised in the main process when the `close`
event is emitted.

To avoid this problem, ensure you clean up any references to renderer callbacks
passed to the main process. This involves cleaning up event handlers, or
ensuring the main process is explicitly told to deference callbacks that came
from a renderer process that is exiting.

## Accessing built-in modules in the main process

The built-in modules in the main process are added as getters in the `remote`
module, so you can use them directly like the `electron` module.

```javascript
const app = remote.app;
```

## Methods

The `remote` module has the following methods:

### `remote.require(module)`

* `module` String

Returns the object returned by `require(module)` in the main process.

### `remote.getCurrentWindow()`

Returns the [`BrowserWindow`](browser-window.md) object to which this web page
belongs.

### `remote.getCurrentWebContents()`

Returns the [`WebContents`](web-contents.md) object of this web page.

### `remote.getGlobal(name)`

* `name` String

Returns the global variable of `name` (e.g. `global[name]`) in the main
process.

### `remote.process`

Returns the `process` object in the main process. This is the same as
`remote.getGlobal('process')` but is cached.

[rmi]: http://en.wikipedia.org/wiki/Java_remote_method_invocation
