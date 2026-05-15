# Bookmark Object

* `title` string - The title of the bookmark.
* `url` string - The URL of the bookmark.

A `Bookmark` is the payload used by the `electron application/bookmark`
clipboard MIME type. It is passed to
[`clipboard.write()`](../clipboard.md#clipboardwritedata) as a
[`ClipboardItem`](../clipboard-item.md) `data` value, and is what
`getType('electron application/bookmark')` resolves to when reading via
[`clipboard.read()`](../clipboard.md#clipboardread).
