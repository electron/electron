# MenuItem

## Class: MenuItem

> Add items to native application menus and context menus.

Process: [Main](../glossary.md#main-process)

See [`Menu`](menu.md) for examples.

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new MenuItem(options)`

* `options` Object
  * `click` Function (optional) - Will be called with
    `click(menuItem, window, event)` when the menu item is clicked.
    * `menuItem` [MenuItem](menu-item.md)
    * `window` [BaseWindow](base-window.md) | undefined - This will not be defined if no window is open.
    * `event` [KeyboardEvent](structures/keyboard-event.md)
  * `role` string (optional) - Can be `undo`, `redo`, `cut`, `copy`, `paste`, `pasteAndMatchStyle`, `delete`, `selectAll`, `reload`, `forceReload`, `toggleDevTools`, `resetZoom`, `zoomIn`, `zoomOut`, `toggleSpellChecker`, `togglefullscreen`, `window`, `minimize`, `close`, `help`, `about`, `services`, `hide`, `hideOthers`, `unhide`, `quit`, `showSubstitutions`, `toggleSmartQuotes`, `toggleSmartDashes`, `toggleTextReplacement`, `startSpeaking`, `stopSpeaking`, `zoom`, `front`, `appMenu`, `fileMenu`, `editMenu`, `viewMenu`, `shareMenu`, `recentDocuments`, `toggleTabBar`, `selectNextTab`, `selectPreviousTab`, `showAllTabs`, `mergeAllWindows`, `clearRecentDocuments`, `moveTabToNewWindow` or `windowMenu` - Define the action of the menu item, when specified the
    `click` property will be ignored. See [roles](../tutorial/menus.md#roles).
  * `type` string (optional)
    * `normal`
    * `separator`
    * `submenu`
    * `checkbox`
    * `radio`
    * `header` - Only available on macOS 14 and up.
    * `palette` - Only available on macOS 14 and up.
  * `label` string (optional)
  * `sublabel` string (optional) _macOS_ - Available in macOS >= 14.4
  * `toolTip` string (optional) _macOS_ - Hover text for this menu item.
  * `accelerator` string (optional) - An [Accelerator](../tutorial/keyboard-shortcuts.md#accelerators) string.
  * `icon` ([NativeImage](native-image.md) | string) (optional) - Can be a
    [NativeImage](native-image.md) or the file path of an icon.
  * `enabled` boolean (optional) - If false, the menu item will be greyed out and
    unclickable.
  * `acceleratorWorksWhenHidden` boolean (optional) _macOS_ - default is `true`, and when `false` will prevent the accelerator from triggering the item if the item is not visible.
  * `visible` boolean (optional) - If false, the menu item will be entirely hidden.
  * `checked` boolean (optional) - Should only be specified for `checkbox` or `radio` type
    menu items.
  * `registerAccelerator` boolean (optional) _Linux_ _Windows_ - If false, the accelerator won't be registered
    with the system, but it will still be displayed. Defaults to true.
  * `sharingItem` SharingItem (optional) _macOS_ - The item to share when the `role` is `shareMenu`.
  * `submenu` (MenuItemConstructorOptions[] | [Menu](menu.md)) (optional) - Should be specified
    for `submenu` type menu items. If `submenu` is specified, the `type: 'submenu'` can be omitted.
    If the value is not a [`Menu`](menu.md) then it will be automatically converted to one using
    `Menu.buildFromTemplate`.
  * `id` string (optional) - Unique within a single menu. If defined then it can be used
    as a reference to this item by the position attribute.
  * `before` string[] (optional) - Inserts this item before the item with the specified id. If
    the referenced item doesn't exist the item will be inserted at the end of  the menu. Also implies
    that the menu item in question should be placed in the same “group” as the item.
  * `after` string[] (optional) - Inserts this item after the item with the specified id. If the
    referenced item doesn't exist the item will be inserted at the end of
    the menu.
  * `beforeGroupContaining` string[] (optional) - Provides a means for a single context menu to declare
    the placement of their containing group before the containing group of the item
    with the specified id.
  * `afterGroupContaining` string[] (optional) - Provides a means for a single context menu to declare
    the placement of their containing group after the containing group of the item
    with the specified id.

> [!NOTE]
> `acceleratorWorksWhenHidden` is specified as being macOS-only because accelerators always work when items are hidden on Windows and Linux. The option is exposed to users to give them the option to turn it off, as this is possible in native macOS development.

### Instance Properties

The following properties are available on instances of `MenuItem`:

#### `menuItem.id`

A `string` indicating the item's unique id. This property can be
dynamically changed.

#### `menuItem.label`

A `string` indicating the item's visible label.

#### `menuItem.click`

A `Function` that is fired when the MenuItem receives a click event.
It can be called with `menuItem.click(event, focusedWindow, focusedWebContents)`.

* `event` [KeyboardEvent](structures/keyboard-event.md)
* `focusedWindow` [BaseWindow](browser-window.md)
* `focusedWebContents` [WebContents](web-contents.md)

#### `menuItem.submenu`

A [`Menu`](menu.md) (optional) containing the menu
item's submenu, if present.

#### `menuItem.type`

A `string` indicating the type of the item. Can be `normal`, `separator`, `submenu`, `checkbox`, `radio`, `header` or `palette`.

> [!NOTE]
> `header` and `palette` are only available on macOS 14 and up.

#### `menuItem.role`

A `string` (optional) indicating the item's role, if set. Can be `undo`, `redo`, `cut`, `copy`, `paste`, `pasteAndMatchStyle`, `delete`, `selectAll`, `reload`, `forceReload`, `toggleDevTools`, `resetZoom`, `zoomIn`, `zoomOut`, `toggleSpellChecker`, `togglefullscreen`, `window`, `minimize`, `close`, `help`, `about`, `services`, `hide`, `hideOthers`, `unhide`, `quit`, `startSpeaking`, `stopSpeaking`, `zoom`, `front`, `appMenu`, `fileMenu`, `editMenu`, `viewMenu`, `shareMenu`, `recentDocuments`, `toggleTabBar`, `selectNextTab`, `selectPreviousTab`, `showAllTabs`, `mergeAllWindows`, `clearRecentDocuments`, `moveTabToNewWindow` or `windowMenu`

#### `menuItem.accelerator`

An `Accelerator | null` indicating the item's accelerator, if set.

#### `menuItem.userAccelerator` _Readonly_ _macOS_

An `Accelerator | null` indicating the item's [user-assigned accelerator](https://developer.apple.com/documentation/appkit/nsmenuitem/1514850-userkeyequivalent?language=objc) for the menu item.

> [!NOTE]
> This property is only initialized after the `MenuItem` has been added to a `Menu`. Either via `Menu.buildFromTemplate` or via `Menu.append()/insert()`.  Accessing before initialization will just return `null`.

#### `menuItem.icon`

A `NativeImage | string` (optional) indicating the
item's icon, if set.

#### `menuItem.sublabel`

A `string` indicating the item's sublabel.

#### `menuItem.toolTip` _macOS_

A `string` indicating the item's hover text.

#### `menuItem.enabled`

A `boolean` indicating whether the item is enabled. This property can be
dynamically changed.

#### `menuItem.visible`

A `boolean` indicating whether the item is visible. This property can be
dynamically changed.

#### `menuItem.checked`

A `boolean` indicating whether the item is checked. This property can be
dynamically changed.

A `checkbox` menu item will toggle the `checked` property on and off when
selected.

A `radio` menu item will turn on its `checked` property when clicked, and
will turn off that property for all adjacent items in the same menu.

You can add a `click` function for additional behavior.

#### `menuItem.registerAccelerator`

A `boolean` indicating if the accelerator should be registered with the
system or just displayed.

This property can be dynamically changed.

#### `menuItem.sharingItem` _macOS_

A `SharingItem` indicating the item to share when the `role` is `shareMenu`.

This property can be dynamically changed.

#### `menuItem.commandId`

A `number` indicating an item's sequential unique id.

#### `menuItem.menu`

A [`Menu`](menu.md) that the item is a part of.
