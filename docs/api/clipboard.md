# clipboard

The `clipboard` provides methods to perform copy and paste operations. The following example
shows how to write a string to the clipboard:

```javascript
var clipboard = require('clipboard');
clipboard.writeText('Example String');
```

On X Window systems, there is also a selection clipboard. To manipulate it
you need to pass `selection` to each method:

```javascript
var clipboard = require('clipboard');
clipboard.writeText('Example String', 'selection');
console.log(clipboard.readText('selection'));
```

## clipboard.readText([type])

* `type` String

Returns the content in the clipboard as plain text.

## clipboard.writeText(text[, type])

* `text` String
* `type` String

Writes the `text` into the clipboard as plain text.

## clipboard.readHtml([type])

* `type` String

Returns the content in the clipboard as markup.

## clipboard.writeHtml(markup[, type])

* `markup` String
* `type` String

Writes `markup` into the clipboard.

## clipboard.readImage([type])

* `type` String

Returns the content in the clipboard as a [NativeImage](native-image.md).

## clipboard.writeImage(image[, type])

* `image` [NativeImage](native-image.md)
* `type` String

Writes `image` into the clipboard.

## clipboard.clear([type])

* `type` String

Clears the clipboard.

## clipboard.availableFormats([type])

Returns an array of supported `format` for the clipboard `type`.

## clipboard.has(data[, type])

* `data` String
* `type` String

Returns whether the clipboard supports the format of specified `data`.

```javascript
var clipboard = require('clipboard');
console.log(clipboard.has('<p>selection</p>'));
```

**Note:** This API is experimental and could be removed in future.

## clipboard.read(data[, type])

* `data` String
* `type` String

Reads `data` from the clipboard.

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
Writes `data` into clipboard.
