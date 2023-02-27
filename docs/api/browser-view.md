# BrowserView

A `BrowserView` can be used to embed additional web content into a
[`BrowserWindow`](browser-window.md). It is like a child window, except that it is positioned
relative to its owning window. It is meant to be an alternative to the
`webview` tag.

## Class: BrowserView

> Create and control views.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

### Example

```javascript
// In the main process.
const { app, BrowserView, BrowserWindow } = require('electron')

app.whenReady().then(() => {
  const win = new BrowserWindow({ width: 800, height: 600 })

  const view = new BrowserView()
  win.setBrowserView(view)
  view.setBounds({ x: 0, y: 0, width: 300, height: 300 })
  view.webContents.loadURL('https://electronjs.org')
})
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
  * `width` boolean (optional) - If `true`, the view's width will grow and shrink together
    with the window. `false` by default.
  * `height` boolean (optional) - If `true`, the view's height will grow and shrink
    together with the window. `false` by default.
  * `horizontal` boolean (optional) - If `true`, the view's x position and width will grow
    and shrink proportionally with the window. `false` by default.
  * `vertical` boolean (optional) - If `true`, the view's y position and height will grow
    and shrink proportionally with the window. `false` by default.

#### `view.setBounds(bounds)` _Experimental_

* `bounds` [Rectangle](structures/rectangle.md)

Resizes and moves the view to the supplied bounds relative to the window.

#### `view.getBounds()` _Experimental_

Returns [`Rectangle`](structures/rectangle.md)

The `bounds` of this BrowserView instance as `Object`.

#### `view.setBackgroundColor(color)` _Experimental_

* `color` string - Color in Hex, RGB, ARGB, HSL, HSLA or named CSS color format. The alpha channel is
  optional for the hex type.

Examples of valid `color` values:

* Hex
  * #fff (RGB)
  * #ffff (ARGB)
  * #ffffff (RRGGBB)
  * #ffffffff (AARRGGBB)
* RGB
  * rgb\((\[\d]+),\s*(\[\d]+),\s*(\[\d]+)\)
    * e.g. rgb(255, 255, 255)
* RGBA
  * rgba\((\[\d]+),\s*(\[\d]+),\s*(\[\d]+),\s*(\[\d.]+)\)
    * e.g. rgba(255, 255, 255, 1.0)
* HSL
  * hsl\((-?\[\d.]+),\s*(\[\d.]+)%,\s*(\[\d.]+)%\)
    * e.g. hsl(200, 20%, 50%)
* HSLA
  * hsla\((-?\[\d.]+),\s*(\[\d.]+)%,\s*(\[\d.]+)%,\s*(\[\d.]+)\)
    * e.g. hsla(200, 20%, 50%, 0.5)
* Color name
  * Options are listed in [SkParseColor.cpp](https://source.chromium.org/chromium/chromium/src/+/main:third_party/skia/src/utils/SkParseColor.cpp;l=11-152;drc=eea4bf52cb0d55e2a39c828b017c80a5ee054148)
  * Similar to CSS Color Module Level 3 keywords, but case-sensitive.
    * e.g. `blueviolet` or `red`

**Note:** Hex format with alpha takes `AARRGGBB` or `ARGB`, _not_ `RRGGBBA` or `RGA`.

#### `view.showPopoverWindow(options)` _Experimental_ _macOS_

* `options` Object
  * `positioningWindow` BrowserWindow - The window relative to which the popover
    should be positioned.
  * `positioningRect` Rectangle (optional) - The rectangle within
    `positioningWindow` relative to which the popover should be positioned. If
    not set, defaults to the bounds of `positioningWindow`.
  * `size` Size - The size of the popover when opened.
  * `preferredEdge` string (optional) - The edge of `positioningWindow` the
    popover should prefer to be anchored to. Possible values are `max-x-edge`,
    `max-y-edge`, `min-x-edge`, `min-y-edge`. Default is `max-x-edge`.
  * `behavior` string (optional) - Specifies the behavior of the popover.
    Possible values are `application-defined`, `transient`. Default is
    `application-defined`.
  * `animate` boolean (optional) - Whether to animate to showing and closing of
    a popover. Default is `false`.

Shows the popover anchored to the specified position relative to a window .

**Note:** This method's parameters follow the corresponding macOS API's
parameters. See the [documentation](https://developer.apple.com/documentation/appkit/nspopover/1532113-showrelativetorect?language=objc) for further details.

#### `view.closePopoverWindow()` _Experimental_ _macOS_

Closes the popover.
