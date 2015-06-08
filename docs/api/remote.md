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

Note: for the reverse (access renderer process from main process), you can use [webContents.executeJavascript](https://github.com/atom/electron/blob/master/docs/api/browser-window.md#browserwindowwebcontents).

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

Some APIs in the main process accept callbacks, and it would be tempting to
pass callbacks when calling a remote function. The `remote` module does support
doing this, but you should also be extremely careful with this.

First, in order to avoid deadlocks, the callbacks passed to the main process
are called asynchronously, so you should not expect the main process to
get the return value of the passed callbacks.

Second, the callbacks passed to the main process will not get released
automatically after they are called. Instead, they will persistent until the
main process garbage-collects them.

For example, the following code seems innocent at first glance. It installs a
callback for the `close` event on a remote object:

```javascript
var remote = require('remote');
remote.getCurrentWindow().on('close', function() {
  // blabla...
});
```

The problem is that the callback would be stored in the main process until you
explicitly uninstall it! So each time you reload your window, the callback would
be installed again and previous callbacks would just leak. To make things
worse, since the context of previously installed callbacks have been released,
when the `close` event was emitted, exceptions would be raised in the main process.

Generally, unless you are clear what you are doing, you should always avoid
passing callbacks to the main process.

## Remote buffer

An instance of node's `Buffer` is an object, so when you get a `Buffer` from
the main process, what you get is indeed a remote object (let's call it remote
buffer), and everything would just follow the rules of remote objects.

However you should remember that although a remote buffer behaves like the real
`Buffer`, it's not a `Buffer` at all. If you pass a remote buffer to node APIs
that accept a `Buffer`, you should assume the remote buffer would be treated
like a normal object, instead of a `Buffer`.

For example, you can call `BrowserWindow.capturePage` in the renderer process, which
returns a `Buffer` by calling the passed callback:

```javascript
var remote = require('remote');
var fs = require('fs');
remote.getCurrentWindow().capturePage(function(image) {
  var buf = image.toPng();
  fs.writeFile('/tmp/screenshot.png', buf, function(err) {
    console.log(err);
  });
});
```

But you may be surprised to find that the file written was corrupted. This is
because when you called `fs.writeFile`, thinking that `buf` was a `Buffer` when
in fact it was a remote buffer, and it was converted to string before it was
written to the file. Since `buf` contained binary data and could not be represented
by a UTF-8 encoded string, the written file was corrupted.

The work-around is to write the `buf` in the main process, where it is a real
`Buffer`:

```javascript
var remote = require('remote');
remote.getCurrentWindow().capturePage(function(image) {
  var buf = image.toPng();
  remote.require('fs').writeFile('/tmp/screenshot.png', buf, function(err) {
    console.log(err);
  });
});
```

The same thing could happen for all native types, but usually it would just
throw a type error. The `Buffer` deserves your special attention because it
might be converted to string, and APIs accepting `Buffer` usually accept string
too, and data corruption could happen when it contains binary data.

## remote.require(module)

* `module` String

Returns the object returned by `require(module)` in the main process.

## remote.getCurrentWindow()

Returns the [BrowserWindow](browser-window.md) object which this web page
belongs to.

## remote.getCurrentWebContent()

Returns the WebContents object of this web page.

## remote.getGlobal(name)

* `name` String

Returns the global variable of `name` (e.g. `global[name]`) in the main
process.

## remote.process

Returns the `process` object in the main process. This is the same as
`remote.getGlobal('process')`, but gets cached.
