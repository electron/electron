# webFrame

> Customize the rendering of the current web page.

An example of zooming current page to 200%.

```javascript
const {webFrame} = require('electron');

webFrame.setZoomFactor(2);
```

## Methods

The `webFrame` module has the following methods:

### `webFrame.setZoomFactor(factor)`

* `factor` Number - Zoom factor.

Changes the zoom factor to the specified factor. Zoom factor is
zoom percent divided by 100, so 300% = 3.0.

### `webFrame.getZoomFactor()`

Returns the current zoom factor.

### `webFrame.setZoomLevel(level)`

* `level` Number - Zoom level

Changes the zoom level to the specified level. The original size is 0 and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively.

### `webFrame.getZoomLevel()`

Returns the current zoom level.

### `webFrame.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

Sets the maximum and minimum zoom level.

### `webFrame.setSpellCheckProvider(language, autoCorrectWord, provider)`

* `language` String
* `autoCorrectWord` Boolean
* `provider` Object

Sets a provider for spell checking in input fields and text areas.

The `provider` must be an object that has a `spellCheck` method that returns
whether the word passed is correctly spelled.

An example of using [node-spellchecker][spellchecker] as provider:

```javascript
webFrame.setSpellCheckProvider('en-US', true, {
  spellCheck(text) {
    return !(require('spellchecker').isMisspelled(text));
  }
});
```

### `webFrame.registerURLSchemeAsSecure(scheme)`

* `scheme` String

Registers the `scheme` as secure scheme.

Secure schemes do not trigger mixed content warnings. For example, `https` and
`data` are secure schemes because they cannot be corrupted by active network
attackers.

### `webFrame.registerURLSchemeAsBypassingCSP(scheme)`

* `scheme` String

Resources will be loaded from this `scheme` regardless of the current page's
Content Security Policy.

### `webFrame.registerURLSchemeAsPrivileged(scheme)`

* `scheme` String

Registers the `scheme` as secure, bypasses content security policy for resources,
allows registering ServiceWorker and supports fetch API.

### `webFrame.insertText(text)`

* `text` String

Inserts `text` to the focused element.

### `webFrame.executeJavaScript(code[, userGesture])`

* `code` String
* `userGesture` Boolean (optional) - Default is `false`.

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.


### `webFrame.getResourceUsage()`

Returns more detailed memory usage information in kilobytes of Blink's internal
memory caches. Returns an Object of the following shape:

```js
{
  "images": {
    "count": 22,
    "size": 2549,         // 2549kb
    "liveSize": 2542,     // 2542kb, etc...
    "decodedSize": 478,
    "purgedSize": 0,
    "purgeableSize": 0
  },
  "cssStyleSheets": {
    "count": 7,
    /* ... */
  },
  "xslStyleSheets": {
    "count": 0,
    /* ... */
  },
  "fonts": {
    "count": 18,
    /* ... */
  },
  "other": {
    "count": 0,
    /* ... */
  }
}
```

### `webFrame.purgeCaches()`

Attempts to free memory that is no longer being used (i.e. images from a
previous navigation, etc etc).

Note that blindly calling this method probably makes Electron slower since it
will have to refill these emptied caches, you should only call it if an event
in your app has occured that makes you think your page is actually using less
memory (i.e. you have navigated from a super heavy page to a mostly empty one,
and intend to stay there)

[spellchecker]: https://github.com/atom/node-spellchecker
