# shell

`shell` 模块提供了集成其他桌面客户端的关联功能.


在用户默认浏览器中打开URL的示例:

```javascript
const {shell} = require('electron')

shell.openExternal('https://github.com')
```

## Methods

`shell` 模块包含以下函数:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

打开文件所在文件夹,一般情况下还会选中它.

### `shell.openItem(fullPath)`

* `fullPath` String

以默认打开方式打开文件.

### `shell.openExternal(url)`

* `url` String

以系统默认设置打开外部协议.(例如,mailto: somebody@somewhere.io会打开用户默认的邮件客户端)


### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

删除指定路径文件,并返回此操作的状态值(boolean类型).

### `shell.beep()`

播放 beep 声音.
