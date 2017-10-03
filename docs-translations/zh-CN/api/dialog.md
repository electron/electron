# dialog

> 显示用于打开和保存文件，alert框等的原生的系统对话框

进程: [Main](../glossary.md#main-process)

对话框例子，展示了选择文件和目录:

```javascript
const {dialog} = require('electron')
console.log(dialog.showOpenDialog({properties: ['openFile', 'openDirectory', 'multiSelections']}))
```

对话框从 Electron 的主线程打开。如果要从渲染器进程使用对话框
对象，记得使用 remote 访问它：

```javascript
const {dialog} = require('electron').remote
console.log(dialog)
```

## 方法

`dialog` 模块有以下方法：

### `dialog.showOpenDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (可选)
* `options` Object
  * `title` String (可选)
  * `defaultPath` String (可选)
  * `buttonLabel` String (可选) - Custom label for the confirmation button, when
    left empty the default label will be used.
  * `filters` [FileFilter[]](structures/file-filter.md) (optional)
  * `properties` String[] (可选) - Contains which features the dialog should
    use. The following values are supported:
    * `openFile` - Allow files to be selected.
    * `openDirectory` - Allow directories to be selected.
    * `multiSelections` - Allow multiple paths to be selected.
    * `showHiddenFiles` - Show hidden files in dialog.
    * `createDirectory` _macOS_ - Allow creating new directories from dialog.
    * `promptToCreate` _Windows_ - Prompt for creation if the file path entered
      in the dialog does not exist. This does not actually create the file at
      the path but allows non-existent paths to be returned that should be
      created by the application.
  * `normalizeAccessKeys` Boolean (可选) - Normalize the keyboard access keys
    across platforms. Default is `false`. Enabling this assumes `&` is used in
    the button labels for the placement of the keyboard shortcut access key
    and labels will be converted so they work correctly on each platform, `&`
    characters are removed on macOS, converted to `_` on Linux, and left
    untouched on Windows. For example, a button label of `Vie&w` will be
    converted to `Vie_w` on Linux and `View` on macOS and can be selected
    via `Alt-W` on Windows and Linux.
* `callback` Function (可选)
  * `filePaths` String[] - An array of file paths chosen by the user

成功使用这个方法的话，就返回一个可供用户选择的文件路径数组，失败返回 `undefined`。

`browserWindow` 参数允许对话框将自身附加到父窗口，使其成为模态。

`filters` 当需要限定用户的行为的时候，指定一个文件数组给用户展示或选择。例如：

```javascript
{
  filters: [
    {name: 'Images', extensions: ['jpg', 'png', 'gif']},
    {name: 'Movies', extensions: ['mkv', 'avi', 'mp4']},
    {name: 'Custom File Type', extensions: ['as']},
    {name: 'All Files', extensions: ['*']}
  ]
}
```

`extensions` 数组应当只包含扩展名，不应该包含通配符或 '.' 号 （例如
`'png'` 正确，但是 `'.png'` 和 `'*.png'` 不正确）。展示全部文件的话，使用
`'*'` 通配符 （不支持其他通配符）。

如果 `callback` 被调用，将异步调用 API ，并且结果将用过  `callback(filenames)` 展示。

**注意:** 在 Windows 和 Linux ，一个打开的 dialog 不能既是文件选择框又是目录选择框, 所以如果在这些平台上设置 `properties` 的值为
`['openFile', 'openDirectory']` ，将展示一个目录选择框。

### `dialog.showSaveDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (可选)
* `options` Object
  * `title` String (可选)
  * `defaultPath` String (可选)
  * `buttonLabel` String (可选) - Custom label for the confirmation button, when
    left empty the default label will be used.
  * `filters` [FileFilter[]](structures/file-filter.md) (optional)
* `callback` Function (可选)
  * `filename` String

成功使用这个方法的话，就返回一个可供用户选择的文件路径数组，失败返回 `undefined`。

`browserWindow` 参数允许对话框将自身附加到父窗口，使其成为模态。

`filters` 指定展示一个文件类型数组, 例子
`dialog.showOpenDialog` 。

如果 `callback` 被调用, 将异步调用 API ，并且结果将用过  `callback(filenames)` 展示。

### `dialog.showMessageBox([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (可选)
* `options` Object
  * `type` String - 可以是 `"none"`, `"info"`, `"error"`, `"question"` 或
  `"warning"`. 在 Windows, "question" 与 "info" 展示图标相同, 除非你使用 "icon" 参数.
  * `buttons` String[]- (可选)  - 按钮上文字的数组，在 Windows 系统中，空数组在按钮上会显示 “OK”.
  * `defaultId` Integer (可选) - 在 message box 对话框打开的时候，设置默认选中的按钮，值为在 buttons 数组中的索引.
  * `title` String (可选) - message box 的标题，一些平台不显示.
  * `message` String (可选) - message box 的内容.
  * `detail` String (可选)- 额外信息.
  * `checkboxLabel` String (可选) - 如果有该参数，message box 中会显示一个 checkbox 复选框，它的勾选状态可以在 `callback` 回调方法中获取。
  * `checkboxChecked` Boolean (可选) - checkbox 的初始值，默认为`false`.
  * `icon` [NativeImage](native-image.md)(可选)
  * `cancelId` Integer - 当用户不是通过按钮而是使用其他方式关闭对话框时，比如按`Esc`键，就返回该值.默认值为对应 "cancel" 或 "no" 标签 button 的索引值, 如果没有这种 button，就返回0. 该选项在 Windows 上无效.
  * `noLink` Boolean(可选) - 在 Windows 系统中，Electron 将尝试识别哪个button 是普通 button (如 "Cancel" 或 "Yes"), 然后在对话框中以链接命令(command links)方式展现其它的 button . 这能让对话框展示得很炫酷.如果你不喜欢这种效果，你可以设置 `noLink` 为 `true`.
* `callback` Function (可选)
    * `response` Number - 被点击按钮的索引值。
    * `checkboxChecked` Boolean - 如果设置了 `checkboxLabel` ,会显示 checkbox 的选中状态，否则显示 `false`

返回 `Integer`，如果提供了回调，它会返回点击的按钮的索引或者 undefined 。

显示 message box 时, 它会阻塞进程，直到 message box 关闭为止.返回点击按钮的索引值。

`browserWindow` 参数允许对话框将自身附加到父窗口，使其成为模态。

如果 `callback` 被调用, 将异步调用 API ，并且结果将用过  `callback(response)` 展示。

### `dialog.showErrorBox(title, content)`

* `title` String - 错误框中的标题
* `content` String - 错误框中的内容

展示一个传统的包含错误信息的对话框.

在 `app` 模块触发 `ready` 事件之前，这个 api 可以被安全调用，通常它被用来在启动的早期阶段报告错误.  在 Linux 上，如果在 `app` 模块触发 `ready` 事件之前调用，message 将会被触发显示 stderr ，并且没有实际 GUI 框显示.

## Sheets

在 macOS 上，如果你想像 sheets 一样展示对话框，只需要在`browserWindow` 参数中提供一个 `BrowserWindow` 的引用对象.，如果没有则为模态窗口。

你可以调用 `BrowserWindow.getCurrentWindow().setSheetOffset(offset)` 来改变
sheets 的窗口框架的偏移量。
