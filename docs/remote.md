## Synopsis

It's common that the developers want to use modules in browsers from the renderer, like closing current window, opening file dialogs, etc. Instead of writing IPC code for every operation you want to do, atom-shell provides the `remote` module to let you do RPC call just like using normal javascript objects.

An example of creating a window in renderer:

```javascript
var remote = require('remote');
var BrowserWindow = remote.require('browser-window');
var win = new BrowserWindow({ width: 800, height: 600 });
win.loadUrl('https://github.com');
```

## Lifetime of remote objects

Every object returned by `remote` module represents an object in browser (e.g. a remote object), so when you call methods of an object, or call a returned function, or even create a object with the returned constructor, you are indeed making a synchronous RPC call. And when the renderer releases the last reference to the remote object, the browser would release the corresponding reference too.

This also means that, if the renderer keeps a reference to an object in browser, the object would never be released. So be careful to never leak the remote objects.

## Passing callbacks

Many APIs in browser accepts callbacks, so the `remote` module also supports passing callbacks when calling remote functions, and the callbacks passed would become remote functions in the browser.

But in order to avoid possible dead locks, the callbacks passed to browser would be called asynchronously in browser, so you should never expect the browser to get the return value of the passed callback.

Another thing is the lifetime of the remote callbacks in browser, it might be very tempting to do things like following:

```javascript
var remote = require('remote');
remote.getCurrentWindow().on('close', function() {
  // blabla...
});
```

Yes it will work correctly, but when you reload the window, the callback you setup on the object in browser will not be erased, resources are leaked and there is no magic in javascript to release a referenced object.

So if you really need to keep a reference of callbacks in browser, you should write the callback in browser and send messages to renderer. And also make use of DOM's events like `unload` and `beforeunload`, they will work perfectly.

## remote.require(module)

* `module` String

Return a module in browser.

## remote.getCurrentWindow()

Return the `BrowserWindow` object that represents current window.

`Note:` it doesn't return the `window` object which represents the global scope, instead it returns an instance of the `BrowserWindow` class which is created with `browser-window` module in browser.

## remote.getGlobal(name)

* `name` String

Return the `global[name]` value in browser.

## remote.process

Getter to return the `process` object in browser, this is the same with `remote.getGlobal('process')` but gets cached.