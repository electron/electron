# dialog

`dialog` 模块提供了api来展示原生的系统对话框，例如打开文件框，alert框，所以web应用可以给用户带来跟系统应用相同的体验.

对话框例子，展示了选择文件和目录:

```javascript
var win = ...;  // BrowserWindow in which to show the dialog
const dialog = require('electron').dialog;
console.log(dialog.showOpenDialog({ properties: [ 'openFile', 'openDirectory', 'multiSelections' ]}));
```

**macOS 上的注意事项**: 如果你想像sheets一样展示对话框，只需要在`browserWindow` 参数中提供一个 `BrowserWindow` 的引用对象.

## 方法

`dialog` 模块有以下方法:

### `dialog.showOpenDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (可选)
* `options` Object
  * `title` String
  * `defaultPath` String
  * `filters` Array
  * `properties` Array - 包含了对话框的特性值, 可以包含 `openFile`, `openDirectory`, `multiSelections` and
    `createDirectory`
* `callback` Function (可选)

成功使用这个方法的话，就返回一个可供用户选择的文件路径数组，失败返回 `undefined`.

`filters` 当需要限定用户的行为的时候，指定一个文件数组给用户展示或选择. 例如:

```javascript
{
  filters: [
    { name: 'Images', extensions: ['jpg', 'png', 'gif'] },
    { name: 'Movies', extensions: ['mkv', 'avi', 'mp4'] },
    { name: 'Custom File Type', extensions: ['as'] },
    { name: 'All Files', extensions: ['*'] }
  ]
}
```

`extensions` 数组应当只包含扩展名，不应该包含通配符或'.'号 (例如
`'png'` 正确，但是 `'.png'` 和 `'*.png'` 不正确). 展示全部文件的话, 使用
`'*'` 通配符 (不支持其他通配符).

如果 `callback` 被调用, 将异步调用 API ，并且结果将用过  `callback(filenames)` 展示.

**注意:** 在 Windows 和 Linux ，一个打开的 dialog 不能既是文件选择框又是目录选择框, 所以如果在这些平台上设置 `properties` 的值为
`['openFile', 'openDirectory']` , 将展示一个目录选择框.

### `dialog.showSaveDialog([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (可选)
* `options` Object
  * `title` String
  * `defaultPath` String
  * `filters` Array
* `callback` Function (可选)

成功使用这个方法的话，就返回一个可供用户选择的文件路径数组，失败返回 `undefined`.

`filters` 指定展示一个文件类型数组, 例子
`dialog.showOpenDialog` .

如果 `callback` 被调用, 将异步调用 API ，并且结果将用过  `callback(filenames)` 展示.

### `dialog.showMessageBox([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (可选)
* `options` Object
  * `type` String - 可以是 `"none"`, `"info"`, `"error"`, `"question"` 或
  `"warning"`. 在 Windows, "question" 与 "info" 展示图标相同, 除非你使用 "icon" 参数.
  * `buttons` Array - buttons 内容，数组.
  * `defaultId` Integer - 在message box 对话框打开的时候，设置默认button选中，值为在 buttons 数组中的button索引.
  * `title` String - message box 的标题，一些平台不显示.
  * `message` String - message box 内容.
  * `detail` String - 额外信息.
  * `icon` [NativeImage](native-image.md)
  * `cancelId` Integer - 当用户关闭对话框的时候，不是通过点击对话框的button，就返回值.默认值为对应 "cancel" 或 "no" 标签button 的索引值, 或者如果没有这种button，就返回0. 在 macOS 和  Windows 上， "Cancel" button 的索引值将一直是 `cancelId`, 不管之前是不是特别指出的.
  * `noLink` Boolean - 在 Windows ，Electron 将尝试识别哪个button 是普通 button (如 "Cancel" 或 "Yes"), 然后再对话框中以链接命令(command links)方式展现其它的 button . 这能让对话框展示得很炫酷.如果你不喜欢这种效果，你可以设置 `noLink` 为 `true`.
* `callback` Function

展示 message box, 它会阻塞进程，直到 message box 关闭为止.返回点击按钮的索引值.

如果 `callback` 被调用, 将异步调用 API ，并且结果将用过  `callback(response)` 展示.

### `dialog.showErrorBox(title, content)`

展示一个传统的包含错误信息的对话框.

在 `app` 模块触发 `ready` 事件之前，这个 api 可以被安全调用，通常它被用来在启动的早期阶段报告错误.  在 Linux 上，如果在 `app` 模块触发 `ready` 事件之前调用，message 将会被触发显示stderr，并且没有实际GUI 框显示.