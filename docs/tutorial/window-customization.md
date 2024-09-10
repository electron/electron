# Window Customization

The `BrowserWindow` module is the foundation of your Electron application, and it exposes many APIs that let you customize the look and behavior of your app’s windows. This tutorial covers how to implement various use-cases for window customization on macOS, Windows, and Linux.

## Custom Titlebars

### Basic Tutorial

Application windows have a default [chrome][] applied by the OS. Not to be confused with the Google Chrome browser, window *chrome* refers to the parts of the window (e.g. titlebar, toolbars, controls) that are not a part of the main web content. While the default titlebar is useful for handing common window behaviors, many modern applications opt to remove it in favor of building a fully customized title bar.

You can follow along with this tutorial by opening Fiddle with the following starter code.

```fiddle docs/fiddles/features/window-customization/starter-code

```

#### Removing the default titlebar

Let’s start by configuring a window with a hidden titlebar and native window controls. To do this, we will first remove the default titlebar by setting the `titleBarStyle` option in the `BrowserWindow` constructor.

```fiddle docs/fiddles/features/window-customization/remove-titlebar

```

#### Adding native window controls *Windows* *Linux*

On macOS, you should see that setting `titleBarStyle: 'hidden'` removed the titlebar while keeping the window’s traffic light controls available in the upper left hand corner. However, on Windows and Linux, you’ll need to add window controls back into your `BrowserWindow` by setting the `titleBarOverlay` option in the `BrowserWindow` constructor.

```fiddle docs/fiddles/features/window-customization/native-window-controls

```

Setting `titleBarOverlay` to `true` is the simplest way to expose window controls back into your `BrowserWindow`. If you’re interested in customizing the window controls further, check out the sections Customizing traffic lights and Customizing window controls that cover this in more detail.

#### Create a custom titlebar

Now, let’s implement a simple custom color titlebar within the webContents of our BrowserWindow. There’s nothing fancy here, just HTML and CSS!

```fiddle docs/fiddles/features/window-customization/custom-titlebar

```

#### Setting a custom drag region

Currently our application window can’t be moved. Since we’ve removed the default titlebar, the application needs to tell Electron which regions are draggable. We’ll do this by adding the CSS style `app-region: drag` to the custom titlebar. Now we’ll be able drag the custom titlebar to reposition our app window!

```fiddle docs/fiddles/features/window-customization/custom-drag-region

```

For more information around how to manage drag regions defined by your electron application, see the Custom draggable regions section below.

### Customizing traffic lights *macOS*

#### Customize the look of your traffic lights *macOS*

The `customButtonsOnHover` title bar style will hide the traffic lights until you hover
over them. This is useful if you want to create custom traffic lights in your HTML but still
use the native UI to control the window.

```js
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({ titleBarStyle: 'customButtonsOnHover' })
```

#### Customize the traffic light position *macOS*

To modify the position of the traffic light window controls, there are two configuration
options available.

Applying `hiddenInset` title bar style will shift the vertical inset of the traffic lights
by a fixed amount.

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({ titleBarStyle: 'hiddenInset' })
```

If you need more granular control over the positioning of the traffic lights, you can pass
a set of coordinates to the `trafficLightPosition` option in the `BrowserWindow`
constructor.

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({
  titleBarStyle: 'hidden',
  trafficLightPosition: { x: 10, y: 10 }
})
```

#### Show and hide the traffic lights programmatically *macOS*

You can also show and hide the traffic lights programmatically from the main process.
The `win.setWindowButtonVisibility` forces traffic lights to be show or hidden depending
on the value of its boolean parameter.

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()
// hides the traffic lights
win.setWindowButtonVisibility(false)
```

> Note: Given the number of APIs available, there are many ways of achieving this. For instance,
> combining `frame: false` with `win.setWindowButtonVisibility(true)` will yield the same
> layout outcome as setting `titleBarStyle: 'hidden'`.

### Customizing window controls

The [Window Controls Overlay API][] is a web standard that gives web apps the ability to
customize their title bar region when installed on desktop. Electron exposes this API
through the `BrowserWindow` constructor option `titleBarOverlay`.

This option only works whenever a custom `titlebarStyle` is applied.
When `titleBarOverlay` is enabled, the window controls become exposed in their default
position, and DOM elements cannot use the area underneath this region.

The `titleBarOverlay` option accepts two different value formats.

Specifying `true` on either platform will result in an overlay region with default
system colors:

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({
  titleBarStyle: 'hidden',
  titleBarOverlay: true
})
```

`titleBarOverlay` can also be set to an object. The height of the overlay can be specified with the `height` property. On Windows and Linux, the color of the overlay and can be specified using the `color` property. On Windows and Linux, the color of the overlay and its symbols can be specified using the `color` and `symbolColor` properties respectively. The `rgba()`, `hsla()`, and `#RRGGBBAA` color formats are supported to apply transparency.

If a color option is not specified, the color will default to its system color for the window control buttons. Similarly, if the height option is not specified it will default to the default height:

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({
  titleBarStyle: 'hidden',
  titleBarOverlay: {
    color: '#2f3241',
    symbolColor: '#74b1be',
    height: 60
  }
})
```

> Note: Once your title bar overlay is enabled from the main process, you can access the overlay's
> color and dimension values from a renderer using a set of readonly
> [JavaScript APIs][overlay-javascript-apis] and [CSS Environment Variables][overlay-css-env-vars].

## Custom draggable regions

By default, the frameless window is non-draggable. Apps need to specify
`app-region: drag` in CSS to tell Electron which regions are draggable
(like the OS's standard titlebar), and apps can also use
`app-region: no-drag` to exclude the non-draggable area from the
 draggable region. Note that only rectangular shapes are currently supported.

To make the whole window draggable, you can add `app-region: drag` as
`body`'s style:

```css title='styles.css'
body {
  app-region: drag;
}
```

And note that if you have made the whole window draggable, you must also mark
buttons as non-draggable, otherwise it would be impossible for users to click on
them:

```css title='styles.css'
button {
  app-region: no-drag;
}
```

If you're only setting a custom titlebar as draggable, you also need to make all
buttons in titlebar non-draggable.

### Tip: disable text selection

When creating a draggable region, the dragging behavior may conflict with text selection.
For example, when you drag the titlebar, you may accidentally select its text contents.
To prevent this, you need to disable text selection within a draggable area like this:

```css
.titlebar {
  user-select: none;
  app-region: drag;
}
```

### Tip: disable context menus

On some platforms, the draggable area will be treated as a non-client frame, so
when you right click on it, a system menu will pop up. To make the context menu
behave correctly on all platforms, you should never use a custom context menu on
draggable areas.

## Frameless windows

A frameless window removes all [chrome](https://developer.mozilla.org/en-US/docs/Glossary/Chrome) applied by the OS, including window controls.

To create a frameless window, set `frame` to `false` in the `BrowserWindow` constructor.

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({ frame: false })
```

## Transparent windows

By setting the `transparent` option to `true`, you can make a fully transparent window.

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow({ transparent: true })
```

### Limitations

* You cannot click through the transparent area. See
  [#1335](https://github.com/electron/electron/issues/1335) for details.
* Transparent windows are not resizable. Setting `resizable` to `true` may make
  a transparent window stop working on some platforms.
* The CSS [`blur()`][] filter only applies to the window's web contents, so there is no way to apply
  blur effect to the content below the window (i.e. other applications open on
  the user's system).
* The window will not be transparent when DevTools is opened.
* On *Windows*:
  * Transparent windows will not work when DWM is disabled.
  * Transparent windows can not be maximized using the Windows system menu or by double
  clicking the title bar. The reasoning behind this can be seen on
  PR [#28207](https://github.com/electron/electron/pull/28207).
* On *macOS*:
  * The native window shadow will not be shown on a transparent window.

## Click-through windows

To create a click-through window, i.e. making the window ignore all mouse
events, you can call the [win.setIgnoreMouseEvents(ignore)][ignore-mouse-events]
API:

```js title='main.js'
const { BrowserWindow } = require('electron')
const win = new BrowserWindow()
win.setIgnoreMouseEvents(true)
```

### Forward mouse events *macOS* *Windows*

Ignoring mouse messages makes the web contents oblivious to mouse movement,
meaning that mouse movement events will not be emitted. On Windows and macOS, an
optional parameter can be used to forward mouse move messages to the web page,
allowing events such as `mouseleave` to be emitted:

```js title='main.js'
const { BrowserWindow, ipcMain } = require('electron')
const path = require('node:path')

const win = new BrowserWindow({
  webPreferences: {
    preload: path.join(__dirname, 'preload.js')
  }
})

ipcMain.on('set-ignore-mouse-events', (event, ignore, options) => {
  const win = BrowserWindow.fromWebContents(event.sender)
  win.setIgnoreMouseEvents(ignore, options)
})
```

```js title='preload.js'
window.addEventListener('DOMContentLoaded', () => {
  const el = document.getElementById('clickThroughElement')
  el.addEventListener('mouseenter', () => {
    ipcRenderer.send('set-ignore-mouse-events', true, { forward: true })
  })
  el.addEventListener('mouseleave', () => {
    ipcRenderer.send('set-ignore-mouse-events', false)
  })
})
```

This makes the web page click-through when over the `#clickThroughElement` element,
and returns to normal outside it.

[`blur()`]: https://developer.mozilla.org/en-US/docs/Web/CSS/filter-function/blur()
[chrome]: https://developer.mozilla.org/en-US/docs/Glossary/Chrome
[ignore-mouse-events]: ../api/browser-window.md#winsetignoremouseeventsignore-options
[overlay-css-env-vars]: https://github.com/WICG/window-controls-overlay/blob/main/explainer.md#css-environment-variables
[overlay-javascript-apis]: https://github.com/WICG/window-controls-overlay/blob/main/explainer.md#javascript-apis
[Window Controls Overlay API]: https://github.com/WICG/window-controls-overlay/blob/main/explainer.md
