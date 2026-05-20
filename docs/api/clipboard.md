# clipboard

<!--
```YAML history
deprecated:
  - pr-url: https://github.com/electron/electron/pull/48877
    description: "Using the `clipboard` API directly in the renderer process is deprecated."
    breaking-changes-header: deprecated-clipboard-api-access-from-renderer-processes
```
-->

> Perform copy and paste operations on the system clipboard.

Process: [Main](../glossary.md#main-process)

The `clipboard` module is modeled after the
[W3C Clipboard API](https://w3c.github.io/clipboard-apis/#clipboard-interface):
`clipboard.read()` returns a `Promise` that resolves with a list of
[`ClipboardItem`](clipboard-item.md) objects, and `clipboard.write()`
accepts an array of `ClipboardItem` instances that map
[MIME types](https://developer.mozilla.org/en-US/docs/Web/HTTP/MIME_types)
to [Buffer](https://nodejs.org/api/buffer.html) payloads.

In addition to the standard MIME types (`text/plain`, `text/html`,
`text/rtf`, `image/png`, `image/jpeg`, …), Electron exposes a small
set of custom MIME types so the clipboard can carry desktop-specific
payloads. These follow the W3C
[custom format proposal](https://github.com/w3c/editing/blob/gh-pages/docs/clipboard-pickling/explainer.md#custom-formats),
using an `electron` prefix instead of `web` to avoid collisions. The
custom MIME types Electron exposes are:

* `electron application/bookmark` — a URL bookmark. Unlike every other
  MIME type, its payload is a [Bookmark](structures/bookmark.md) object
  on both the write and read sides rather than a `Buffer`, so
  `getType('electron application/bookmark')` resolves to
  `{ title: string, url: string }`.
* `electron application/findtext` (_macOS_) — the contents of the
  active app's find pasteboard.
* `electron application/osclipboard;format="<name>"` — a raw payload
  for a platform-specific clipboard format. The `<name>` is the
  platform format (e.g. `HTML Format` on Windows or
  `public.utf8-plain-text` on macOS). `clipboard.read()` also surfaces
  any platform clipboard format that has no standard MIME mapping under
  this MIME type, so a raw OS format round-trips through the same string
  on write and read.

Beyond the well-known MIME types, both `clipboard.read()` and
`clipboard.write()` accept arbitrary MIME types including MIME types starting with
the `web` prefix (followed by a space, e.g. `web application/x.my-format`)
that follow the W3C [web custom format proposal](https://github.com/w3c/editing/blob/gh-pages/docs/clipboard-pickling/explainer.md#custom-formats).

```js
const { clipboard, ClipboardItem } = require('electron')

clipboard.write([
  new ClipboardItem({
    'web application/x.my-app-clip': Buffer.from('arbitrary payload')
  })
])
```

On Linux there is also a `selection` clipboard. It is exposed via the
[`clipboard.selection`](#clipboardselection-linux-readonly) sub-namespace,
which mirrors the top-level `clipboard` interface.
The `selection` clipboard operates against the
selection clipboard instead of the system clipboard.

It exposes the same surface as the top-level `clipboard` module, but
each method targets the selection clipboard rather than the system
clipboard. The two clipboards are independent: writing via
`clipboard.selection` does not affect the data returned by
`clipboard.read()` (and vice versa).

> [!NOTE]
> The `selection` clipboard does not support the W3C [web custom format](https://github.com/w3c/editing/blob/gh-pages/docs/clipboard-pickling/explainer.md#custom-formats).

```js
const { clipboard } = require('electron')

async function run () {
  await clipboard.selection.writeText('Example string')
  console.log(await clipboard.selection.readText())
}

run()
```

## Methods

The `clipboard` module has the following methods.

### `clipboard.readText()`

Returns `Promise<string>` - A promise that resolves with the content of the
clipboard as plain text. Modeled after the W3C
[`navigator.clipboard.readText`](https://developer.mozilla.org/en-US/docs/Web/API/Clipboard/readText)
API.

```js
const { clipboard } = require('electron')

async function readText () {
  await clipboard.writeText('hello i am a bit of text!')
  const text = await clipboard.readText()
  console.log(text)
  // 'hello i am a bit of text!'
}

readText()
```

### `clipboard.writeText(text)`

* `text` string

Returns `Promise<void>` - A promise that resolves once the text has been
written to the clipboard. Modeled after the W3C
[`navigator.clipboard.writeText`](https://developer.mozilla.org/en-US/docs/Web/API/Clipboard/writeText)
API.

```js
const { clipboard } = require('electron')

clipboard.writeText('hello i am a bit of text!')
```

### `clipboard.read()`

Returns `Promise<ClipboardItem[]>` - A promise that resolves with an array of
[ClipboardItem](clipboard-item.md) objects containing the clipboard's
contents. Each `ClipboardItem` has a `types` array of MIME types and a
`getType(type)` method that returns a `Promise<Buffer>` for the payload of
that type. The one exception is
`getType('electron application/bookmark')`, which resolves to a plain
[Bookmark](structures/bookmark.md) object instead of a `Buffer`. The items
returned by `clipboard.read()` are lightweight — the platform clipboard is
queried lazily, only when `getType` is called.

```js
const { clipboard } = require('electron')

async function dumpClipboard () {
  const items = await clipboard.read()
  for (const item of items) {
    for (const type of item.types) {
      const buffer = await item.getType(type)
      console.log(type, buffer)
    }
  }
}

dumpClipboard()
```

### `clipboard.write(data)`

* `data` [ClipboardItem](clipboard-item.md)[] - An array of
  [`ClipboardItem`](clipboard-item.md) instances constructed via
  `new ClipboardItem({ [mime]: payload })`. The accepted payload type
  depends on the MIME type. Text-typed MIME types (`text/plain`,
  `text/html`, `text/rtf`, and `electron application/findtext`) accept a
  `string`. The `electron application/bookmark` MIME type accepts a plain
  `{ title: string, url: string }` object. All other MIME types — the
  W3C `web` custom format prefix,
  `electron application/osclipboard;format="..."`, `image/*`, and any
  other arbitrary MIME — accept a
  [`Buffer`](https://nodejs.org/api/buffer.html) (committed verbatim). If
  you have a `Blob` or a `Promise`, resolve it to a `Buffer` (or a
  `string` for text MIME types) before calling `write()`.

Returns `Promise<void>` - Resolves once the data has been written to the
clipboard. All entries supplied in a single `write()` call are committed
to the system clipboard atomically.

```js
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

### `clipboard.has(mimetype)`

* `mimetype` string - MIME type to check

Returns `Promise<boolean>` - A promise that resolves with `true` if the
clipboard contains data of the specified `mimetype`, otherwise `false`.
To check for a raw format, eg `public/utf8-plain-text`, use the `electron application/osclipboard`
MIME type (`electron application/osclipboard;format="public/utf8-plain-text"`).

```js
const { clipboard } = require('electron')

async function check () {
  const hasFormat = await clipboard.has('text/html')
  console.log(hasFormat)
  // 'true' or 'false'
  const rawFormat = 'electron application/osclipboard;format="public/utf8-plain-text"'
  const hasRawFormat = await clipboard.has('rawFormat')
}

check()
```

### `clipboard.clear()`

Clears the clipboard content.

### `clipboard.selection.readText()` _Linux_

Returns `Promise<string>` - A promise that resolves with the content of
the selection clipboard as plain text. Equivalent to
[`clipboard.readText()`](#clipboardreadtext) but reads from the primary
selection.

### `clipboard.selection.writeText(text)` _Linux_

* `text` string

Returns `Promise<void>` - A promise that resolves once the text has been
written to the selection clipboard. Equivalent to
[`clipboard.writeText(text)`](#clipboardwritetexttext) but writes to the
selection clipboard.

### `clipboard.selection.read()` _Linux_

Returns `Promise<ClipboardItem[]>` - A promise that resolves with an
array of [ClipboardItem](clipboard-item.md) objects containing the
selection clipboard's contents. Equivalent to
[`clipboard.read()`](#clipboardread) but reads from the primary
selection.

### `clipboard.selection.write(data)` _Linux_

* `data` [ClipboardItem](clipboard-item.md)[] - An array of
  [`ClipboardItem`](clipboard-item.md) instances. See
  [`clipboard.write(data)`](#clipboardwritedata) for the accepted
  payload types per MIME.

Returns `Promise<void>` - Resolves once the data has been written to the
selection clipboard. All entries supplied in a single `write()` call are
committed atomically. Equivalent to
[`clipboard.write(data)`](#clipboardwritedata) but writes to the primary
selection.

### `clipboard.selection.has(mimetype)` _Linux_

* `mimetype` string - MIME type to check

Returns `Promise<boolean>` - A promise that resolves with `true` if the
selection clipboard contains data of the specified `mimetype`, otherwise
`false`. Equivalent to [`clipboard.has(mimetype)`](#clipboardhasmimetype) but
queries the selection clipboard.

### `clipboard.selection.clear()` _Linux_

Clears the selection clipboard. Equivalent to
[`clipboard.clear()`](#clipboardclear) but targets the primary
selection.

## Properties

### `clipboard.selection` _Linux_ _Readonly_

A `Clipboard` property — a `Clipboard` object on Linux that
operates against the selection clipboard instead of the system clipboard,
and `undefined` on all other platforms. It exposes the same `read`,
`write`, `readText`, `writeText`, `has`, and `clear` methods as the
top-level `clipboard` module.
