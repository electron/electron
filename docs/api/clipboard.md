# clipboard

> Perform copy and paste operations on the system clipboard.

Process: [Main](../tutorial/quick-start.md#main-process), [Renderer](../tutorial/quick-start.md#renderer-process)

The following example shows how to write a string to the clipboard:

```javascript
const {clipboard} = require('electron')
clipboard.writeText('Example String')
```

On X Window systems, there is also a selection clipboard. To manipulate it
you need to pass `selection` to each method:

```javascript
const {clipboard} = require('electron')
clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
```

## Methods

The `clipboard` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in future.

### `clipboard.readText([type])`

* `type` String (optional)

Returns `String` - The content in the clipboard as plain text.

### `clipboard.writeText(text[, type])`

* `text` String
* `type` String (optional)

Writes the `text` into the clipboard as plain text.

### `clipboard.readHTML([type])`

* `type` String (optional)

Returns `String` - The content in the clipboard as markup.

### `clipboard.writeHTML(markup[, type])`

* `markup` String
* `type` String (optional)

Writes `markup` to the clipboard.

### `clipboard.readImage([type])`

* `type` String (optional)

Returns `NativeImage` - The content in the clipboard as a [NativeImage](native-image.md).

### `clipboard.writeImage(image[, type])`

* `image` [NativeImage](native-image.md)
* `type` String (optional)

Writes `image` to the clipboard.

### `clipboard.readRTF([type])`

* `type` String (optional)

Returns `String` - The content in the clipboard as RTF.

### `clipboard.writeRTF(text[, type])`

* `text` String
* `type` String (optional)

Writes the `text` into the clipboard in RTF.

### `clipboard.readBookmark()` _macOS_ _Windows_

Returns `Object`:

* `title` String
* `url` String

Returns an Object containing `title` and `url` keys representing the bookmark in
the clipboard. The `title` and `url` values will be empty strings when the
bookmark is unavailable.

### `clipboard.writeBookmark(title, url[, type])` _macOS_ _Windows_

* `title` String
* `url` String
* `type` String (optional)

Writes the `title` and `url` into the clipboard as a bookmark.

**Note:** Most apps on Windows don't support pasting bookmarks into them so
you can use `clipboard.write` to write both a bookmark and fallback text to the
clipboard.

```js
clipboard.write({
  text: 'http://electron.atom.io',
  bookmark: 'Electron Homepage'
})
```

### `clipboard.readFindText()` _macOS_

Returns `String` - The text on the find pasteboard. This method uses synchronous
IPC when called from the renderer process. The cached value is reread from the
find pasteboard whenever the application is activated.

### `clipboard.writeFindText(text)` _macOS_

* `text` String

Writes the `text` into the find pasteboard as plain text. This method uses
synchronous IPC when called from the renderer process.

### `clipboard.clear([type])`

* `type` String (optional)

Clears the clipboard content.

### `clipboard.availableFormats([type])`

* `type` String (optional)

Returns `String[]` - An array of supported formats for the clipboard `type`.

### `clipboard.has(data[, type])` _Experimental_

* `data` String
* `type` String (optional)

Returns `Boolean` - Whether the clipboard supports the format of specified `data`.

```javascript
const {clipboard} = require('electron')
console.log(clipboard.has('<p>selection</p>'))
```

### `clipboard.read(data[, type])` _Experimental_

* `data` String
* `type` String (optional)

Returns `String` - Reads `data` from the clipboard.

### `clipboard.write(data[, type])`

* `data` Object
  * `text` String (optional)
  * `html` String (optional)
  * `image` [NativeImage](native-image.md) (optional)
  * `rtf` String (optional)
  * `bookmark` String (optional) - The title of the url at `text`.
* `type` String (optional)

```javascript
const {clipboard} = require('electron')
clipboard.write({text: 'test', html: '<b>test</b>'})
```
Writes `data` to the clipboard.
