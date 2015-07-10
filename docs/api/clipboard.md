# clipboard

The `clipboard` provides methods to do copy/paste operations. An example of
writing a string to clipboard:

```javascript
var clipboard = require('clipboard');
clipboard.writeText('Example String');
```

On X Window systems, there is also a selection clipboard, to manipulate in it
you need to pass `selection` to each method:

```javascript
var clipboard = require('clipboard');
clipboard.writeText('Example String', 'selection');
console.log(clipboard.readText('selection'));
```

## clipboard.readText([type])

* `type` String

Returns the content in clipboard as plain text.

## clipboard.writeText(text[, type])

* `text` String
* `type` String

Writes the `text` into clipboard as plain text.

## clipboard.readHtml([type])

* `type` String

Returns the content in clipboard as markup.

## clipboard.writeHtml(markup[, type])

* `markup` String
* `type` String

Writes the `markup` into clipboard.

## clipboard.readImage([type])

* `type` String

Returns the content in clipboard as [NativeImage](native-image.md).

## clipboard.writeImage(image[, type])

* `image` [NativeImage](native-image.md)
* `type` String

Writes the `image` into clipboard.

## clipboard.clear([type])

* `type` String

Clears everything in clipboard.

## clipboard.availableFormats([type])

Returns an array of supported `format` for the clipboard `type`.

## clipboard.has(data[, type])

* `data` String
* `type` String

Returns whether clipboard supports the format of specified `data`.

```javascript
var clipboard = require('clipboard');
console.log(clipboard.has('<p>selection</p>'));
```

**Note:** This API is experimental and could be removed in future.

## clipboard.read(data[, type])

* `data` String
* `type` String

Reads the `data` in clipboard.

**Note:** This API is experimental and could be removed in future.

## clipboard.write(data[, type])

* `data` Object
  * `text` String
  * `html` String
  * `image` [NativeImage](native-image.md)
* `type` String

```javascript
var clipboard = require('clipboard');
clipboard.write({text: 'test', html: "<b>test</b>"});
```
Writes the `data` iinto clipboard.
