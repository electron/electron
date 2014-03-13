# dialog

The `dialog` module provides APIs to show native system dialogs, so web
applications can get the same user experience with native applications.

An example of showing a dialog to select multiple files and directories:

```javascript
var win = ...;  // window in which to show the dialog
var dialog = require('dialog');
console.log(dialog.showOpenDialog({ properties: [ 'openFile', 'openDirectory', 'multiSelections' ]}));
```

## dialog.showOpenDialog([browserWindow], options, [callback])

* `browserWindow` BrowserWindow
* `options` Object
  * `title` String
  * `defaultPath` String
  * `properties` Array - Contains which features the dialog should use, can
    contain `openFile`, `openDirectory`, `multiSelections` and
    `createDirectory`
* `callback` Function

On success, returns an array of file paths chosen by the user, otherwise
returns `undefined`.

If a `callback` is passed, the API call would be asynchronous and the result
would be passed via `callback(filenames)`

**Note:** On Windows and Linux, an open dialog could not be both file selector
and directory selector at the same time, so if you set `properties` to
`['openFile', 'openDirectory']` on these platforms, a directory selector would
be showed.

## dialog.showSaveDialog([browserWindow], options, [callback])

* `browserWindow` BrowserWindow
* `options` Object
  * `title` String
  * `defaultPath` String
* `callback` Function

On success, returns the path of file chosen by the user, otherwise returns
`undefined`.

If a `callback` is passed, the API call would be asynchronous and the result
would be passed via `callback(filename)`

## dialog.showMessageBox([browserWindow], options, [callback])

* `browserWindow` BrowserWindow
* `options` Object
  * `type` String - Can be `"none"`, `"info"` or `"warning"`
  * `buttons` Array - Array of texts for buttons
  * `title` String - Title of the message box, some platforms will not show it
  * `message` String - Content of the message box
  * `detail` String - Extra information of the message
* `callback` Function

Shows a message box, it will block until the message box is closed. It returns
the index of the clicked button.

If a `callback` is passed, the API call would be asynchronous and the result
would be passed via `callback(response)`
