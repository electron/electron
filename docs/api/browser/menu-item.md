# menu-item

## Class: MenuItem

### new MenuItem(options)

* `options` Object
  * `click` Function - Callback when the menu item is clicked
  * `selector` String - Call the selector of first responder when clicked (OS
     X only)
  * `type` String - Can be `normal`, `separator`, `submenu`, `checkbox` or
     `radio`
  * `label` String
  * `sublabel` String
  * `accelerator` String - In the form of `Command+R`, `Ctrl+C`,
    `Shift+Command+D`, `D`, etc.
  * `enabled` Boolean
  * `visible` Boolean
  * `checked` Boolean
  * `groupId` Boolean - Should be specified for `radio` type menu item
  * `submenu` Menu - Should be specified for `submenu` type menu item, when
     it's specified the `type: 'submenu'` can be omitted for the menu item

## Notes on accelerator

On OS X, the `Ctrl` would automatically translated to `Command`, if you really
want `Ctrl` on OS X, you should use `MacCtrl`.
