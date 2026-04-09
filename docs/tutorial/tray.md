---
title: Tray Menu
description: Create a Tray icon with its own menu in the system's notification area.
slug: tray
hide_title: true
---

# Tray Menu

This guide will take you through the process of creating an icon with its own context menu to the
system tray.

* On macOS, the icon will be located on the top right corner of your screen in the
  [menu bar extras](https://developer.apple.com/design/human-interface-guidelines/the-menu-bar#Menu-bar-extras)
  area.
* On Windows, the icon will be located in the [notification area](https://learn.microsoft.com/en-us/windows/win32/shell/notification-area)
  at the end of the taskbar.
* On Linux, the location of the Tray will differ based on your desktop environment.

## Creating a Tray icon

The tray icon for your Electron app needs to be created programmatically with an instance of
the [Tray](../api/tray.md#new-trayimage-guid) class. The class constructor requires a single
instance of a [NativeImage](../api/native-image.md#class-nativeimage) or a path to a compatible icon
file.

> [!NOTE]
> File formats vary per operating system. For more details, see the
> [Platform Considerations](../api/tray.md#platform-considerations) section of the Tray API documentation.

## Minimizing to tray

In order to keep the app and the system tray icon alive even when all windows are closed, you need to
have a listener for the [`window-all-closed`](../api/app.md#event-window-all-closed) event on the
`app` module. The base Electron templates generally listen for this event but quit the app on
Windows and Linux to emulate standard OS behavior.

```js title='Setting up minimize to tray'
app.on('window-all-closed', () => {
  // having this listener active will prevent the app from quitting.
})
```

## Attaching a context menu

You can attach a context menu to the Tray object by passing in a [Menu](../api/menu.md) instance
into the [`tray.setContextMenu`](../api/tray.md#traysetcontextmenumenu) function.

> [!NOTE]
> Unlike with regular [context menus](./context-menu.md), Tray context menus don't need to be
> manually instrumented using the `menu.popup` API. Instead, the Tray object handles click events
> for you (although various click-related events exist on the API for advanced use cases).

```js title='Creating a Tray menu that can quit the app'
const { nativeImage } = require('electron/common')
const { app, Tray, Menu } = require('electron/main')

// save a reference to the Tray object globally to avoid garbage collection
let tray

// 16x16 red circle data URL
const icon = nativeImage.createFromDataURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAACTSURBVHgBpZKBCYAgEEV/TeAIjuIIbdQIuUGt0CS1gW1iZ2jIVaTnhw+Cvs8/OYDJA4Y8kR3ZR2/kmazxJbpUEfQ/Dm/UG7wVwHkjlQdMFfDdJMFaACebnjJGyDWgcnZu1/lrCrl6NCoEHJBrDwEr5NrT6ko/UV8xdLAC2N49mlc5CylpYh8wCwqrvbBGLoKGvz8Bfq0QPWEUo/EAAAAASUVORK5CYII=')

// The Tray can only be instantiated after the 'ready' event is fired
app.whenReady().then(() => {
  tray = new Tray(icon)
  const contextMenu = Menu.buildFromTemplate([
    { role: 'quit' }
  ])
  tray.setContextMenu(contextMenu)
})
```

> [!TIP]
> To learn more about crafting menus in Electron, see the [Menus](./menus.md#building-menus) guide.

> [!WARNING]
> The `enabled` and `visibility` properties are not available for top-level menu items in the tray on macOS.

## Runnable Fiddle demo

Below is a runnable example of attaching various menu items to the Tray's context menu that help
control app state and interact with the Tray API itself.

```fiddle docs/fiddles/menus/tray-menu
```
