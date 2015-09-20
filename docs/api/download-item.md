# DownloadItem

`DownloadItem` is an EventEmitter represents a download item in Electron. It
is used in `will-download` event of `Session` module, and allows users to
control the download item.

```javascript
// Disable showing the download saving dialog.
win.webContents.session.setOpenDownloadDialog(false);

win.webContents.session.on('will-download', function(event, item, webContents) {
  console.log("Download from " + item.getURL());
  console.log(item.getMimeType());
  console.log(item.getSuggestedFilename());
  console.log(item.getTotalBytes());
  item.on('updated', function() {
    console.log('Recived bytes: ' + item.getReceiveBytes());
  });
  item.on('completed', function() {
    if (item.getReceiveBytes() >= item.getTotalBytes()) {
      console.log("Download successfully");
    } else {
      console.log("Download is cancelled or interrupted that can't be resumed");
    }
  });
```


## Events

### Event: 'updated'

Emits when the `downloadItem` gets updated.

### Event: 'completed'

Emits when the download is in a terminal state. This includes a completed
download, a cancelled download(via `downloadItem.cancel()`), and interrputed
download that can't be resumed.

## Methods

The `downloadItem` object has the following methods:

### `downloadItem.pause()`

Pauses the download.

### `downloadItem.resume()`

Resumes the download that has been paused.

### `downloadItem.cancel()`

Cancels the download operation.

### `downloadItem.getURL()`

Returns a `String` represents the origin url where the item is downloaded from.

### `downloadItem.getMimeType()`

Returns a `String` represents the mime type.

### `downloadItem.hasUserGesture()`

Returns a `Boolean` indicates whether the download has user gesture.

### `downloadItem.getSuggestedFilename()`

Returns a `String` represents the suggested file name of the download file.

**Note:** The suggested file name is not always the same as the actual one saved
in local disk. If user changes the file name in a prompted download saving
dialog, the actual name of saved file will be different with the suggested one.

### `downloadItem.getTotalBytes()`

Returns a `Integer` represents the total size in bytes of the download item.

### `downloadItem.getReceivedBytes()`

Returns a `Integer` represents the received bytes of the download item.
