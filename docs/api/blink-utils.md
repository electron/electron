# blinkUtils

> A utility layer to interact with blink objects

Process: [Renderer](../glossary.md#renderer-process)

## Methods

The `blinkUtils` module has the following methods:

### `blinkUtils.getPathForFile(file)`

* `file` File - A web [File](https://developer.mozilla.org/en-US/docs/Web/API/File) object.

Returns `string` - The file system path that this `File` object points to. In the case where the object passed in is not a `File` object an exception is thrown. In the case where the File object passed in was constructed in JS and is not backed by a file on disk an empty string is returned.

This method superceded the previous augmentation to the `File` object with the `path` property.  An example is included below.

```js
// Before
const oldPath = document.querySelector('input').files[0].path

// After
const { blinkUtils } = require('electron')
const newPath = blinkUtils.getPathForFile(document.querySelector('input').files[0])
```
