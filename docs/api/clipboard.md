# clipboard

> Perform copy and paste operations on the system clipboard.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

On Linux, there is also a `selection` clipboard. To manipulate it
you need to pass `selection` to each method:

```javascript
const { clipboard } = require('electron')

clipboard.writeText('Example String', 'selection')
console.log(clipboard.readText('selection'))
```

## Methods

The `clipboard` module has the following methods:

**Note:** Experimental APIs are marked as such and could be removed in future.

### `clipboard.readText([type])`

* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Returns `String` - The content in the clipboard as plain text.

```js
const { clipboard } = require('electron')

clipboard.writeText('hello i am a bit of text!')

const text = clipboard.readText()
console.log(text)
// hello i am a bit of text!'
```

### `clipboard.writeText(text[, type])`

* `text` String
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes the `text` into the clipboard as plain text.

```js
const { clipboard } = require('electron')

const text = 'hello i am a bit of text!'
clipboard.writeText(text)
```

### `clipboard.readHTML([type])`

* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Returns `String` - The content in the clipboard as markup.

```js
const { clipboard } = require('electron')

clipboard.writeHTML('<b>Hi</b>')
const html = clipboard.readHTML()

console.log(html)
// <meta charset='utf-8'><b>Hi</b>
```

### `clipboard.writeHTML(markup[, type])`

* `markup` String
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes `markup` to the clipboard.

```js
const { clipboard } = require('electron')

clipboard.writeHTML('<b>Hi</b>')
```

### `clipboard.readImage([type])`

* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Returns [`NativeImage`](native-image.md) - The image content in the clipboard.

### `clipboard.writeImage(image[, type])`

* `image` [NativeImage](native-image.md)
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes `image` to the clipboard.

### `clipboard.readRTF([type])`

* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Returns `String` - The content in the clipboard as RTF.

```js
const { clipboard } = require('electron')

clipboard.writeRTF('{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}')

const rtf = clipboard.readRTF()
console.log(rtf)
// {\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}
```

### `clipboard.writeRTF(text[, type])`

* `text` String
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes the `text` into the clipboard in RTF.

```js
const { clipboard } = require('electron')

const rtf = '{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\nThis is some {\\b bold} text.\\par\n}'
clipboard.writeRTF(rtf)
```

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
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes the `title` and `url` into the clipboard as a bookmark.

**Note:** Most apps on Windows don't support pasting bookmarks into them so
you can use `clipboard.write` to write both a bookmark and fallback text to the
clipboard.

```js
const { clipboard } = require('electron')

clipboard.writeBookmark({
  text: 'https://electronjs.org',
  bookmark: 'Electron Homepage'
})
```

### `clipboard.readFindText()` _macOS_

Returns `String` - The text on the find pasteboard, which is the pasteboard that holds information about the current state of the active application’s find panel.

This method uses synchronous IPC when called from the renderer process.
The cached value is reread from the find pasteboard whenever the application is activated.

### `clipboard.writeFindText(text)` _macOS_

* `text` String

Writes the `text` into the find pasteboard (the pasteboard that holds information about the current state of the active application’s find panel) as plain text. This method uses synchronous IPC when called from the renderer process.

### `clipboard.clear([type])`

* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Clears the clipboard content.

### `clipboard.availableFormats([type])`

* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Returns `String[]` - An array of supported formats for the clipboard `type`.

```js
const { clipboard } = require('electron')

const formats = clipboard.availableFormats()
console.log(formats)
// [ 'text/plain', 'text/html' ]
```

### `clipboard.has(format[, type])` _Experimental_

* `format` String
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Returns `Boolean` - Whether the clipboard supports the specified `format`.

```js
const { clipboard } = require('electron')

const hasFormat = clipboard.has('public/utf8-plain-text')
console.log(hasFormat)
// 'true' or 'false'
```

### `clipboard.read(format)` _Experimental_

* `format` String

Returns `String` - Reads `format` type from the clipboard.

`format` should contain valid ASCII characters and have `/` separator.
`a/c`, `a/bc` are valid formats while `/abc`, `abc/`, `a/`, `/a`, `a`
are not valid.

### `clipboard.readBuffer(format)` _Experimental_

* `format` String

Returns `Buffer` - Reads `format` type from the clipboard.

```js
const { clipboard } = require('electron')

const buffer = Buffer.from('this is binary', 'utf8')
clipboard.writeBuffer('public/utf8-plain-text', buffer)

const ret = clipboard.readBuffer('public/utf8-plain-text')

console.log(buffer.equals(out))
// true
```

### `clipboard.writeBuffer(format, buffer[, type])` _Experimental_

* `format` String
* `buffer` Buffer
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes the `buffer` into the clipboard as `format`.

```js
const { clipboard } = require('electron')

const buffer = Buffer.from('writeBuffer', 'utf8')
clipboard.writeBuffer('public/utf8-plain-text', buffer)
```

### `clipboard.write(data[, type])`

* `data` Object
  * `text` String (optional)
  * `html` String (optional)
  * `image` [NativeImage](native-image.md) (optional)
  * `rtf` String (optional)
  * `bookmark` String (optional) - The title of the URL at `text`.
* `type` String (optional) - Can be `selection` or `clipboard`; default is 'clipboard'. `selection` is only available on Linux.

Writes `data` to the clipboard.

```js
const { clipboard } = require('electron')

clipboard.write({
  text: 'test',
  html: '<b>Hi</b>',
  rtf: '{\\rtf1\\utf8 text}',
  bookmark: 'a title'
})

console.log(clipboard.readText())
// 'test'

console.log(clipboard.readHTML())
// <meta charset='utf-8'><b>Hi</b>

console.log(clipboard.readRTF())
// '{\\rtf1\\utf8 text}'

console.log(clipboard.readBookmark())
// { title: 'a title', url: 'test' }
```
