# shell

> Manage files and URLs using their default applications.

The `shell` module provides functions related to desktop integration.

An example of opening a URL in the user's default browser:

```javascript
const {shell} = require('electron')

shell.openExternal('https://github.com')
```

## Methods

The `shell` module has the following methods:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

Returns `Boolean` - Whether the item was successfully shown

Show the given file in a file manager. If possible, select the file.

### `shell.openItem(fullPath)`

* `fullPath` String

Returns `Boolean` - Whether the item was successfully opened.

Open the given file in the desktop's default manner.

### `shell.openExternal(url[, options])`

* `url` String
* `options` Object (optional) _macOS_
  * `activate` Boolean - `true` to bring the opened application to the
    foreground. The default is `true`.

Returns `Boolean` - Whether an application was available to open the URL.

Open the given external protocol URL in the desktop's default manner. (For
example, mailto: URLs in the user's default mail agent).

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Returns `Boolean` - Whether the item was successfully moved to the trash

Move the given file to trash and returns a boolean status for the operation.

### `shell.beep()`

Play the beep sound.

### `shell.writeShortcutLink(shortcutPath[, operation], options)` _Windows_

* `shortcutPath` String
* `operation` String (optional) - Default is `create`, can be one of following:
  * `create` - Creates a new shortcut, overwriting if necessary.
  * `update` - Updates specified properties only on an existing shortcut.
  * `replace` - Overwrites an existing shortcut, fails if the shortcut doesn't
    exist.
* `options` Object
  * `target` String - The target to launch from this shortcut.
  * `cwd` String (optional) - The working directory. Default
    is empty.
  * `args` String (optional) - The arguments to be applied to `target` when
    launching from this shortcut. Default is empty.
  * `description` String (optional) - The description of the shortcut. Default
    is empty.
  * `icon` String (optional) - The path to the icon, can be a DLL or EXE. `icon`
    and `iconIndex` have to be set together. Default is empty, which uses the
    target's icon.
  * `iconIndex` Integer (optional) - The resource ID of icon when `icon` is a
    DLL or EXE. Default is 0.
  * `appUserModelId` String (optional) - The Application User Model ID. Default
    is empty.

Returns `Boolean` - Whether the shortcut was created successfully

Creates or updates a shortcut link at `shortcutPath`.

### `shell.readShortcutLink(shortcutPath)` _Windows_

* `shortcutPath` String

Returns `Object`:
* `target` String - The target to launch from this shortcut.
* `cwd` String (optional) - The working directory. Default is empty.
* `args` String (optional) - The arguments to be applied to `target` when
launching from this shortcut. Default is empty.
* `description` String (optional) - The description of the shortcut. Default
is empty.
* `icon` String (optional) - The path to the icon, can be a DLL or EXE. `icon`
and `iconIndex` have to be set together. Default is empty, which uses the
target's icon.
* `iconIndex` Integer (optional) - The resource ID of icon when `icon` is a
DLL or EXE. Default is 0.
* `appUserModelId` String (optional) - The Application User Model ID. Default
is empty.

Resolves the shortcut link at `shortcutPath`.

An exception will be thrown when any error happens.
