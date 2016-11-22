# DownloadItem

`DownloadItem`(下载项)是一个在Electron中展示下载项的
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)。
它被用于`Session`模块中的`will-download`事件，允许用户去控制下载项。

```javascript
// In the main process.
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.webContents.session.on('will-download', (event, item, webContents) => {
  // Set the save path, making Electron not to prompt a save dialog.
  item.setSavePath('/tmp/save.pdf')

  item.on('updated', (event, state) => {
    if (state === 'interrupted') {
      console.log('Download is interrupted but can be resumed')
    } else if (state === 'progressing') {
      if (item.isPaused()) {
        console.log('Download is paused')
      } else {
        console.log(`Received bytes: ${item.getReceivedBytes()}`)
      }
    }
  })
  item.once('done', (event, state) => {
    if (state === 'completed') {
      console.log('Download successfully')
    } else {
      console.log(`Download failed: ${state}`)
    }
  })
})
```

## 事件

### 事件: 'updated'

当`downloadItem`获得更新时触发。

### 事件: 'done'

* `event` Event
* `state` String
  * `completed` - 下载成功完成。
  * `cancelled` - 下载被取消。
  * `interrupted` - 与文件服务器错误的中断连接。

当下载处于一个终止状态时触发。这包括了一个完成的下载，一个被取消的下载(via `downloadItem.cancel()`),
和一个被意外中断的下载(无法恢复)。

## 方法

`downloadItem`对象有以下一些方法:

### `downloadItem.setSavePath(path)`

* `path` String - 设置下载项的保存文件路径.

这个API仅仅在`session`的`will-download`回调函数中可用。
如果用户没有这个API去设置保存路径，Electron会用原始程序去确定保存路径(通常提示一个保存对话框)。

### `downloadItem.pause()`

暂停下载。

### `downloadItem.resume()`

恢复被暂停的下载。

### `downloadItem.cancel()`

取消下载操作。

### `downloadItem.getURL()`

以`String`形式返回一个该下载项的下载源url。

### `downloadItem.getMimeType()`

返回一个表示mime类型的`String`。

### `downloadItem.hasUserGesture()`

返回一个`Boolean`表示下载是否有用户动作。

### `downloadItem.getFilename()`

返回一个表示下载项文件名的`String`。

**Note:** 此文件名不一定总是保存在本地硬盘上的实际文件名。
如果用户在下载保存对话框中修改了文件名，保存的文件的实际名称会与`downloadItem.getFilename()`方法返回的文件名不同。

### `downloadItem.getTotalBytes()`

返回一个表示下载项总字节数大小的`Integer`。如果大小未知，返回0。

### `downloadItem.getReceivedBytes()`

返回一个表示下载项已经接收的字节数大小的`Integer`。

### `downloadItem.getContentDisposition()`

以`String`形式返回响应头(response header)中的`Content-Disposition`域。
