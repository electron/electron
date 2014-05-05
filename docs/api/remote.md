# remote

The `remote` module provides a simple way to do inter-process communication
between renderer process and browser process.

In atom-shell, all GUI related modules are only available in the browser
process, if users want to call an browser side API in the renderer process
, they usually would have to explicitly send inter-process messages to the
browser process. But with the `remote` module, users can invoke methods of
objects living in browser process without sending inter-process messages
directly, like Java's
[RMI](http://en.wikipedia.org/wiki/Java_remote_method_invocation).

An example of creating a browser window in renderer process:

```javascript
var remote = require('remote');
var BrowserWindow = remote.require('browser-window');
var win = new BrowserWindow({ width: 800, height: 600 });
win.loadUrl('https://github.com');
```

## Remote objects

Each object (including function) returned by `remote` module represents an
object in browser process (we call it remote object or remote function), when
you invoke methods of a remote object, or call a remote function, or even create
a new object with the remote constructor (function), you are actually sending
synchronous inter-process messages.

In the example above, both `BrowserWindow` and `win` were remote objects. And
`new BrowserWindow` didn't create a `BrowserWindow` object in renderer process,
instead it created a `BrowserWindow` object in browser process, and returned the
corresponding remote object in renderer process, namely the `win` object.

## Lifetime of remote objects

Atom-shell makes sure that as long as the remote object in renderer process
lives (in other words, has not been garbage collected), the corresponding object
in browser process would never be released. And when the remote object has been
garbage collected, the corresponding object in browser process would be
dereferenced.

But it also means that, if the remote object is leaked in renderer process, like
being stored in a map but never got freed, the corresponding object in browser
process would also be leaked too. So you should be very careful not to leak
remote objects.

Primary value types like strings and numbers, however, are sent by copy.

## Passing callbacks to browser

Some APIs in browser process accepts callbacks, and it would be attempting to
pass callbacks when calling a remote function. Yes `remote` module does support
doing this, but you should also be extremely careful on this.

First, in order to avoid dead locks, the callbacks passed to browser process
would be called asynchronously, so you should not expect the browser process to
get the return value of the passed callbacks.

Second, the callbacks passed to browser process would not get released
automatically after they were called, instead they would persistent until the
browser process garbage collected them.

For example, following code seems innocent at first glance, It installed a
callback for the `close` event on a remote object:

```javascript
var remote = require('remote');
remote.getCurrentWindow().on('close', function() {
  // blabla...
});
```

But the callback would be stored in the browser process persistently until you
explicitly uninstall it! So each time you reload your window, the callback would
be installed for once and previous callbacks were just leak. To make things
worse, since the context of previously installed callbacks have been released,
when `close` event was emitted exceptions would happen in browser process.

So generally, unless you are clear what you are doing, you should always avoid
passing callbacks to browser process.

## Remote buffer

An instance of node's `Buffer` is an object, so when you got a `Buffer` from
browser process, what you got was indeed a remote object (let's call it remote
buffer), and everything would just follow the rules of remote objects.

However you should remember that though a remote buffer behaves like the real
`Buffer`, it's not a `Buffer` at all. If you pass a remote buffer to node APIs
that accepting `Buffer`, you should assume the remote buffer would be treated
like a normal object, instead of a `Buffer`.

For example you can call `BrowserWindow.capturePage` in renderer process, which
returns a `Buffer` by calling passed callback:

```javascript
var remote = require('remote');
var fs = require('fs');
remote.getCurrentWindow().capturePage(function(buf) {
  fs.writeFile('/tmp/screenshot.png', buf, function(err) {
    console.log(err);
  });
});
```

But you may be surprised to find that the file written was corrupted. This is
because when you called `fs.writeFile`, you thought `buf` was a `Buffer`, but
indeed it was a remote buffer, and it would be converted to string before it was
written to file. Since `buf` contained binary data and could not be represented
by UTF-8 encoded string, the written file would be corrupted.

The workaround is to write the `buf` in browser process, where it is a real
`Buffer`:

```javascript
var remote = require('remote');
remote.getCurrentWindow().capturePage(function(buf) {
  remote.require('fs').writeFile('/tmp/screenshot.png', buf, function(err) {
    console.log(err);
  });
});
```

The same thing could happen for all native types, but usually it would just
throw a type error. The `Buffer` deserves your special attention because it
can be converted to string and APIs accepting `Buffer` usually accept string
too, and data corruption only happens when it contains binary data.

## remote.require(module)

* `module` String

Returns the object returned by `require(module)` in the browser process.

## remote.getCurrentWindow()

Returns the [BrowserWindow](browser-window.md) object which
represents current window.

## remote.getGlobal(name)

* `name` String

Returns the global variable of `name` (e.g. `global[name]`) in the browser
process.

## remote.process

Returns the `process` object in the browser process, this is the same with
`remote.getGlobal('process')` but gets cached.
