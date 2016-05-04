# Synopsis

> How to use Node.js and Electron APIs.

All of [Node.js's built-in modules](http://nodejs.org/api/) are available in
Electron and third-party node modules also fully supported as well (including
the [native modules](../tutorial/using-native-node-modules.md)).

Electron also provides some extra built-in modules for developing native
desktop applications. Some modules are only available in the main process, some
are only available in the renderer process (web page), and some can be used in
both processes.

The basic rule is: if a module is [GUI][gui] or low-level system related, then
it should be only available in the main process. You need to be familiar with
the concept of [main process vs. renderer process](../tutorial/quick-start.md#the-main-process)
scripts to be able to use those modules.

The main process script is just like a normal Node.js script:

```javascript
const electron = require('electron');
const app = electron.app;
const BrowserWindow = electron.BrowserWindow;

var window = null;

app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadURL('https://github.com');
});
```

The renderer process is no different than a normal web page, except for the
extra ability to use node modules:

```html
<!DOCTYPE html>
<html>
<body>
<script>
  const remote = require('electron').remote;
  console.log(remote.app.getVersion());
</script>
</body>
</html>
```

To run your app, read [Run your app](../tutorial/quick-start.md#run-your-app).

## Destructuring assignment

If you are using CoffeeScript or Babel, you can also use
[destructuring assignment][destructuring-assignment] to make it easier to use
built-in modules:

```javascript
const {app, BrowserWindow} = require('electron')
```

However if you are using plain JavaScript, you have to wait until Chrome fully
supports ES6.

[gui]: https://en.wikipedia.org/wiki/Graphical_user_interface
[destructuring-assignment]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment
[issue-387]: https://github.com/electron/electron/issues/387
