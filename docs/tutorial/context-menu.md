---
title: Context Menu
description: Configure cross-platform native OS menus with the Menu API.
slug: context-menu
hide_title: true
---

# Context Menu

Context menus are pop-up menus that appear when right-clicking (or pressing a shortcut
such as <kbd>Shift</kbd> + <kbd>F10</kbd> on Windows) somewhere in an app's interface.

No context menu will appear by default in Electron. However, context menus can be created by using
the [`menu.popup`](../api/menu.md#menupopupoptions) function on an instance of the
[Menu](../api/menu.md) class. You will need to listen for specific context menu events and set up
the trigger for `menu.popup` manually.

There are two ways of listening for context menu events in Electron: either via the main process
through [webContents](../api/web-contents.md) or in the renderer process via the
[`contextmenu`](https://developer.mozilla.org/en-US/docs/Web/API/Element/contextmenu_event) web event.

## Using the `context-menu` event (main)

Whenever a right-click is detected within the bounds of a specific `WebContents` instance, a
[`context-menu`](../api/web-contents.md#event-context-menu) event is triggered. The `params` object
passed to the listener provides an extensive list of attributes to distinguish which type of element
is receiving the event.

For example, if you want to provide a context menu for links, check for the `linkURL` parameter.
If you want to check for editable elements such as `<textarea/>`, check for the `isEditable` parameter.

```fiddle docs/fiddles/menus/context-menu/web-contents
```

## Using the `contextmenu` event (renderer)

Alternatively, you can also listen to the [`contextmenu`](https://developer.mozilla.org/en-US/docs/Web/API/Element/contextmenu_event)
event available on DOM elements in the renderer process and call the `menu.popup` function via IPC.

> [!TIP]
> To learn more about IPC basics in Electron, see the [Inter-Process Communication](./ipc.md) guide.

```fiddle docs/fiddles/menus/context-menu/dom|focus=preload.js
```

## Additional macOS menu items (e.g. Writing Tools)

On macOS, the [Writing Tools](https://support.apple.com/en-ca/guide/mac-help/mchldcd6c260/15.0/mac/15.0),
[AutoFill](https://support.apple.com/en-mz/guide/safari/ibrwf71ba236/mac), and
[Services](https://support.apple.com/en-ca/guide/mac-help/mchlp1012/mac) menu items
are disabled by default for context menus in Electron. To enable these features, pass the
[WebFrameMain](../api/web-frame-main.md) associated to the target `webContents` to the `frame`
parameter in `menu.popup`.

```js title='Associating a frame to the context menu'
const { BrowserWindow, Menu } = require('electron/main')

const menu = Menu.buildFromTemplate([{ role: 'editMenu' }])
const win = new BrowserWindow()
win.webContents.on('context-menu', (_event, params) => {
  // Whether the context is editable.
  if (params.isEditable) {
    menu.popup({
    // highlight-next-line
      frame: params.frame
    })
  }
})
```
