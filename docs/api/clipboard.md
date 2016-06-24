# clipboard

> Perform copy and paste operations on the system clipboard.

The following example shows how to write a string to the clipboard:

```javascript
const {clipboard} = require('electron');
clipboard.writeText('Example String');
```

On X Window systems, there is also a selection clipboard. To manipulate it
you need to pass `selection` to each method:

```javascript
clipboard.writeText('Example String', 'selection');
console.log(clipboard.readText('selection'));
```

## Methods

The `clipboard` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in future.

### `clipboard.readText([type])`

* `type` String (optional)

Returns the content in the clipboard as plain text.

### `clipboard.writeText(text[, type])`

* `text` String
* `type` String (optional)

Writes the `text` into the clipboard as plain text.

### `clipboard.readHTML([type])`

* `type` String (optional)

Returns the content in the clipboard as markup.

### `clipboard.writeHTML(markup[, type])`

* `markup` String
* `type` String (optional)

Writes `markup` to the clipboard.

### `clipboard.readImage([type])`

* `type` String (optional)

Returns the content in the clipboard as a [NativeImage](native-image.md).

### `clipboard.writeImage(image[, type])`

* `image` [NativeImage](native-image.md)
* `type` String (optional)

Writes `image` to the clipboard.

### `clipboard.readRTF([type])`

* `type` String (optional)

Returns the content in the clipboard as RTF.

### `clipboard.writeRTF(text[, type])`

* `text` String
* `type` String (optional)

Writes the `text` into the clipboard in RTF.

### `clipboard.readBookmark()`

Returns an Object containing `title` and `url` keys representing the bookmark in
the clipboard. The `title` and `url` values will be empty strings when the
bookmark is unavailable.

### `clipboard.writeBookmark(title, url[, type])`

* `title` String
* `url` String
* `type` String (optional)

Writes the `title` and `url` into the clipboard as a bookmark.

### `clipboard.clear([type])`

* `type` String (optional)

Clears the clipboard content.

### `clipboard.availableFormats([type])`

* `type` String (optional)

Returns an array of supported formats for the clipboard `type`.

### `clipboard.has(data[, type])` _Experimental_

* `data` String
* `type` String (optional)

Returns whether the clipboard supports the format of specified `data`.

```javascript
console.log(clipboard.has('<p>selection</p>'));
```

### `clipboard.read(data[, type])` _Experimental_

* `data` String
* `type` String (optional)

Reads `data` from the clipboard.

### `clipboard.write(data[, type])`

* `data` Object
  * `text` String
  * `html` String
  * `image` [NativeImage](native-image.md)
  * `rtf` String
  * `bookmark` String - The title of the url at `text`.
* `type` String (optional)

```javascript
clipboard.write({text: 'test', html: "<b>test</b>"});
```
Writes `data` to the clipboard.
