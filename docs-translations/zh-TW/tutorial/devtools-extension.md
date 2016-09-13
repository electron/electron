# DevTools 擴充套件

要使除錯更簡單，Electron 有對 [Chrome DevTools(開發人員工具) Extension][devtools-extension] 基本的支援。

多數的 DevTools 擴充可以簡單地透過下載原始碼然後使用 `BrowserWindow.addDevToolsExtension` API 來載入它們，已載入的擴充套件會被記住，如此一來你就不用每次建立一個視窗的時候就要呼叫 API。

** 注意: React DevTools 無法使用，參考 [issue](https://github.com/electron/electron/issues/915) **

例如使用 [React DevTools Extension](https://github.com/facebook/react-devtools)，首先你需要下載它的原始碼：

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

照著 [`react-devtools/shells/chrome/Readme.md`](https://github.com/facebook/react-devtools/blob/master/shells/chrome/Readme.md) 的指示來建置擴充套件。

接著你就可以在 Electron 中打開任何視窗來載入擴充套件了，然後在 DevTools console 執行以下的程式碼：

```javascript
const BrowserWindow = require('electron').remote.BrowserWindow;
BrowserWindow.addDevToolsExtension('/some-directory/react-devtools/shells/chrome');
```

要卸載擴充套件，你可以呼叫 `BrowserWindow.removeDevToolsExtension` 
API，並帶入套件的名稱，則套件就不會在下次起打開 DevTools 的時候載入了：

```javascript
BrowserWindow.removeDevToolsExtension('React Developer Tools');
```

## DevTools 擴充套件的格式

理想上，所有寫給 Chrome 瀏覽器用的 DevTools 擴充套件都要能夠被 Electron 載入，但它們需要是在一個普通的目錄下，那些以 `crx` 包裝的擴充套件，Electron 沒有辦法載入它們除非你找到一個解壓縮他們到目錄的方法。

## Background Pages

目前 Electron 沒有支援 Chrome 擴充套件的 background pages，所以一些會用到 background pages 的 DevTools 擴充套件可能會無法在 Electron 中運作。

## `chrome.*` APIs

一些 Chrome 擴充套件可能使用到 `chrome.*` API，而儘管在 Electron 中有一些這種 API 的實作，還是沒有所有的部分都被實作到。

因為並非所有 `chrome.*` API 都有實作，如果一個 DevTools 擴充套件有使用到而非 `chrome.devtools.*`，這個套件非常有可能無法運作，你可以回報失敗的擴充套件到 issue tracker，那我們就可以把這些 API 加入支援。

[devtools-extension]: https://developer.chrome.com/extensions/devtools
