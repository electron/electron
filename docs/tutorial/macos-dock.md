# macOS Dock

Electron has APIs to configure the app's icon in the macOS Dock. A macOS-only
API exists to create a custom dock menu, but
Electron also uses the app's dock icon to implement cross-platform features
like [recent documents][recent-documents] and
[application progress][progress-bar].

The custom dock is commonly used to add shortcuts to tasks the user wouldn't
want to open the whole app window for.

__Dock menu of Terminal.app:__

![Dock Menu][dock-menu-image]

To set your custom dock menu, you can use the `app.dock.setMenu` API, which is
only available on macOS:

```javascript
const { app, Menu } = require('electron')

const dockMenu = Menu.buildFromTemplate([
  {
    label: 'New Window',
    click () { console.log('New Window') }
  }, {
    label: 'New Window with Settings',
    submenu: [
      { label: 'Basic' },
      { label: 'Pro' }
    ]
  },
  { label: 'New Command...' }
])

app.dock.setMenu(dockMenu)
```

[dock-menu-image]: https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png
[recent-documents]: ./recent-documents.md
[progress-bar]: ./progress-bar.md
