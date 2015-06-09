# dialog

The `dialog` module provides APIs to show native system dialogs, so web
applications can deliver the same user experience as native applications.

An example of showing a dialog to select multiple files and directories:

```javascript
var win = ...;  // window in which to show the dialog
var dialog = require('dialog');
console.log(dialog.showOpenDialog({ properties: [ 'openFile', 'openDirectory', 'multiSelections' ]}));
```

**Note for OS X**: If you want to present dialogs as sheets, the only thing you have to do is provide a `BrowserWindow` reference in the `browserWindow` parameter.

## dialog.showOpenDialog([browserWindow], [options], [callback])

* `browserWindow` BrowserWindow
* `options` Object
  * `title` String
  * `defaultPath` String
  * `filters` Array
  * `properties` Array - Contains which features the dialog should use, can
    contain `openFile`, `openDirectory`, `multiSelections` and
    `createDirectory`
* `callback` Function

On success, returns an array of file paths chosen by the user, otherwise
returns `undefined`.

The `filters` specifies an array of file types that can be displayed or
selected, an example is:

```javascript
{
  filters: [
    { name: 'Images', extensions: ['jpg', 'png', 'gif'] },
    { name: 'Movies', extensions: ['mkv', 'avi', 'mp4'] },
    { name: 'Custom File Type', extensions: ['as'] }
  ]
}
```

If a `callback` is passed, the API call would be asynchronous and the result
would be passed via `callback(filenames)`

**Note:** On Windows and Linux, an open dialog can not be both a file selector
and a directory selector, so if you set `properties` to
`['openFile', 'openDirectory']` on these platforms, a directory selector will be shown.

## dialog.showSaveDialog([browserWindow], [options], [callback])

* `browserWindow` BrowserWindow
* `options` Object
  * `title` String
  * `defaultPath` String
  * `filters` Array
* `callback` Function

On success, returns the path of the file chosen by the user, otherwise returns
`undefined`.

The `filters` specifies an array of file types that can be displayed, see
`dialog.showOpenDialog` for an example.

If a `callback` is passed, the API call will be asynchronous and the result
will be passed via `callback(filename)`

## dialog.showMessageBox([browserWindow], options, [callback])

* `browserWindow` BrowserWindow
* `options` Object
  * `type` String - Can be `"none"`, `"info"` or `"warning"`
  * `buttons` Array - Array of texts for buttons
  * `title` String - Title of the message box, some platforms will not show it
  * `message` String - Content of the message box
  * `detail` String - Extra information of the message
  * `icon` [NativeImage](native-image.md)
* `callback` Function

Shows a message box, it will block until the message box is closed. It returns
the index of the clicked button.

If a `callback` is passed, the API call will be asynchronous and the result
will be passed via `callback(response)`

## dialog.showErrorBox(title, content)

Runs a modal dialog that shows an error message.

This API can be called safely before the `ready` event of `app` module emits, it
is usually used to report errors in early stage of startup.
