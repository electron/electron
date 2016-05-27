# shell

> Manage files and URLs using their default applications.

The `shell` module provides functions related to desktop integration.

An example of opening a URL in the user's default browser:

```javascript
const {shell} = require('electron');

shell.openExternal('https://github.com');
```

## Methods

The `shell` module has the following methods:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

Show the given file in a file manager. If possible, select the file. Expects `fullPath` to be a fully-specified path (`/users/<username>/path/to/file` or `~/path/to/file`). Relative paths (`./file`) do not work.

### `shell.openItem(fullPath)`

* `fullPath` String

Open the given file in the desktop's default manner. Expects `fullPath` to be a fully-specified path (`/users/<username>/path/to/file`). Relative paths (`./file`) also work. Paths using `~` do not work.

### `shell.openExternal(url[, options])`

* `url` String
* `options` Object (optional) _OS X_
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
