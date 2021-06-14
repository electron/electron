# BrowserView

A `BrowserView` can be used to embed additional web content into a
[`BrowserWindow`](browser-window.md). It is like a child window, except that it is positioned
relative to its owning window. It is meant to be an alternative to the
`webview` tag.

## Class: BrowserView

> Create and control views.

Process: [Main](../glossary.md#main-process)

### Example

```javascript
// In the main process.
const { BrowserView, BrowserWindow } = require('electron')

const win = new BrowserWindow({ width: 800, height: 600 })

const view = new BrowserView()
win.setBrowserView(view)
view.setBounds({ x: 0, y: 0, width: 300, height: 300 })
view.webContents.loadURL('https://electronjs.org')
```

### `new BrowserView([options])` _Experimental_

* `options` Object (optional)
  * `webPreferences` Object (optional) - See [BrowserWindow](browser-window.md).

### Instance Properties

Objects created with `new BrowserView` have the following properties:

#### `view.webContents` _Experimental_

A [`WebContents`](web-contents.md) object owned by this view.

### Instance Methods

Objects created with `new BrowserView` have the following instance methods:

#### `view.setAutoResize(options)` _Experimental_

* `options` Object
  * `width` Boolean (optional) - If `true`, the view's width will grow and shrink together
    with the window. `false` by default.
  * `height` Boolean (optional) - If `true`, the view's height will grow and shrink
    together with the window. `false` by default.
  * `horizontal` Boolean (optional) - If `true`, the view's x position and width will grow
    and shrink proportionally with the window. `false` by default.
  * `vertical` Boolean (optional) - If `true`, the view's y position and height will grow
    and shrink proportionally with the window. `false` by default.

#### `view.setBounds(bounds)` _Experimental_

* `bounds` [Rectangle](structures/rectangle.md)

Resizes and moves the view to the supplied bounds relative to the window.

#### `view.getBounds()` _Experimental_

Returns [`Rectangle`](structures/rectangle.md)

The `bounds` of this BrowserView instance as `Object`.

#### `view.setBackgroundColor(color)` _Experimental_

* `color` String - Color in `#aarrggbb` or `#argb` form. The alpha channel is
  optional.
