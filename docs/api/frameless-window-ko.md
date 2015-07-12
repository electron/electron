# Frameless window

frameless window는 테두리가 없는 윈도우 창을 말합니다.

## Frameless window 만들기

Frameless window를 만드려면 [BrowserWindow](browser-window-ko.md) 객체의 `options`에서 `frame` 옵션을 `false`로 지정하기만 하면됩니다:

```javascript
var BrowserWindow = require('browser-window');
var win = new BrowserWindow({ width: 800, height: 600, frame: false });
```

## 투명한 윈도우

By setting the `transparent` option to `true`, you can also make the frameless
window transparent:

```javascript
var win = new BrowserWindow({ transparent: true, frame: false });
```

### API의 한계

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

## 드래그 가능 위치 지정

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

## 텍스트 선택

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

## 컨텍스트 메뉴

On some platforms, the draggable area would be treated as non-client frame, so
when you right click on it a system menu would be popuped. To make context menu
behave correctly on all platforms, you should never custom context menu on
draggable areas.
