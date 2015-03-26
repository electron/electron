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

## clipboard.has(format[, type])

* `format` String
* `type` String

Returns whether clipboard has data in specified `format`.

**Note:** This API is experimental and could be removed in future.

## clipboard.read(format[, type])

* `format` String
* `type` String

Reads the data in clipboard of the `format`.

**Note:** This API is experimental and could be removed in future.
