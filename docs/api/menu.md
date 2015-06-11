# menu

The `Menu` class is used to create native menus that can be used as
application menus and context menus. Each menu consists of multiple menu
items, and each menu item can have a submenu.

Below is an example of creating a menu dynamically in a web page by using
the [remote](remote.md) module, and showing it when the user right clicks
the page:

```html
<!-- index.html -->
<script>
var remote = require('remote');
var Menu = remote.require('menu');
var MenuItem = remote.require('menu-item');

var menu = new Menu();
menu.append(new MenuItem({ label: 'MenuItem1', click: function() { console.log('item 1 clicked'); } }));
menu.append(new MenuItem({ type: 'separator' }));
menu.append(new MenuItem({ label: 'MenuItem2', type: 'checkbox', checked: true }));

window.addEventListener('contextmenu', function (e) {
  e.preventDefault();
  menu.popup(remote.getCurrentWindow());
}, false);
</script>
```

Another example of creating the application menu with the simple template API:

```html
<!-- index.html -->
<script>
var remote = require('remote');
var Menu = remote.require('menu');
var template = [
  {
    label: 'Electron',
    submenu: [
      {
        label: 'About Electron',
        selector: 'orderFrontStandardAboutPanel:'
      },
      {
        type: 'separator'
      },
      {
        label: 'Services',
        submenu: []
      },
      {
        type: 'separator'
      },
      {
        label: 'Hide Electron',
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
        selector: 'terminate:'
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
      }
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'Command+R',
        click: function() { remote.getCurrentWindow().reload(); }
      },
      {
        label: 'Toggle DevTools',
        accelerator: 'Alt+Command+I',
        click: function() { remote.getCurrentWindow().toggleDevTools(); }
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
      }
    ]
  },
  {
    label: 'Help',
    submenu: []
  }
];

menu = Menu.buildFromTemplate(template);

Menu.setApplicationMenu(menu);
</script>
```

## Class: Menu

### new Menu()

Creates a new menu.

### Class Method: Menu.setApplicationMenu(menu)

* `menu` Menu

Sets `menu` as the application menu on OS X. On Windows and Linux, the `menu`
will be set as each window's top menu.

### Class Method: Menu.sendActionToFirstResponder(action)

* `action` String

Sends the `action` to the first responder of application, this is used for
emulating default Cocoa menu behaviors, usually you would just use the
`selector` property of `MenuItem`.

**Note:** This method is OS X only.

### Class Method: Menu.buildFromTemplate(template)

* `template` Array

Generally, the `template` is just an array of `options` for constructing
[MenuItem](menu-item.md), the usage can be referenced above.

You can also attach other fields to element of the `template`, and they will
become properties of the constructed menu items.

### Menu.popup(browserWindow, [x, y])

* `browserWindow` BrowserWindow
* `x` Number
* `y` Number

Popups this menu as a context menu in the `browserWindow`. You can optionally
provide a `(x,y)` coordinate to place the menu at, otherwise it will be placed
at the current mouse cursor position.

### Menu.append(menuItem)

* `menuItem` MenuItem

Appends the `menuItem` to the menu.

### Menu.insert(pos, menuItem)

* `pos` Integer
* `menuItem` MenuItem

Inserts the `menuItem` to the `pos` position of the menu.

### Menu.items

Get the array containing the menu's items.

## Notes on OS X application menu

OS X has a completely different style of application menu from Windows and
Linux, and here are some notes on making your app's menu more native-like.

### Standard menus

On OS X there are many system defined standard menus, like the `Services` and
`Windows` menus. To make your menu a standard menu, you can just set your menu's
label to one of followings, and Electron will recognize them and make them
become standard menus:

* `Window`
* `Help`
* `Services`

### Standard menu item actions

OS X has provided standard actions for some menu items (which are called
`selector`s), like `About xxx`, `Hide xxx`, and `Hide Others`. To set the action
of a menu item to a standard action, you can set the `selector` attribute of the
menu item.

### Main menu's name

On OS X the label of application menu's first item is always your app's name,
no matter what label you set. To change it you have to change your app's name
by modifying your app bundle's `Info.plist` file. See
[About Information Property List Files](https://developer.apple.com/library/ios/documentation/general/Reference/InfoPlistKeyReference/Articles/AboutInformationPropertyListFiles.html)
for more.


## Menu item position

You can make use of `position` and `id` to control how the item would be placed
when building a menu with `Menu.buildFromTemplate`.

The `position` attribute of `MenuItem` has the form `[placement]=[id]` where
placement is one of `before`, `after`, or `endof` and `id` is the unique ID of
an existing item in the menu:

* `before` - Inserts this item before the id referenced item. If the
  referenced item doesn't exist the item will be inserted at the end of
  the menu.
* `after` - Inserts this item after id referenced item. If the referenced
  item doesn't exist the item will be inserted at the end of the menu.
* `endof` - Inserts this item at the end of the logical group containing
  the id referenced item. (Groups are created by separator items). If
  the referenced item doesn't exist a new separator group is created with
  the given id and this item is inserted after that separator.

When an item is positioned following unpositioned items are inserted after
it, until a new item is positioned. So if you want to position a group of
menu items in the same location you only need to specify a position for
the first item.

### Examples

Template:

```javascript
[
  {label: '4', id: '4'},
  {label: '5', id: '5'},
  {label: '1', id: '1', position: 'before=4'},
  {label: '2', id: '2'},
  {label: '3', id: '3'}
]
```

Menu:

```
- 1
- 2
- 3
- 4
- 5
```

Template:

```javascript
[
  {label: 'a', position: 'endof=letters'},
  {label: '1', position: 'endof=numbers'},
  {label: 'b', position: 'endof=letters'},
  {label: '2', position: 'endof=numbers'},
  {label: 'c', position: 'endof=letters'},
  {label: '3', position: 'endof=numbers'}
]
```

Menu:

```
- ---
- a
- b
- c
- ---
- 1
- 2
- 3
```
