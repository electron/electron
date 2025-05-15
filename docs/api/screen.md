# screen

> Retrieve information about screen size, displays, cursor position, etc.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

`screen` is an [EventEmitter][event-emitter].

> [!NOTE]
> In the renderer / DevTools, `window.screen` is a reserved DOM
> property, so writing `let { screen } = require('electron')` will not work.

An example of creating a window that fills the whole screen:

```fiddle docs/fiddles/screen/fit-screen
// Retrieve information about screen size, displays, cursor position, etc.
//
// For more info, see:
// https://www.electronjs.org/docs/latest/api/screen

const { app, BrowserWindow, screen } = require('electron/main')

let mainWindow = null

app.whenReady().then(() => {
  // Create a window that fills the screen's available work area.
  const primaryDisplay = screen.getPrimaryDisplay()
  const { width, height } = primaryDisplay.workAreaSize

  mainWindow = new BrowserWindow({ width, height })
  mainWindow.loadURL('https://electronjs.org')
})
```

Another example of creating a window in the external display:

```js
const { app, BrowserWindow, screen } = require('electron')

let win

app.whenReady().then(() => {
  const displays = screen.getAllDisplays()
  const externalDisplay = displays.find((display) => {
    return display.bounds.x !== 0 || display.bounds.y !== 0
  })

  if (externalDisplay) {
    win = new BrowserWindow({
      x: externalDisplay.bounds.x + 50,
      y: externalDisplay.bounds.y + 50
    })
    win.loadURL('https://github.com')
  }
})
```

> [!NOTE]
> Screen coordinates used by this module are [`Point`](structures/point.md) structures.
> There are two kinds of coordinates available to the process:
>
> * **Physical screen points** are raw hardware pixels on a display.
> * **Device-independent pixel (DIP) points** are virtualized screen points scaled based on the DPI
>   (dots per inch) of the display.

## Events

The `screen` module emits the following events:

### Event: 'display-added'

Returns:

* `event` Event
* `newDisplay` [Display](structures/display.md)

Emitted when `newDisplay` has been added.

### Event: 'display-removed'

Returns:

* `event` Event
* `oldDisplay` [Display](structures/display.md)

Emitted when `oldDisplay` has been removed.

### Event: 'display-metrics-changed'

Returns:

* `event` Event
* `display` [Display](structures/display.md)
* `changedMetrics` string[]

Emitted when one or more metrics change in a `display`. The `changedMetrics` is
an array of strings that describe the changes. Possible changes are `bounds`,
`workArea`, `scaleFactor` and `rotation`.

## Methods

The `screen` module has the following methods:

### `screen.getCursorScreenPoint()`

Returns [`Point`](structures/point.md)

The current absolute position of the mouse pointer.

> [!NOTE]
> The return value is a DIP point, not a screen physical point.

### `screen.getPrimaryDisplay()`

Returns [`Display`](structures/display.md) - The primary display.

### `screen.getAllDisplays()`

Returns [`Display[]`](structures/display.md) - An array of displays that are currently available.

### `screen.getDisplayNearestPoint(point)`

* `point` [Point](structures/point.md)

Returns [`Display`](structures/display.md) - The display nearest the specified point.

### `screen.getDisplayMatching(rect)`

* `rect` [Rectangle](structures/rectangle.md)

Returns [`Display`](structures/display.md) - The display that most closely
intersects the provided bounds.

### `screen.screenToDipPoint(point)` _Windows_

* `point` [Point](structures/point.md)

Returns [`Point`](structures/point.md)

Converts a screen physical point to a screen DIP point.
The DPI scale is performed relative to the display containing the physical point.

### `screen.dipToScreenPoint(point)` _Windows_

* `point` [Point](structures/point.md)

Returns [`Point`](structures/point.md)

Converts a screen DIP point to a screen physical point.
The DPI scale is performed relative to the display containing the DIP point.

### `screen.screenToDipRect(window, rect)` _Windows_

* `window` [BrowserWindow](browser-window.md) | null
* `rect` [Rectangle](structures/rectangle.md)

Returns [`Rectangle`](structures/rectangle.md)

Converts a screen physical rect to a screen DIP rect.
The DPI scale is performed relative to the display nearest to `window`.
If `window` is null, scaling will be performed to the display nearest to `rect`.

### `screen.dipToScreenRect(window, rect)` _Windows_

* `window` [BrowserWindow](browser-window.md) | null
* `rect` [Rectangle](structures/rectangle.md)

Returns [`Rectangle`](structures/rectangle.md)

Converts a screen DIP rect to a screen physical rect.
The DPI scale is performed relative to the display nearest to `window`.
If `window` is null, scaling will be performed to the display nearest to `rect`.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
