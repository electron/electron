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

Show the given file in a file manager. If possible, select the file. Returns
true if the item was successfully shown, false otherwise.

### `shell.openItem(fullPath)`

* `fullPath` String

Open the given file in the desktop's default manner. Returns true if the item
was successfully opened, false otherwise.

### `shell.openExternal(url[, options])`

* `url` String
* `options` Object (optional) _macOS_
  * `activate` Boolean - `true` to bring the opened application to the
    foreground. The default is `true`.

Open the given external protocol URL in the desktop's default manner. (For
example, mailto: URLs in the user's default mail agent.) Returns true if an
application was available to open the URL, false otherwise.

### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Move the given file to trash and returns a boolean status for the operation.

### `shell.beep()`

Play the beep sound.

### `shell.writeShortcutLink(shortcutPath[, operation], options)` _Windows_

* `shortcutPath` String
* `operation` String (optional) - Default is `create`, can be one of followings:
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

Creates or updates a shortcut link at `shortcutPath`. On success `true` is
returned, otherwise `false` is returned.

### `shell.readShortcutLink(shortcutPath)` _Windows_

* `shortcutPath` String

Resolves the shortcut link at `shortcutPath`. An object is returned with the
fields described in the `options` of `shell.writeShortcutLink`.

An exception will be thrown when any error happens.
