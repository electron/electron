# menu

The `Menu` class is used to create native menus that can be used as
application menu and context menu. Each menu is consisted of multiple menu
items, and each menu item can have a submenu.

An example of creating a menu dynamically and show it when user right clicks
the page:

```javascript
var Menu = require('menu');
var MenuItem = require('menu-item');

var menu = new Menu();
menu.append(new MenuItem({ label: 'MenuItem1', click: function() { console.log('item 1clicked'); } }));
menu.append(new MenuItem({ type: 'separator' }));
menu.append(new MenuItem({ label: 'MenuItem2', type: 'checkbox', clicked: true }));

window.addEventListener('contextmenu', function (e) {
  e.preventDefault();
  menu.popup();
}, false);
```

Another example of creating the application menu with the simple template API:

```javascript
var template = [
  {
    label: 'Atom Shell',
    submenu: [
      {
        label: 'About Atom Shell',
        selector: 'orderFrontStandardAboutPanel:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Hide Atom Shell',
        accelerator: 'Command+H',
        selector: 'hide:'
      },
      {
        label: 'Hide Others',
        accelerator: 'Command+Shift+H',
        selector: 'hideOtherApplications:'
      },
      {
        label: 'Show All',
        selector: 'unhideAllApplications:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function() { app.quit(); }
      },
    ]
  },
  {
    label: 'Edit',
    submenu: [
      {
        label: 'Undo',
        accelerator: 'Command+Z',
        selector: 'undo:'
      },
      {
        label: 'Redo',
        accelerator: 'Shift+Command+Z',
        selector: 'redo:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Cut',
        accelerator: 'Command+X',
        selector: 'cut:'
      },
      {
        label: 'Copy',
        accelerator: 'Command+C',
        selector: 'copy:'
      },
      {
        label: 'Paste',
        accelerator: 'Command+V',
        selector: 'paste:'
      },
      {
        label: 'Select All',
        accelerator: 'Command+A',
        selector: 'selectAll:'
      },
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'Command+R',
        click: function() { BrowserWindow.getFocusedWindow().reloadIgnoringCache(); }
      },
      {
        label: 'Toggle DevTools',
        accelerator: 'Alt+Command+I',
        click: function() { BrowserWindow.getFocusedWindow().toggleDevTools(); }
      },
    ]
  },
  {
    label: 'Window',
    submenu: [
      {
        label: 'Minimize',
        accelerator: 'Command+M',
        selector: 'performMiniaturize:'
      },
      {
        label: 'Close',
        accelerator: 'Command+W',
        selector: 'performClose:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Bring All to Front',
        selector: 'arrangeInFront:'
      },
    ]
  },
];

menu = Menu.buildFromTemplate(template);
Menu.setApplicationMenu(menu);
```

## Class: Menu

### new Menu()

Creates a new menu.

### Class Method: Menu.setApplicationMenu(menu)

* `menu` Menu

Sets `menu` as the application menu.

**Note:** This method is OS X only.

### Class Method: Menu.sendActionToFirstResponder(action)

* `action` String

Sends the `action` to the first responder of application, this is used for
emulating default Cocoa menu behaviors, usually you would just use the
`selector` property of `MenuItem`.

**Note:** This method is OS X only.

### Class Method: Menu.buildFromTemplate(template)

* `template` Array

Generally, the `template` is just an array of `options` for constructing
`MenuItem`, the usage can be referenced above.

You can also attach other fields to element of the `template`, and they will
become properties of the constructed menu items.

### Menu.popup(browserWindow)

* `browserWindow` BrowserWindow

Popups the this as context menu in the `browserWindow`.

### Menu.append(menuItem)

* `menuItem` MenuItem

Appends the `menuItem` to the menu.

### Menu.insert(pos, menuItem)

* `pos` Integer
* `menuItem` MenuItem

Inserts the `menuItem` to the `pos` position of the menu.

### Menu.items

Get the array containing the menu's items.
