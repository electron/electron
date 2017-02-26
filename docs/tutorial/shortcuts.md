# Keyboard Shortcuts

> Configure local and global keyboard shortcuts

## Global Shortcuts

You can use the [globalShortcut] module to detect keyboard events even when
the application does not have keyboard focus.

```js
const {app, globalShortcut} = require('electron')

app.on('ready', () => {
  globalShortcut.register('CommandOrControl+X', () => {
    console.log('CommandOrControl+X is pressed')
  })
})
```

## Local Shortcuts

You can use the [Menu] module to configure keyboard shortcuts that will
be triggered only when the app is focused. To do so, specify an
[`accelerator`] property when creating a [MenuItem].

```js
const {Menu, MenuItem} = require('electron')
const menu = new Menu()

menu.append(new MenuItem({
  label: 'Print',
  accelerator: 'CmdOrCtrl+P'
  click: () => { console.log('time to print stuff') }
}))
```

It's easy to configure different key combinations based on the user's operating system.

```js
{
  accelerator: process.platform === 'darwin' ? 'Alt+Cmd+I' : 'Ctrl+Shift+I'
}
```


## Local Shortcuts without a Menu

If you want to configure a local keyboard shortcut to trigger an action that
_does not_ have a corresponding menu item, you can use the
[electron-localshortcut] npm module.

If you want to handle keyboard shortcuts for a [BrowserWindow], you can use the `keyup` and `keydown` event listeners on the window object inside the renderer process.

```js
window.addEventListener('keyup', doSomething, true)
```

Note the third parameter `true` which means the listener will always receive key presses before other listeners so they can't have `stopPropagation()` called on them.

[Menu]: ../api/menu.md
[MenuItem]: ../api/menu-item.md
[globalShortcut]: ../api/global-shortcut.md
[`accelerator`]: ../api/accelerator.md
[electron-localshortcut]: http://ghub.io/electron-localshortcut
[BrowserWindow]: ../api/browser-window.md
