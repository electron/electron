# ClipboardItem

> A single clipboard entry that pairs one or more MIME-typed payloads.

Process: [Main](../glossary.md#main-process)

`ClipboardItem` is modeled after the W3C
[`ClipboardItem`](https://developer.mozilla.org/en-US/docs/Web/API/ClipboardItem)
class. Each `ClipboardItem` carries one or more MIME-typed payloads that
represent the same conceptual clipboard entry — for example, a single copy
operation that exposes both a plain-text and an HTML representation of a
selection.

## Class: ClipboardItem

> Construct a clipboard entry for [`clipboard.write()`](clipboard.md#clipboardwritedata)
> or inspect an entry returned from [`clipboard.read()`](clipboard.md#clipboardread).

Process: [Main](../glossary.md#main-process)

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new ClipboardItem(items)`

* `items` Record\<string, string | ClipboardBookmark | Buffer\> - An object whose keys are
  [MIME types](https://developer.mozilla.org/en-US/docs/Web/HTTP/MIME_types)
  and whose values are the payload for that type. Mirrors the W3C
  [`ClipboardItem(items)`](https://developer.mozilla.org/en-US/docs/Web/API/ClipboardItem/ClipboardItem#parameters)
  constructor's `items` parameter. The accepted value type depends on the
  MIME type. Text-typed MIME types (`text/plain`, `text/html`,
  `text/rtf`, and `electron application/findtext`) accept a `string`. The
  `electron application/bookmark` custom format accepts a [ClipboardBookmark](structures/clipboard-bookmark.md) object. All other MIME types accept a
  [`Buffer`](https://nodejs.org/api/buffer.html) of the raw payload
  bytes.

Creates a new `ClipboardItem` describing one clipboard entry with one or
more MIME-typed representations. The constructed item can be passed to
[`clipboard.write()`](clipboard.md#clipboardwritedata). Resolve any
`Blob` or `Promise` payload to a `Buffer` (or a `string` for text MIME
types) before calling the constructor.

```js
// Each `ClipboardItem` describes one clipboard entry with one or more
// MIME-typed representations. The bookmark custom format takes a structured
// `{ title, url }` object instead of a Buffer.
const { clipboard, ClipboardItem, nativeImage } = require('electron')

const png = nativeImage.createFromPath('/path/to/icon.png').toPNG()

clipboard.write([
  new ClipboardItem({
    'text/plain': 'hello',
    'text/html': '<b>hello</b>',
    'image/png': png,
    'electron application/bookmark': {
      title: 'Electron',
      url: 'https://electronjs.org'
    }
  })
])
```

### Instance Properties

#### `clipboardItem.types` _Readonly_

A `string[]` property — the MIME types of the data carried by this entry.
For a constructed `ClipboardItem` these are the keys passed to the
constructor; for an item returned by
[`clipboard.read()`](clipboard.md#clipboardread) these are the MIME types
the platform clipboard currently makes available.

### Instance Methods

#### `clipboardItem.getType(type)`

* `type` string - mime type to retrieve.

Returns `Promise<Buffer> | Promise<ClipboardBookmark>` - Resolves with the payload for the
given MIME type. Modeled after the W3C
[`ClipboardItem.getType`](https://developer.mozilla.org/en-US/docs/Web/API/ClipboardItem/getType)
method. The promise resolves to a `Buffer` for most MIME types; the one
exception is `getType('electron application/bookmark')`, which resolves
to a [ClipboardBookmark](structures/clipboard-bookmark.md) object instead.
Rejects when `type` is not present in
[`clipboardItem.types`](#clipboarditemtypes-readonly).

```js
const { clipboard } = require('electron')

async function dumpClipboard () {
  const items = await clipboard.read()
  for (const item of items) {
    for (const type of item.types) {
      const payload = await item.getType(type)
      console.log(type, payload)
    }
  }
}
```

#### `clipboardItem.getType(bookmark)`

* `bookmark` 'electron application/bookmark'

Returns `Promise<ClipboardBookmark>` - Resolves with a [ClipboardBookmark](structures/clipboard-bookmark.md)
when a bookmark is available in the clipboard.
Rejects when a bookmark is not available in the clipboard.

```js
const { clipboard } = require('electron')

async function dumpClipboard () {
  const bookmarkType = 'electron application/bookmark'
  const items = await clipboard.read()
  for (const item of items) {
    if (item.types.includes(bookmarkType)) {
      const bookmark = await item.getType(bookmarkType)
      console.log('Bookmark found: ', bookmark)
    } else {
      console.log('There is no bookmark present')
    }
  }
}
```
