# webFrame

> Customize the rendering of the current web page.

Process: [Renderer](../glossary.md#renderer-process)

`webFrame` export of the Electron module is an instance of the `WebFrame`
class representing the top frame of the current `BrowserWindow`. Sub-frames can
be retrieved by certain properties and methods (e.g. `webFrame.firstChild`).

An example of zooming current page to 200%.

```javascript
const { webFrame } = require('electron')

webFrame.setZoomFactor(2)
```

## Methods

The `WebFrame` class has the following instance methods:

### `webFrame.setZoomFactor(factor)`

* `factor` Number - Zoom factor.

Changes the zoom factor to the specified factor. Zoom factor is
zoom percent divided by 100, so 300% = 3.0.

### `webFrame.getZoomFactor()`

Returns `Number` - The current zoom factor.

### `webFrame.setZoomLevel(level)`

* `level` Number - Zoom level.

Changes the zoom level to the specified level. The original size is 0 and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively.

### `webFrame.getZoomLevel()`

Returns `Number` - The current zoom level.

### `webFrame.setVisualZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

Sets the maximum and minimum pinch-to-zoom level.

### `webFrame.setLayoutZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

Sets the maximum and minimum layout-based (i.e. non-visual) zoom level.

### `webFrame.setSpellCheckProvider(language, provider)`

* `language` String
* `provider` Object
  * `spellCheck` Function.
    * `words` String[]
    * `callback` Function
      * `misspeltWords` String[]

Sets a provider for spell checking in input fields and text areas.

The `provider` must be an object that has a `spellCheck` method that accepts
an array of individual words for spellchecking.
The `spellCheck` function runs asynchronously and calls the `callback` function
with an array of misspelt words when complete.

An example of using [node-spellchecker][spellchecker] as provider:

```javascript
const { webFrame } = require('electron')
const spellChecker = require('spellchecker')
webFrame.setSpellCheckProvider('en-US', {
  spellCheck (words, callback) {
    setTimeout(() => {
      const spellchecker = require('spellchecker')
      const misspelled = words.filter(x => spellchecker.isMisspelled(x))
      callback(misspelled)
    }, 0)
  }
})
```

### `webFrame.registerURLSchemeAsBypassingCSP(scheme)`

* `scheme` String

Resources will be loaded from this `scheme` regardless of the current page's
Content Security Policy.

### `webFrame.registerURLSchemeAsPrivileged(scheme[, options])`

* `scheme` String
* `options` Object (optional)
  * `secure` Boolean (optional) - Default true.
  * `bypassCSP` Boolean (optional) - Default true.
  * `allowServiceWorkers` Boolean (optional) - Default true.
  * `supportFetchAPI` Boolean (optional) - Default true.
  * `corsEnabled` Boolean (optional) - Default true.

Registers the `scheme` as secure, bypasses content security policy for resources,
allows registering ServiceWorker and supports fetch API.

Specify an option with the value of `false` to omit it from the registration.
An example of registering a privileged scheme, without bypassing Content Security Policy:

```javascript
const { webFrame } = require('electron')
webFrame.registerURLSchemeAsPrivileged('foo', { bypassCSP: false })
```

### `webFrame.insertText(text)`

* `text` String

Inserts `text` to the focused element.

### `webFrame.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean (optional) - Default is `false`.
* `callback` Function (optional) - Called after script has been executed.
  * `result` Any

Returns `Promise<any>` - A promise that resolves with the result of the executed code
or is rejected if the result of the code is a rejected promise.

Evaluates `code` in page.

In the browser window some HTML APIs like `requestFullScreen` can only be
invoked by a gesture from the user. Setting `userGesture` to `true` will remove
this limitation.

### `webFrame.executeJavaScriptInIsolatedWorld(worldId, scripts[, userGesture, callback])`

* `worldId` Integer - The ID of the world to run the javascript in, `0` is the default world, `999` is the world used by Electrons `contextIsolation` feature.  You can provide any integer here.
* `scripts` [WebSource[]](structures/web-source.md)
* `userGesture` Boolean (optional) - Default is `false`.
* `callback` Function (optional) - Called after script has been executed.
  * `result` Any

Work like `executeJavaScript` but evaluates `scripts` in an isolated context.

### `webFrame.setIsolatedWorldContentSecurityPolicy(worldId, csp)`

* `worldId` Integer - The ID of the world to run the javascript in, `0` is the default world, `999` is the world used by Electrons `contextIsolation` feature.  You can provide any integer here.
* `csp` String

Set the content security policy of the isolated world.

### `webFrame.setIsolatedWorldHumanReadableName(worldId, name)`

* `worldId` Integer - The ID of the world to run the javascript in, `0` is the default world, `999` is the world used by Electrons `contextIsolation` feature.  You can provide any integer here.
* `name` String

Set the name of the isolated world. Useful in devtools.

### `webFrame.setIsolatedWorldSecurityOrigin(worldId, securityOrigin)`

* `worldId` Integer - The ID of the world to run the javascript in, `0` is the default world, `999` is the world used by Electrons `contextIsolation` feature.  You can provide any integer here.
* `securityOrigin` String

Set the security origin of the isolated world.

### `webFrame.getResourceUsage()`

Returns `Object`:

* `images` [MemoryUsageDetails](structures/memory-usage-details.md)
* `scripts` [MemoryUsageDetails](structures/memory-usage-details.md)
* `cssStyleSheets` [MemoryUsageDetails](structures/memory-usage-details.md)
* `xslStyleSheets` [MemoryUsageDetails](structures/memory-usage-details.md)
* `fonts` [MemoryUsageDetails](structures/memory-usage-details.md)
* `other` [MemoryUsageDetails](structures/memory-usage-details.md)

Returns an object describing usage information of Blink's internal memory
caches.

```javascript
const { webFrame } = require('electron')
console.log(webFrame.getResourceUsage())
```

This will generate:

```javascript
{
  images: {
    count: 22,
    size: 2549,
    liveSize: 2542
  },
  cssStyleSheets: { /* same with "images" */ },
  xslStyleSheets: { /* same with "images" */ },
  fonts: { /* same with "images" */ },
  other: { /* same with "images" */ }
}
```

### `webFrame.clearCache()`

Attempts to free memory that is no longer being used (like images from a
previous navigation).

Note that blindly calling this method probably makes Electron slower since it
will have to refill these emptied caches, you should only call it if an event
in your app has occurred that makes you think your page is actually using less
memory (i.e. you have navigated from a super heavy page to a mostly empty one,
and intend to stay there).

[spellchecker]: https://github.com/atom/node-spellchecker

### `webFrame.getFrameForSelector(selector)`

* `selector` String - CSS selector for a frame element.

Returns `WebFrame` - The frame element in `webFrame's` document selected by
`selector`, `null` would be returned if `selector` does not select a frame or
if the frame is not in the current renderer process.

### `webFrame.findFrameByName(name)`

* `name` String

Returns `WebFrame` - A child of `webFrame` with the supplied `name`, `null`
would be returned if there's no such frame or if the frame is not in the current
renderer process.

### `webFrame.findFrameByRoutingId(routingId)`

* `routingId` Integer - An `Integer` representing the unique frame id in the
   current renderer process. Routing IDs can be retrieved from `WebFrame`
   instances (`webFrame.routingId`) and are also passed by frame
   specific `WebContents` navigation events (e.g. `did-frame-navigate`)

Returns `WebFrame` - that has the supplied `routingId`, `null` if not found.

## Properties

### `webFrame.top`

A `WebFrame` representing top frame in frame hierarchy to which `webFrame`
belongs, the property would be `null` if top frame is not in the current
renderer process.

### `webFrame.opener`

A `WebFrame` representing the frame which opened `webFrame`, the property would
be `null` if there's no opener or opener is not in the current renderer process.

### `webFrame.parent`

A `WebFrame` representing parent frame of `webFrame`, the property would be
`null` if `webFrame` is top or parent is not in the current renderer process.

### `webFrame.firstChild`

A `WebFrame` representing the first child frame of `webFrame`, the property
would be `null` if `webFrame` has no children or if first child is not in the
current renderer process.

### `webFrame.nextSibling`

A `WebFrame` representing next sibling frame, the property would be `null` if
`webFrame` is the last frame in its parent or if the next sibling is not in the
current renderer process.

### `webFrame.routingId`

An `Integer` representing the unique frame id in the current renderer process.
Distinct WebFrame instances that refer to the same underlying frame will have
the same `routingId`.
