---
title: Dock Menu
description: Configure your app's Dock presence on macOS.
slug: macos-dock
hide_title: true
---

# Dock Menu

On macOS, the [Dock](https://support.apple.com/en-ca/guide/mac-help/mh35859/mac) is an interface
element that displays open and frequently-used apps. While opened or pinned, each app has its own
icon in the Dock.

> [!NOTE]
> On macOS, the Dock is the entry point for certain cross-platform features like
> [Recent Documents](./recent-documents.md) and [Application Progress](./progress-bar.md).
> Read those guides for more details.

## Dock API

All functionality for the Dock is exposed via the [Dock](../api/dock.md) class exposed via
[`app.dock`](../api/app.md#appdock-macos-readonly) property. There is a single `Dock` instance per
Electron application, and this property only exists on macOS.

One of the main uses for your app's Dock icon is to expose additional app menus. The Dock menu is
triggered by right-clicking or <kbd>Ctrl</kbd>-clicking the app icon. By default, the app's Dock menu
will come with system-provided window management utilities, including the ability to show all windows,
hide the app, and switch betweeen different open windows.

To set an app-defined custom Dock menu, pass any [Menu](../api/menu.md) instance into the
[`dock.setMenu`](../api/dock.md#docksetmenumenu-macos) API.

> [!TIP]
> For best practices to make your Dock menu feel more native, see Apple's
> [Human Interface Guidelines](https://developer.apple.com/design/human-interface-guidelines/dock-menus)
> page on Dock menus.

## Attaching a context menu

```js title='Setting a Dock menu'
const { app, BrowserWindow, Menu } = require('electron/main')

// dock.setMenu only works after the 'ready' event is fired
app.whenReady().then(() => {
  const dockMenu = Menu.buildFromTemplate([
    {
      label: 'New Window',
      click: () => { const win = new BrowserWindow() }
    }
    // add more menu options to the array
  ])

  // Dock is undefined on platforms outside of macOS
  // highlight-next-line
  app.dock?.setMenu(dockMenu)
})
```

> [!NOTE]
> Unlike with regular [context menus](./context-menu.md), Dock context menus don't need to be
> manually instrumented using the `menu.popup` API. Instead, the Dock object handles click events
> for you.

> [!TIP]
> To learn more about crafting menus in Electron, see the [Menus](./menus.md#building-menus) guide.

## Runnable Fiddle demo

Below is a runnable example of how you can use the Dock menu to create and close windows in your
Electron app.

```fiddle docs/fiddles/menus/dock-menu
```
