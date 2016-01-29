# 應用程式打包

為了減少圍繞著 Windows 上長路徑名稱問題的 [issues](https://github.com/joyent/node/issues/6960) ，稍微地加速 `require` 和隱藏你的原始碼避免不小心被看到，你可以選擇把你的應用程式打包成一個 [asar][asar] 壓縮檔，只需要改變一點點你的原始碼就好。
## 產生 `asar` 壓縮檔

一個 [asar][asar] 壓縮檔是一個簡單的類 tar 格式的檔案，它會把幾個檔案串接成一個檔案， Electron 可以不需要解壓縮整個檔案就從中讀取任意檔案。

把你的應用程式打包成 `asar` 壓縮檔的步驟：

### 1. 安裝 asar 工具包

```bash
$ npm install -g asar
```

### 2. 用 `asar pack` 打包

```bash
$ asar pack your-app app.asar
```

## 使用 `asar` 壓縮檔

在 Electron 中有兩組 API：Node.js 提供的 Node APIs 和 Chromium 提供的 Web
APIs，兩組 API 都支援從 `asar` 壓縮檔中讀取檔案。 

### Node API

因為 Electron 中有一些特別的補釘，像是 `fs.readFile` 和 `require` 這樣的 Node API 會將 `asar` 壓縮檔視為許多虛擬目錄，將裡頭的檔案視為在檔案系統上的一般檔案。

例如，假設我們有一個 `example.asar` 壓縮檔在 `/path/to` 中:

```bash
$ asar list /path/to/example.asar
/app.js
/file.txt
/dir/module.js
/static/index.html
/static/main.css
/static/jquery.min.js
```

讀取一個在 `asar` 壓縮檔中的檔案：

```javascript
const fs = require('fs');
fs.readFileSync('/path/to/example.asar/file.txt');
```

列出所有在壓縮檔根目錄下的檔案：

```javascript
const fs = require('fs');
fs.readdirSync('/path/to/example.asar');
```

使用一個壓縮檔中的模組：

```javascript
require('/path/to/example.asar/dir/module.js');
```

你也可以利用 `BrowserWindow` 在 `asar` 壓縮檔中呈現一個網頁：

```javascript
const BrowserWindow = require('electron').BrowserWindow;
var win = new BrowserWindow({width: 800, height: 600});
win.loadURL('file:///path/to/example.asar/static/index.html');
```

### Web API

在一個網頁中，壓縮檔中的檔案都可以透過 `file:` 這個協定被存取，如同 Node API，`asar` 壓縮檔都被視為目錄。

例如，要透過 `$.get` 取得一個檔案：

```html
<script>
var $ = require('./jquery.min.js');
$.get('file:///path/to/example.asar/file.txt', function(data) {
  console.log(data);
});
</script>
```

### 把一個 `asar` 壓縮檔視為一般檔案

在一些像是驗證 `asar` 壓縮檔檢查碼(checksum)的例子中，我們需要以檔案的方式讀取 `asar` 壓縮檔中的內容，為了達到這個目的，你可以使用內建的
`original-fs` 模組，它提供了沒有 `asar` 支援的原生 `fs` API:

```javascript
var originalFs = require('original-fs');
originalFs.readFileSync('/path/to/example.asar');
```

你也可以設定 `process.noAsar` 為 `true` 來關掉在 `fs` 模組中的 `asar` 支援：

```javascript
process.noAsar = true;
fs.readFileSync('/path/to/example.asar');
```

## Node API 上的限制

儘管我們盡可能的努力嘗試著使 Node API 中的 `asar` 壓縮檔像目錄一樣運作，還是有一些基於 Node API 低階本質的限制存在。 

### 壓縮檔都是唯讀的

所有壓縮檔都無法被修改，因此所有可以修改檔案的 Node API 都無法與 `asar ` 壓縮檔一起運作。

### 使用中的目錄無法被設為壓縮檔中的目錄

儘管 `asar` 壓縮檔被視為目錄，卻並沒有真正的目錄在檔案系統中，所以你永遠無法將使用中的目錄設定成 `asar` 壓縮檔中的目錄，把他們以 `cwd` 選項的方式傳遞，對某些 API 也會造成錯誤。

### 更多透過 API 拆封的方法

大部分 `fs` API 可以讀取一個檔案，或是不用拆封就從 `asar` 壓縮檔中取得一個檔案的資訊，但對一些需要傳遞真實檔案路徑給現行系統呼叫的 API ，Electron 將會解開需要的檔案到一個暫時的檔案，然後傳遞該暫時檔案的路徑給那些 API 以便使他們可以運作，這會增加這些 API 一點負擔。


需要額外拆封的 API ：

* `child_process.execFile`
* `child_process.execFileSync`
* `fs.open`
* `fs.openSync`
* `process.dlopen` - 在原生模組中被 `require` 使用

### `fs.stat` 的不真實的狀態資訊

`fs.stat` 回傳的 `Stats` 物件和它在 `asar` 壓縮檔中的檔案朋友都是以猜測的方式產生的，因為那些檔案不存在檔案系統，所以你不應該信任 `Stats` 物件，除了取得檔案大小和確認檔案型態之外。

### 執行 `asar` 壓縮檔中的二進位檔

有像是 `child_process.exec`、`child_process.spawn` 和 `child_process.execFile` 的 Node APIs 可以執行二進位檔，但只有 `execFile` 是 `asar` 壓縮檔中可以執行二進位檔的。

這是因為 `exec` 和 `spawn` 接受的輸入是 `command` 而不是 `file`，而 `command` 們都是在 shell 底下執行，我們找不到可靠的方法來決定是否一個命令使用一個在 `asar` 壓縮檔中的檔案，而儘管我們找得到，我們也無法確定是否我們可以在沒有外部影響(side effect)的情況下替換掉命令中的路徑。

## 加入拆封檔案到 `asar` 壓縮檔中

如前述，一些 Node API 再被呼叫時會拆封檔案到檔案系統，除了效能議題外，它也會導致掃毒軟體發出 false alerts。

要繞過這個問題，你可以透過使用 `--unpack` 選向來拆封一些建立壓縮檔的檔案，以下是一個不包含共享原生模組的函式庫的例子：

```bash
$ asar pack app app.asar --unpack *.node
```

執行這個命令以後，除了 `app.asar` 以外，還有一個帶有拆封檔案的 `app.asar.unpacked` 資料夾被產生出來，當你要發布給使用者時，你應該把它和 `app.asar` 一起複。

[asar]: https://github.com/atom/asar
