# Frameless window

A frameless window is a window that has no chrome.

## Create a frameless window

To create a frameless window, you only need to specify `frame` to `false` in
[BrowserWindow](browser-window.md)'s `options`:


```javascript
var BrowserWindow = require('browser-window');
var win = new BrowserWindow({ width: 800, height: 600, frame: false });
```

## Transparent window

By setting the `transparent` option to `true`, you can also make the frameless
window transparent:

```javascript
var win = new BrowserWindow({ transparent: true, frame: false });
```

### Limitations

* You can not click through the transparent area, we are going to introduce an
  API to set window shape to solve this, but currently blocked at an
  [upstream bug](https://code.google.com/p/chromium/issues/detail?id=387234).
* Transparent window is not resizable, setting `resizable` to `true` may make
  transparent window stop working on some platforms.
* The `blur` filter only applies to the web page, so there is no way to apply
  blur effect to the content below the window.
* On Windows transparent window will not work when DWM is disabled.
* On Linux users have to put `--enable-transparent-visuals --disable-gpu` in
  command line to disable GPU and allow ARGB to make transparent window, this is
  caused by an upstream bug that [alpha channel doesn't work on some NVidia
  drivers](https://code.google.com/p/chromium/issues/detail?id=369209) on Linux.
* On Mac the native window shadow will not show for transparent window.

## Draggable region

By default, the frameless window is non-draggable. Apps need to specify
`-webkit-app-region: drag` in CSS to tell Electron which regions are draggable
(like the OS's standard titlebar), and apps can also use
`-webkit-app-region: no-drag` to exclude the non-draggable area from the
 draggable region. Note that only rectangular shape is currently supported.

To make the whole window draggable, you can add `-webkit-app-region: drag` as
`body`'s style:

```html
<body style="-webkit-app-region: drag">
</body>
```

And note that if you have made the whole window draggable, you must also mark
buttons as non-draggable, otherwise it would be impossible for users to click on
them:

```css
button {
  -webkit-app-region: no-drag;
}
```

If you're only using a custom titlebar, you also need to make buttons in
titlebar non-draggable.

## Text selection

One thing on frameless window is that the dragging behaviour may conflict with
selecting text, for example, when you drag the titlebar, you may accidentally
select the text on titlebar. To prevent this, you need to disable text
selection on dragging area like this:

```css
.titlebar {
  -webkit-user-select: none;
  -webkit-app-region: drag;
}
```

## Context menu

On some platforms, the draggable area would be treated as non-client frame, so
when you right click on it a system menu would be popuped. To make context menu
behave correctly on all platforms, you should never custom context menu on
draggable areas.
