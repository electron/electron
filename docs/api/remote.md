# remote

The `remote` module provides a simple way to do inter-process communication
between the renderer process and the main process.

In Electron, only GUI-unrelated modules are available in the renderer process.
Without the `remote` module, users who wanted to call a main process API in
the renderer process would have to explicitly send inter-process messages
to the main process. With the `remote` module, users can invoke methods of
main process object without explicitly sending inter-process messages,
similar to Java's
[RMI](http://en.wikipedia.org/wiki/Java_remote_method_invocation).

An example of creating a browser window in renderer process:

```javascript
var remote = require('remote');
var BrowserWindow = remote.require('browser-window');
var win = new BrowserWindow({ width: 800, height: 600 });
win.loadUrl('https://github.com');
```

Note: for the reverse (access renderer process from main process), you can use
[webContents.executeJavascript](browser-window.md#webcontents-executejavascript-code).

## Remote objects

Each object (including functions) returned by the `remote` module represents an
object in the main process (we call it a remote object or remote function).
When you invoke methods of a remote object, call a remote function, or create
a new object with the remote constructor (function), you are actually sending
synchronous inter-process messages.

In the example above, both `BrowserWindow` and `win` were remote objects and
`new BrowserWindow` didn't create a `BrowserWindow` object in the renderer process.
Instead, it created a `BrowserWindow` object in the main process and returned the
corresponding remote object in the renderer process, namely the `win` object.

## Lifetime of remote objects

Electron makes sure that as long as the remote object in the renderer process
lives (in other words, has not been garbage collected), the corresponding object
in the main process would never be released. When the remote object has been
garbage collected, the corresponding object in the main process would be
dereferenced.

If the remote object is leaked in renderer process (e.g. stored in a map but never
freed), the corresponding object in the main process would also be leaked,
so you should be very careful not to leak remote objects.

Primary value types like strings and numbers, however, are sent by copy.

## Passing callbacks to the main process

Code in the main process can accept callbacks from the renderer - for instance the `remote` module -
but you should be extremely careful when using this feature.

First, in order to avoid deadlocks, the callbacks passed to the main process
are called asynchronously. You should not expect the main process to
get the return value of the passed callbacks.

For instance you can't use a function from the renderer process in a `Array.map` called in the main process:

```javascript
// main process mapNumbers.js
exports.withRendererCallback = function(mapper) {
  return [1,2,3].map(mapper);
}

exports.withLocalCallback = function() {
  return exports.mapNumbers(function(x) {
    return x + 1; 
  });
}

// renderer process
var mapNumbers = require("remote").require("mapNumbers");

var withRendererCb = mapNumbers.withRendererCallback(function(x) {
  return x + 1;
})

var withLocalCb = mapNumbers.withLocalCallback()

console.log(withRendererCb, withLocalCb) // [true, true, true], [2, 3, 4]
```

As you can see, the renderer callback's synchronous return value was not as expected,
and didn't match the return value of an indentical callback that lives in the main process.

Second, the callbacks passed to the main process will persist until the
main process garbage-collects them.

For example, the following code seems innocent at first glance. It installs a
callback for the `close` event on a remote object:

```javascript
var remote = require('remote');
remote.getCurrentWindow().on('close', function() {
  // blabla...
});
```

But remember the callback is referenced by the main process until you
explicitly uninstall it! If you do not, each time you reload your window the callback will
be installed again, leaking one callback each restart.

To make things worse, since the context of previously installed callbacks have been released,
when the `close` event was emitted exceptions would be raised in the main process.

To avoid this problem, ensure you clean up any references to renderer callbacks passed to the main 
process. This involves cleaning up event handlers, or ensuring the main process is explicitly told to deference
callbacks that came from a renderer process that is exiting.

## remote.require(module)

* `module` String

Returns the object returned by `require(module)` in the main process.

## remote.getCurrentWindow()

Returns the [BrowserWindow](browser-window.md) object which this web page
belongs to.

## remote.getCurrentWebContents()

Returns the WebContents object of this web page.

## remote.getGlobal(name)

* `name` String

Returns the global variable of `name` (e.g. `global[name]`) in the main
process.

## remote.process

Returns the `process` object in the main process. This is the same as
`remote.getGlobal('process')`, but gets cached.
