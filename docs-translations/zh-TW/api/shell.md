# shell

`shell` 模組提供一些整合桌面應用的功能。


一個範例示範如何利用使用者的預設瀏覽器開啟 URL:

```javascript
var shell = require('shell')

shell.openExternal('https://github.com')
```

## Methods

`shell` 模組有以下幾個方法:

### `shell.showItemInFolder(fullPath)`

* `fullPath` String

顯示在檔案管理中指定的檔案，如果可以的話，選擇該檔案。

### `shell.openItem(fullPath)`

* `fullPath` String

打開指定的檔案，在桌面的預設方式。
Open the given file in the desktop's default manner.

### `shell.openExternal(url)`

* `url` String

打開一個指定的外部協定 URL 在桌面的預設方式。（舉例來說 mailto: URLs 會打開使用者的預設信箱）


### `shell.moveItemToTrash(fullPath)`

* `fullPath` String

移動指定檔案至垃圾桶，並會對這個操作回傳一個 boolean 狀態

### `shell.beep()`

播放 beep 聲音。
