# Keyboard Shortcuts

## Overview

This feature allows you to configure local and global keyboard shortcuts
for your Electron application.

## Example

### Local Shortcuts

Local keyboard shortcuts are triggered only when the application is focused.
To configure a local keyboard shortcut, you need to specify an [`accelerator`]
property when creating a [MenuItem] within the [Menu] module.

Starting with a working application from the
[Quick Start Guide](quick-start.md), update the `main.js` file with the
following lines:

```js
const { Menu, MenuItem } = require('electron')

const menu = new Menu()
menu.append(new MenuItem({
  label: 'Electron',
  submenu: [{
    role: 'help',
    accelerator: process.platform === 'darwin' ? 'Alt+Cmd+I' : 'Ctrl+Shift+I',
    click: () => { console.log('Electron rocks!') }
  }]
}))

Menu.setApplicationMenu(menu)
```

> NOTE: From the code above you can see that the key combination differs based on the
user's operating system: for Mac it is `Alt+Cmd+I` whereas for Linux and
Windows it is `Ctrl+Shift+I`.

After launching the Electron application, you should see the application menu
along with the local shortcut you just defined:

![Menu with a local shortcut](../images/local-shortcut.png)

If you click `Help` or press the defined key combination and then open
the Console, you will see the message that was generated after
triggering the `click` event: "Electron rocks!"

### Global Shortcuts

To configure a local keyboard shortcut, you need to use the [globalShortcut]
module to detect keyboard events even when the application does not have
keyboard focus.

Starting with a working application from the
[Quick Start Guide](quick-start.md), update the `main.js` file with the
following lines:

```js
const { app, globalShortcut } = require('electron')

app.whenReady().then(() => {
  globalShortcut.register('Alt+CommandOrControl+I', () => {
    console.log('Electron loves global shortcuts!')
  })
}).then(createWindow)
```

After launching the Electron application, if you press the defined key
combination and then open the Console, you will see that Electron loves global
shortcuts!

### Shortcuts within a BrowserWindow

#### Overview

If you want to handle keyboard shortcuts for a [BrowserWindow], you can use the
`keyup` and `keydown` event listeners on the window object inside the renderer
process.

```js
window.addEventListener('keyup', doSomething, true)
```

Note the third parameter `true` which means the listener will always receive
key presses before other listeners so they can't have `stopPropagation()`
called on them.

The [`before-input-event`](../api/web-contents.md#event-before-input-event) event
is emitted before dispatching `keydown` and `keyup` events in the page. It can
be used to catch and handle custom shortcuts that are not visible in the menu.

If you don't want to do manual shortcut parsing there are libraries that do
advanced key detection such as [mousetrap].

```js
Mousetrap.bind('4', () => { console.log('4') })
Mousetrap.bind('?', () => { console.log('show shortcuts!') })
Mousetrap.bind('esc', () => { console.log('escape') }, 'keyup')

// combinations
Mousetrap.bind('command+shift+k', () => { console.log('command shift k') })

// map multiple combinations to the same callback
Mousetrap.bind(['command+k', 'ctrl+k'], () => {
  console.log('command k or control k')

  // return false to prevent default behavior and stop event from bubbling
  return false
})

// gmail style sequences
Mousetrap.bind('g i', () => { console.log('go to inbox') })
Mousetrap.bind('* a', () => { console.log('select all') })

// konami code!
Mousetrap.bind('up up down down left right left right b a enter', () => {
  console.log('konami code')
})
```

[Menu]: ../api/menu.md
[MenuItem]: ../api/menu-item.md
[globalShortcut]: ../api/global-shortcut.md
[`accelerator`]: ../api/accelerator.md
[BrowserWindow]: ../api/browser-window.md
[mousetrap]: https://github.com/ccampbell/mousetrap
