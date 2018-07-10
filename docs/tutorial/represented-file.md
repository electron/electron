# Represented File for macOS BrowserWindows

On macOS a window can set its represented file, so the file's icon can show in
the title bar and when users Command-Click or Control-Click on the title a path
popup will show.

You can also set the edited state of a window so that the file icon can indicate
whether the document in this window has been modified.

__Represented file popup menu:__

![Represented File][represented-image]

To set the represented file of window, you can use the
[BrowserWindow.setRepresentedFilename][setrepresentedfilename] and
[BrowserWindow.setDocumentEdited][setdocumentedited] APIs:

```javascript
const { BrowserWindow } = require('electron')

const win = new BrowserWindow()
win.setRepresentedFilename('/etc/passwd')
win.setDocumentEdited(true)
```

[represented-image]: https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png
[setrepresentedfilename]: ../api/browser-window.md#winsetrepresentedfilenamefilename-macos
[setdocumentedited]: ../api/browser-window.md#winsetdocumenteditededited-macos
