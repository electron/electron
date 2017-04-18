# shell
> 使用系统默认应用管理文件和 URL .

进程: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

`shell` 模块提供了集成其他桌面客户端的关联功能.


在用户默认浏览器中打开URL的示例:

```javascript
const {shell} = require('electron')

shell.openExternal('https://github.com')
```

## 方法

`shell` 模块包含以下函数:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

Returns `Boolean` - 
是否成功打开文件所在文件夹,一般情况下还会选中它.

### `shell.openItem(fullPath)`

* `fullPath` String

Returns `Boolean` - 是否成功的以默认打开方式打开文件.


### `shell.openExternal(url)`

* `url` String
* `options` Object (可选) _macOS_
  * `activate` Boolean - `true` 让打开的应用在前面显示，默认为 `true`.
* `callback` Function (可选) - 如果指定将执行异步打开. _macOS_
  * `error` Error

Returns `Boolean` - 应用程序是否打开URL.如果指定了 callback 回调方法, 则返回 true.

以系统默认设置打开外部协议.(例如,mailto: URLs 会打开用户默认的邮件客户端)


### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

Returns `Boolean` - 文件是否成功移动到垃圾桶

删除指定路径文件,并返回此操作的状态值(boolean类型).

### `shell.beep()`

播放 beep 声音.

### `shell.writeShortcutLink(shortcutPath[, operation], options)` _Windows_

* `shortcutPath` String
* `operation` String (可选) - 默认为 `create`, 可以为下列的值:
  * `create` - 创建一个新的快捷方式，如果存在的话会覆盖.
  * `update` - 仅在现有快捷方式上更新指定属性.
  * `replace` - 覆盖现有的快捷方式，如果快捷方式不存在则会失败.
* `options` [ShortcutDetails](structures/shortcut-details.md)

Returns `Boolean` - 快捷方式是否成功创建

为 `shortcutPath` 创建或更新快捷链接.

### `shell.readShortcutLink(shortcutPath)` _Windows_

* `shortcutPath` String

Returns [`ShortcutDetails`](structures/shortcut-details.md)

读取 `shortcutPath` 的快捷连接的信息.

发生错误时，会抛出异常信息.
