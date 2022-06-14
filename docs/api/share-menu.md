# ShareMenu

The `ShareMenu` class creates [Share Menu][share-menu] on macOS, which can be
used to share information from the current context to apps, social media
accounts, and other services.

For including the share menu as a submenu of other menus, please use the
`shareMenu` role of [`MenuItem`](menu-item.md).

## Class: ShareMenu

> Create share menu on macOS.

Process: [Main](../glossary.md#main-process)

### `new ShareMenu(sharingItem)`

* `sharingItem` SharingItem - The item to share.

Creates a new share menu.

### Instance Methods

The `shareMenu` object has the following instance methods:

#### `shareMenu.popup([options])`

* `options` PopupOptions (optional)
  * `browserWindow` [BrowserWindow](browser-window.md) (optional) - Default is the focused window.
  * `x` number (optional) - Default is the current mouse cursor position.
    Must be declared if `y` is declared.
  * `y` number (optional) - Default is the current mouse cursor position.
    Must be declared if `x` is declared.
  * `positioningItem` number (optional) _macOS_ - The index of the menu item to
    be positioned under the mouse cursor at the specified coordinates. Default
    is -1.
  * `callback` Function (optional) - Called when menu is closed.

Pops up this menu as a context menu in the [`BrowserWindow`](browser-window.md).

#### `shareMenu.closePopup([browserWindow])`

* `browserWindow` [BrowserWindow](browser-window.md) (optional) - Default is the focused window.

Closes the context menu in the `browserWindow`.

[share-menu]: https://developer.apple.com/design/human-interface-guidelines/macos/extensions/share-extensions/
