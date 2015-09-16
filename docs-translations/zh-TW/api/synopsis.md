# Synopsis

所有的 [Node.js's 內建模組](http://nodejs.org/api/) 都可以在 Electron 使用，而且
第三方的 node 模組同樣的全部支援（包含[原生模組](../tutorial/using-native-node-modules.md)）

Electron 也提供一些額外的內建模組用來開發原生桌面應用程式，一些模組只可以使用在主行程上
(main process) 一些只可以使用在渲染行程 (renderer process) 上 (網頁) ，另外還有一些
模組在兩邊的行程都可以使用。

基本的規則是: 如果一個模組是 [GUI](https://zh.wikipedia.org/wiki/%E5%9B%BE%E5%BD%A2%E7%94%A8%E6%88%B7%E7%95%8C%E9%9D%A2)
或者是 low-level 與系統相關的，那麼它就應該只能在主行程上使用 (main process) 你必須要對熟悉 [main process vs. renderer process](../tutorial/quick-start.md#the-main-process) 的觀念，才能去使用這些模組。

主行程 (main process) 腳本是一個像一般 Node.js 的腳本:

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

var window = null;

app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadUrl('https://github.com');
});
```

渲染行程 (renderer process) 跟一般正常的網頁沒有差別，而且還能有使用 node 模組的能力:

```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      var remote = require('remote');
      console.log(remote.require('app').getVersion());
    </script>
  </body>
</html>
```

執行你的應用程式，請閱讀[Run your app](../tutorial/quick-start.md#run-your-app).
