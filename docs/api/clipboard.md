# clipboard

An example of writing a string to clipboard:

```javascript
var clipboard = require('clipboard');
clipboard.writeText('Example String');
```

## clipboard.readText()

Returns the content in clipboard as plain text.

## clipboard.writeText(text)

* `text` String

Writes the `text` into clipboard as plain text.

## clipboard.clear()

Clears everything in clipboard.

## clipboard.has(type)

* `type` String

Returns whether clipboard has data in specified `type`.

**Note:** This API is experimental and could be removed in future.

## clipboard.read(type)

* `type` String

Reads the data in clipboard of the `type`.

**Note:** This API is experimental and could be removed in future.
