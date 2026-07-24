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

* `items` Record\<string, string | ClipboardBookmark | Blob | Promise\<Blob | string\>\> - An object whose keys are
  [MIME types](https://developer.mozilla.org/en-US/docs/Web/HTTP/MIME_types)
  and whose values are the payload for that type. Mirrors the W3C
  [`ClipboardItem(items)`](https://developer.mozilla.org/en-US/docs/Web/API/ClipboardItem/ClipboardItem#parameters)
  constructor's `items` parameter. Every MIME type accepts either a `string`
  or a [`Blob`](https://developer.mozilla.org/en-US/docs/Web/API/Blob): a
  `string` is UTF-8 encoded into the payload bytes (as the W3C spec
  requires), and a `Blob` supplies the raw payload bytes. The
  `electron application/bookmark` custom format is the one exception — it
  accepts a [ClipboardBookmark](structures/clipboard-bookmark.md) object.
  Any non-bookmark value may also be a `Promise` that resolves
  to a `Blob` or `string`; it is awaited when `clipboard.write()` is called.

Creates a new `ClipboardItem` describing one clipboard entry with one or
more MIME-typed representations. The constructed item can be passed to
[`clipboard.write()`](clipboard.md#clipboardwritedata). Each `Blob` or
`Promise` payload is resolved asynchronously when `clipboard.write()` is
called.

> [!WARNING]
> Do not construct a `ClipboardItem` directly from an untrusted object (for
> example, a payload received from a renderer over IPC). The MIME keys are a
> capability surface: `text/uri-list` places real file references on the OS
> clipboard (letting a file be pasted into another application), and the
> `electron application/osclipboard;format=...` and `web`-prefixed (e.g.
> `web application/x.my-format`) formats write raw platform data. Validate
> and allowlist the MIME types — and the shape of each payload — before
> building a `ClipboardItem` from data you did not author.

```js
// Each `ClipboardItem` describes one clipboard entry with one or more
// MIME-typed representations. The bookmark custom format takes a structured
// `{ title, url }` object instead of a Blob.
const { clipboard, ClipboardItem, nativeImage } = require('electron')

const png = nativeImage.createFromPath('/path/to/icon.png').toPNG()

clipboard.write([
  new ClipboardItem({
    'text/plain': 'hello',
    'text/html': '<b>hello</b>',
    'image/png': new Blob([png], { type: 'image/png' }),
    'electron application/bookmark': {
      title: 'Electron',
      url: 'https://electronjs.org'
    }
  })
])
```

### Files: the `text/uri-list` MIME type

The `text/uri-list` MIME type is mapped to the operating system's native
"copied files" clipboard format (`CF_HDROP` on Windows,
`NSFilenamesPboardType` on macOS, and `text/uri-list` on Linux) rather than
being stored as a generic text payload. This lets clipboard entries written
by Electron be pasted as files into native applications such as Finder,
Explorer, or a file manager, and lets Electron read files that were copied
from those applications.

The payload is an [RFC 2483](https://www.rfc-editor.org/rfc/rfc2483) URI
list: one `file://` URI per line, separated by `CRLF`. Use
[`url.pathToFileURL`](https://nodejs.org/api/url.html#urlpathtofileurlpath-options)
to convert an absolute path into a `file://` URI. Although RFC 2483 permits
any URI scheme, this format is files-only — non-`file://` URIs are ignored.

When reading, `getType('text/uri-list')` resolves to a `Blob` whose text is
the `file://` URI list. Because this is a privileged main-process API, the
resolved URIs contain the real absolute paths of the files — unlike the
renderer's `navigator.clipboard`, which sanitizes file paths for privacy.

```js
const { clipboard, ClipboardItem } = require('electron')
const { pathToFileURL } = require('node:url')

// Write two files to the clipboard so they can be pasted into the OS file
// manager.
clipboard.write([
  new ClipboardItem({
    'text/uri-list': [
      pathToFileURL('/path/to/first.txt').href,
      pathToFileURL('/path/to/second.txt').href
    ].join('\r\n')
  })
])

// Read the files currently on the clipboard.
async function readFiles () {
  const [item] = await clipboard.read()
  if (item.types.includes('text/uri-list')) {
    const blob = await item.getType('text/uri-list')
    if (blob instanceof Blob) {
      const uriList = await blob.text()
      return uriList.split(/\r?\n/).filter(Boolean)
    }
  }
  return []
}
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

Returns `Promise<import('buffer').Blob> | Promise<ClipboardBookmark>` - Resolves with the payload for the
given MIME type. Modeled after the W3C
[`ClipboardItem.getType`](https://developer.mozilla.org/en-US/docs/Web/API/ClipboardItem/getType)
method. The promise resolves to a `Blob` for most MIME types; the one
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
