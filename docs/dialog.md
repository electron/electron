## Synopsis

The `dialog` module provides functions to show system dialogs, so web applications can get the same user experience with native applications.

An example of showing a dialog to select multiple files and directories:

```javascript
var win = ...;  // window in which to show the dialog
var dialog = require('dialog');
console.log(dialog.showOpenDialog({ properties: [ 'openFile', 'openDirectory', 'multiSelections' ]}));
```

## dialog.showOpenDialog(options)

* `options` Object
  * `title` String
  * `defaultPath` String
  * `properties` Array - Contains which features the dialog should use, can contain `openFile`, `openDirectory`, `multiSelections` and `createDirectory`

On success, returns an array of file paths chosen by the user, otherwise returns `undefined`.

**Note:** The `dialog.showOpenDialog` API is synchronous and blocks all windows.

## dialog.showSaveDialog(browserWindow, options)

* `browserWindow` BrowserWindow
* `options` Object
  * `title` String
  * `defaultPath` String

On success, returns the path of file chosen by the user, otherwise returns `undefined`.

**Note:** The `dialog.showSaveDialog` API is synchronous and blocks all windows.

## dialog.showMessageBox([browserWindow], options)

* `browserWindow` BrowserWindow
* `options` Object
  * `type` String - Can be `"none"`, `"info"` or `"warning"`
  * `buttons` Array - Array of texts for buttons
  * `title` String - Title of the message box, some platforms will not show it
  * `message` String - Content of the message box
  * `detail` String - Extra information of the message

Shows a message box, it will block until the message box is closed. It returns the index of the clicked button.

**Note:** The `dialog.showMessageBox` API is synchronous and blocks all windows.