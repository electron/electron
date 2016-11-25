# JumpListItem Object

* `type` String - (optional) One of the following:
  * `task` - A task will launch an app with specific arguments.
  * `separator` - Can be used to separate items in the standard `Tasks`
    category.
  * `file` - A file link will open a file using the app that created the
    Jump List, for this to work the app must be registered as a handler for
    the file type (though it doesn't have to be the default handler).
* `path` String - (optional) Path of the file to open, should only be set if `type` is
  `file`.
* `program` String - (optional) Path of the program to execute, usually you should
  specify `process.execPath` which opens the current program. Should only be
  set if `type` is `task`.
* `args` String - (optional) The command line arguments when `program` is executed. Should
  only be set if `type` is `task`.
* `title` String - (optional) The text to be displayed for the item in the Jump List.
  Should only be set if `type` is `task`.
* `description` String - (optional) Description of the task (displayed in a tooltip).
  Should only be set if `type` is `task`.
* `iconPath` String - (optional) The absolute path to an icon to be displayed in a
  Jump List, which can be an arbitrary resource file that contains an icon
  (e.g. `.ico`, `.exe`, `.dll`). You can usually specify `process.execPath` to
  show the program icon.
* `iconIndex` Number - (optional) The index of the icon in the resource file. If a
  resource file contains multiple icons this value can be used to specify the
  zero-based index of the icon that should be displayed for this task. If a
  resource file contains only one icon, this property should be set to zero.
