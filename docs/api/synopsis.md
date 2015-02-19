# Synopsis

All [node.js's built-in modules](http://nodejs.org/api/) are available in
atom-shell, and third-party node modules are fully supported too (including the
[native modules](../tutorial/using-native-node-modules.md)).

Atom-shell also provides some extra built-in modules for developing native
desktop applications. Some modules are only available on the browser side, some
are only available on the client (renderer) side, and some can be used on both sides.
The basic rule is: if a module is GUI or low-level system related, then it should
be only available on the browser side. You need to be familiar with the concept of
[browser-side vs. client-side](../tutorial/quick-start.md#the-browser-side) scripts
to be able to use those modules.

The browser-side script is just like a normal `node.js` script:

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

var window = null;

app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadUrl('https://github.com');
});

```

The web page is no different than a normal web page, except for the extra
ability to use node modules:

```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      var remote = require('remote');
      console.log(remote.require('app').getVersion());
    </script>
  </body>
</html>
```

To run your app, read [Run your app](../tutorial/quick-start.md#run-your-app).
