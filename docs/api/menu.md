# Menu

## Class: Menu

> Create application menus and context menus.

Process: [Main](../glossary.md#main-process)

The presentation of menus varies depending on the operating system:

- Under Windows and Linux, menus are visually similar to Chromium.
- Under macOS, these will be native menus.

> [!TIP]
> See also: [A detailed guide about how to implement menus in your application](../tutorial/menus.md).

> [!WARNING]
> Electron's built-in classes cannot be subclassed in user code.
> For more information, see [the FAQ](../faq.md#class-inheritance-does-not-work-with-electron-built-in-modules).

### `new Menu()`

Creates a new menu.

### Static Methods

The `Menu` class has the following static methods:

#### `Menu.setApplicationMenu(menu)`

- `menu` [Menu](menu.md) | null

Sets `menu` as the application menu on macOS. On Windows and Linux, the
`menu` will be set as each window's top menu.

Also on Windows and Linux, you can use a `&` in the top-level item name to
indicate which letter should get a generated accelerator. For example, using
`&File` for the file menu would result in a generated `Alt-F` accelerator that
opens the associated menu. The indicated character in the button label then gets an
underline, and the `&` character is not displayed on the button label.

In order to escape the `&` character in an item name, add a preceding `&`. For example, `&&File` would result in `&File` displayed on the button label.

Passing `null` will suppress the default menu. On Windows and Linux,
this has the additional effect of removing the menu bar from the window.

> [!NOTE]
> The default menu will be created automatically if the app does not set one.
> It contains standard items such as `File`, `Edit`, `View`, `Window` and `Help`.

#### `Menu.getApplicationMenu()`

Returns `Menu | null` - The application menu, if set, or `null`, if not set.

> [!NOTE]
> The returned `Menu` instance doesn't support dynamic addition or
> removal of menu items. [Instance properties](#instance-properties) can still
> be dynamically modified.

#### `Menu.sendActionToFirstResponder(action)` _macOS_

- `action` string

Sends the `action` to the first responder of application. This is used for
emulating default macOS menu behaviors. Usually you would use the
[`role`](../tutorial/menus.md#roles) property of a [`MenuItem`](menu-item.md).

See the [macOS Cocoa Event Handling Guide](https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/EventOverview/EventArchitecture/EventArchitecture.html#//apple_ref/doc/uid/10000060i-CH3-SW7)
for more information on macOS' native actions.

#### `Menu.buildFromTemplate(template)`

- `template` (MenuItemConstructorOptions | [MenuItem](menu-item.md))[]

Returns [`Menu`](menu.md)

Generally, the `template` is an array of `options` for constructing a
[MenuItem](menu-item.md). The usage can be referenced above.

You can also attach other fields to the element of the `template` and they will become properties of the constructed menu items.

### Instance Methods

The `menu` object has the following instance methods:

#### `menu.popup([options])`

- `options` Object (optional)
  - `window` [BaseWindow](base-window.md) (optional) - Default is the focused window.
  - `frame` [WebFrameMain](web-frame-main.md) (optional) - Provide the relevant frame
    if you want certain OS-level features such as Writing Tools on macOS to function correctly. Typically, this should be `params.frame` from the [`context-menu` event](web-contents.md#event-context-menu) on a WebContents, or the [`focusedFrame` property](web-contents.md#contentsfocusedframe-readonly) of a WebContents.
  - `x` number (optional) - Default is the current mouse cursor position.
    Must be declared if `y` is declared.
  - `y` number (optional) - Default is the current mouse cursor position.
    Must be declared if `x` is declared.
  - `positioningItem` number (optional) _macOS_ - The index of the menu item to
    be positioned under the mouse cursor at the specified coordinates. Default
    is -1.
  - `sourceType` string (optional) _Windows_ _Linux_ - This should map to the `menuSourceType`
    provided by the `context-menu` event. It is not recommended to set this value manually,
    only provide values you receive from other APIs or leave it `undefined`.
    Can be `none`, `mouse`, `keyboard`, `touch`, `touchMenu`, `longPress`, `longTap`, `touchHandle`, `stylus`, `adjustSelection`, or `adjustSelectionReset`.
  - `callback` Function (optional) - Called when menu is closed.

Pops up this menu as a context menu in the [`BaseWindow`](base-window.md).

> [!TIP]
> For more details, see the [Context Menu](../tutorial/context-menu.md) guide.

#### `menu.closePopup([window])`

- `window` [BaseWindow](base-window.md) (optional) - Default is the focused window.

Closes the context menu in the `window`.

#### `menu.append(menuItem)`

- `menuItem` [MenuItem](menu-item.md)

Appends the `menuItem` to the menu.

#### `menu.getMenuItemById(id)`

- `id` string

Returns [`MenuItem | null`](menu-item.md) - the item with the specified `id`

#### `menu.insert(pos, menuItem)`

- `pos` Integer
- `menuItem` [MenuItem](menu-item.md)

Inserts the `menuItem` to the `pos` position of the menu.

### Instance Events

Objects created with `new Menu` or returned by `Menu.buildFromTemplate` emit the following events:

> [!NOTE]
> Some events are only available on specific operating systems and are
> labeled as such.

#### Event: 'menu-will-show'

Returns:

- `event` Event

Emitted when `menu.popup()` is called.

#### Event: 'menu-will-close'

Returns:

- `event` Event

Emitted when a popup is closed either manually or with `menu.closePopup()`.

### Instance Properties

`menu` objects also have the following properties:

#### `menu.items`

A `MenuItem[]` array containing the menu's items.

Each `Menu` consists of multiple [`MenuItem`](menu-item.md) instances and each `MenuItem`
can nest a `Menu` into its `submenu` property.
