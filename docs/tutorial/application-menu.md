---
title: Application Menu
description: Customize the main application menu for your Electron app
slug: application-menu
hide_title: true
---

# Application Menu

Each Electron app has a single top-level application menu.

* On macOS, this menu is shown in the system [menu bar](https://support.apple.com/en-ca/guide/mac-help/mchlp1446/mac).
* On Windows and Linux, this menu is shown at the top of each [BaseWindow](../api/base-window.md).

## Building application menus

The application menu can be set by passing a [Menu](../api/menu.md) instance into the
[`Menu.setApplicationMenu`](../api/menu.md#menusetapplicationmenumenu) static function.

When building an application menu in Electron, each top-level array menu item **must be a submenu**.

Electron will set a default menu for your app if this API is never called. Below is an example of
that default menu being created manually using shorthand [`MenuItem` roles](./menus.md#roles).

```js title='Manually creating the default menu' @ts-expect-error=[93]
const { shell } = require('electron/common')
const { app, Menu } = require('electron/main')

const isMac = process.platform === 'darwin'
const template = [
  // { role: 'appMenu' }
  ...(isMac
    ? [
        {
          label: app.name,
          submenu: [
            { role: 'about' },
            { type: 'separator' },
            { role: 'services' },
            { type: 'separator' },
            { role: 'hide' },
            { role: 'hideOthers' },
            { role: 'unhide' },
            { type: 'separator' },
            { role: 'quit' }
          ]
        }
      ]
    : []),
  // { role: 'fileMenu' }
  {
    label: 'File',
    submenu: [isMac ? { role: 'close' } : { role: 'quit' }]
  },
  // { role: 'editMenu' }
  {
    label: 'Edit',
    submenu: [
      { role: 'undo' },
      { role: 'redo' },
      { type: 'separator' },
      { role: 'cut' },
      { role: 'copy' },
      { role: 'paste' },
      ...(isMac
        ? [
            { role: 'pasteAndMatchStyle' },
            { role: 'delete' },
            { role: 'selectAll' },
            { type: 'separator' },
            {
              label: 'Speech',
              submenu: [{ role: 'startSpeaking' }, { role: 'stopSpeaking' }]
            }
          ]
        : [{ role: 'delete' }, { type: 'separator' }, { role: 'selectAll' }])
    ]
  },
  // { role: 'viewMenu' }
  {
    label: 'View',
    submenu: [
      { role: 'reload' },
      { role: 'forceReload' },
      { role: 'toggleDevTools' },
      { type: 'separator' },
      { role: 'resetZoom' },
      { role: 'zoomIn' },
      { role: 'zoomOut' },
      { type: 'separator' },
      { role: 'togglefullscreen' }
    ]
  },
  // { role: 'windowMenu' }
  {
    label: 'Window',
    submenu: [
      { role: 'minimize' },
      { role: 'zoom' },
      ...(isMac
        ? [{ type: 'separator' }, { role: 'front' }, { type: 'separator' }, { role: 'window' }]
        : [{ role: 'close' }])
    ]
  },
  {
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click: async () => {
          await shell.openExternal('https://electronjs.org')
        }
      }
    ]
  }
]

const menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)
```

> [!IMPORTANT]
> On macOS, the first submenu of the application menu will **always** have your application's name
> as its label. In general, you can populate this submenu by conditionally adding a menu item with
> the `appMenu` role.

> [!TIP]
> For additional descriptions of available roles, see the [`MenuItem` roles](./menus.md#roles)
> section of the general Menus guide.

### Using standard OS menu roles

Defining each submenu explicitly can get very verbose. If you want to re-use default submenus
in your app, you can use various submenu-related roles provided by Electron.

```js title='Using default roles for each submenu' @ts-expect-error=[23]
const { shell } = require('electron/common')
const { app, Menu } = require('electron/main')

const template = [
  ...(process.platform === 'darwin' ? [{ role: 'appMenu' }] : []),
  { role: 'fileMenu' },
  { role: 'editMenu' },
  { role: 'viewMenu' },
  { role: 'windowMenu' },
  {
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click: async () => {
          await shell.openExternal('https://electronjs.org')
        }
      }
    ]
  }
]

const menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)
```

> [!NOTE]
> On macOS, the `help` role defines a top-level Help submenu that has a search bar for
> other menu items. It requires items to be added to its `submenu` to function.

## Supporting "Use Selection for Find" _macOS_

Most macOS apps offer a **Use Selection for Find** command (<kbd>Cmd+E</kbd>) in their
Edit > Find menu. It copies the selected text into the app's search field, and
<kbd>Cmd+G</kbd> then jumps to the next match.

macOS also shares the search text between apps through a dedicated _find pasteboard_,
which is separate from the regular clipboard. When you search for a word in one app and
switch to another app, <kbd>Cmd+G</kbd> keeps searching for the same word. To behave like
a native app, your app should:

1. Write the search text to the find pasteboard whenever it changes.
2. Read the find pasteboard when the app becomes active, and put that text in its search field.

Electron has no built-in menu role for this behavior, but you can build it with the
[`clipboard`](../api/clipboard.md) module, which exposes the find pasteboard as the
`electron application/findtext` custom format.

### Writing and reading the find pasteboard

```js
const { ClipboardItem, clipboard } = require('electron/main')

const FIND_TEXT_MIME = 'electron application/findtext'

// Writing the find pasteboard leaves the regular clipboard untouched.
async function writeFindPasteboard(text) {
  await clipboard.write([new ClipboardItem({ [FIND_TEXT_MIME]: text })])
}

async function readFindPasteboard() {
  const items = await clipboard.read()
  for (const item of items) {
    if (item.types.includes(FIND_TEXT_MIME)) {
      const blob = await item.getType(FIND_TEXT_MIME)
      return blob.text()
    }
  }
  return ''
}
```

### Wiring up the Find menu

Only the renderer process knows what text is selected, so the menu item's `click` handler
asks the focused window for its selection over [IPC](./ipc.md). The renderer then reports
the text back to the main process, which writes it to the find pasteboard.

```js
const { BrowserWindow, Menu } = require('electron/main')

const menu = Menu.buildFromTemplate([
  { role: 'appMenu' },
  {
    label: 'Edit',
    submenu: [
      { role: 'copy' },
      { role: 'paste' },
      { type: 'separator' },
      {
        label: 'Find',
        submenu: [
          {
            label: 'Use Selection for Find',
            accelerator: 'Command+E',
            click: () => {
              BrowserWindow.getFocusedWindow()?.webContents.send('find:use-selection')
            }
          }
        ]
      }
    ]
  }
])

Menu.setApplicationMenu(menu)
```

Use [`app`'s `did-become-active` event](../api/app.md#event-did-become-active-macos) to
pick up text that another app wrote to the find pasteboard while your app was in the background.
For <kbd>Cmd+G</kbd> and <kbd>Shift+Cmd+G</kbd>, pass the search text to
[`webContents.findInPage`](../api/web-contents.md#contentsfindinpagetext-options).

> [!NOTE]
> `findInPage` also matches text inside the page's own form fields. If your search field is
> an `<input>` in the same page that you are searching, the search text counts as a match.
> The example below avoids this by clearing the field when a search starts and showing the
> current search text with CSS generated content, which `findInPage` does not match.

The full example below implements all of this, including Find Next and Find Previous:

```fiddle docs/fiddles/menus/use-selection-for-find
```

## Setting window-specific application menus _Linux_ _Windows_

Since the root application menu exists on each `BaseWindow` on Windows and Linux, you can override
it with a window-specific `Menu` instance via the [`win.setMenu`](../api/browser-window.md#winsetmenumenu-linux-windows) method.

```js title='Override a window's menu'
const { BrowserWindow, Menu } = require('electron/main')

const win = new BrowserWindow()
const menu = Menu.buildFromTemplate([
  {
    label: 'my custom menu',
    submenu: [{ role: 'copy' }, { role: 'paste' }]
  }
])
win.setMenu(menu)
```

> [!TIP]
> You can remove a specific window's application menu by calling the
> [`win.removeMenu`](../api/base-window.md#winremovemenu-linux-windows) API.
