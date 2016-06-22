# MenuItem

> Add items to native application menus and context menus.

See [`menu`](menu.md) for examples.

## Class: MenuItem

Create a new `MenuItem` with the following method:

### new MenuItem(options)

* `options` Object
  * `click` Function - Will be called with
     `click(menuItem, browserWindow, event)` when the menu item is clicked
  * `role` String - Define the action of the menu item; when specified the
     `click` property will be ignored
  * `type` String - Can be `normal`, `separator`, `submenu`, `checkbox` or
     `radio`
  * `label` String
  * `sublabel` String
  * `accelerator` [Accelerator](accelerator.md)
  * `icon` [NativeImage](native-image.md)
  * `enabled` Boolean - If false, the menu item will be greyed out and
    unclickable.
  * `visible` Boolean - If false, the menu item will be entirely hidden.
  * `checked` Boolean - Should only be specified for `checkbox` or `radio` type
      menu items.
  * `submenu` Menu - Should be specified for `submenu` type menu items. If
     `submenu` is specified, the `type: 'submenu'` can be omitted. If the value
     is not a `Menu` then it will be automatically converted to one using
     `Menu.buildFromTemplate`.
  * `id` String - Unique within a single menu. If defined then it can be used
     as a reference to this item by the position attribute.
  * `position` String - This field allows fine-grained definition of the
     specific location within a given menu.

It is best to specify `role` for any menu item that matches a standard role,
rather than trying to manually implement the behavior in a `click` function.
The built-in `role` behavior will give the best native experience.

The `role` property can have following values:

* `undo`
* `redo`
* `cut`
* `copy`
* `paste`
* `pasteandmatchstyle`
* `selectall`
* `delete`
* `minimize` - Minimize current window
* `close` - Close current window
* `quit`- Quit the application
* `togglefullscreen`- Toggle full screen mode on the current window

On macOS `role` can also have following additional values:

* `about` - Map to the `orderFrontStandardAboutPanel` action
* `hide` - Map to the `hide` action
* `hideothers` - Map to the `hideOtherApplications` action
* `unhide` - Map to the `unhideAllApplications` action
* `front` - Map to the `arrangeInFront` action
* `zoom` - Map to the `performZoom` action
* `window` - The submenu is a "Window" menu
* `help` - The submenu is a "Help" menu
* `services` - The submenu is a "Services" menu

When specifying `role` on macOS, `label` and `accelerator` are the only options
that will affect the MenuItem. All other options will be ignored.

## Instance Properties

The following properties (and no others) can be updated on an existing `MenuItem`:

  * `enabled` Boolean
  * `visible` Boolean
  * `checked` Boolean

Their meanings are as described above.

A `checkbox` menu item will toggle its `checked` property on and off when
selected. You can add a `click` function to do additional work.

A `radio` menu item will turn on its `checked` property when clicked, and
will turn off that property for all adjacent items in the same menu. Again,
you can add a `click` function for additional behavior.
