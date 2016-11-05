# MenuItem

> Add items to native application menus and context menus.

Process: [Main](../tutorial/quick-start.md#main-process)

See [`Menu`](menu.md) for examples.

## Class: MenuItem

Create a new `MenuItem` with the following method:

### `new MenuItem(options)`

* `options` Object
  * `click` Function - (optional) Will be called with
    `click(menuItem, browserWindow, event)` when the menu item is clicked.
    * `menuItem` MenuItem
    * `browserWindow` BrowserWindow
    * `event` Event
  * `role` String - (optional) Define the action of the menu item, when specified the
    `click` property will be ignored.
  * `type` String - (optional) Can be `normal`, `separator`, `submenu`, `checkbox` or
    `radio`.
  * `label` String - (optional)
  * `sublabel` String - (optional)
  * `accelerator` [Accelerator](accelerator.md) - (optional)
  * `icon` ([NativeImage](native-image.md) | String) - (optional)
  * `enabled` Boolean - (optional) If false, the menu item will be greyed out and
    unclickable.
  * `visible` Boolean - (optional) If false, the menu item will be entirely hidden.
  * `checked` Boolean - (optional) Should only be specified for `checkbox` or `radio` type
    menu items.
  * `submenu` Menu - (optional) Should be specified for `submenu` type menu items. If
    `submenu` is specified, the `type: 'submenu'` can be omitted. If the value
    is not a `Menu` then it will be automatically converted to one using
    `Menu.buildFromTemplate`.
  * `id` String - (optional) Unique within a single menu. If defined then it can be used
    as a reference to this item by the position attribute.
  * `position` String - (optional) This field allows fine-grained definition of the
    specific location within a given menu.

It is best to specify `role` for any menu item that matches a standard role,
rather than trying to manually implement the behavior in a `click` function.
The built-in `role` behavior will give the best native experience.

The `label` and `accelerator` are optional when using a `role` and will default
to appropriate values for each platform.

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
* `resetzoom` - Reset the focused page's zoom level to the original size
* `zoomin` - Zoom in the focused page by 10%
* `zoomout` - Zoom out the focused page by 10%

On macOS `role` can also have following additional values:

* `about` - Map to the `orderFrontStandardAboutPanel` action
* `hide` - Map to the `hide` action
* `hideothers` - Map to the `hideOtherApplications` action
* `unhide` - Map to the `unhideAllApplications` action
* `startspeaking` - Map to the `startSpeaking` action
* `stopspeaking` - Map to the `stopSpeaking` action
* `front` - Map to the `arrangeInFront` action
* `zoom` - Map to the `performZoom` action
* `window` - The submenu is a "Window" menu
* `help` - The submenu is a "Help" menu
* `services` - The submenu is a "Services" menu

When specifying `role` on macOS, `label` and `accelerator` are the only options
that will affect the MenuItem. All other options will be ignored.

### Instance Properties

The following properties are available on instances of `MenuItem`:

#### `menuItem.enabled`

A Boolean indicating whether the item is enabled, this property can be
dynamically changed.

#### `menuItem.visible`

A Boolean indicating whether the item is visible, this property can be
dynamically changed.

#### `menuItem.checked`

A Boolean indicating whether the item is checked, this property can be
dynamically changed.

A `checkbox` menu item will toggle the `checked` property on and off when
selected.

A `radio` menu item will turn on its `checked` property when clicked, and
will turn off that property for all adjacent items in the same menu.

You can add a `click` function for additional behavior.
