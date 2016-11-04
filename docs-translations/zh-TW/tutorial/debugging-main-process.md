# 主行程 Debug

瀏覽器視窗開發工具 DevTools 只可以用來幫渲染器的行程腳本(renderer process script，網頁畫面)除錯(debug)，為了提供一個方法在主行程中除錯，Electron 有提供 `--debug` 和 `--debug-brk` 開關。

## 命令列開關

使用以下命令列切換來為 Electron 的主行程除錯：

### `--debug=[port]`

當這個開關被使用，Electron 將會透過 `port` 來監聽 V8 的除錯協定訊息，預設的 `port` 是 `5858`。

### `--debug-brk=[port]`

同 `--debug` 但暫停在腳本的第一行。

## 使用 node-inspector 來除錯

__Note:__ Electron 近期沒有跟 node-inspector 處的很好，而且如果你透過 node-inspector's console 查看 `process` 物件，主行程將會爆掉。

### 1. 確保你有安裝 [node-gyp required tools][node-gyp-required-tools]

### 2. 安裝 [node-inspector][node-inspector]

```bash
$ npm install node-inspector
```

### 3. 安裝一個修補過版本的 `node-pre-gyp`

```bash
$ npm install git+https://git@github.com/enlight/node-pre-gyp.git#detect-electron-runtime-in-find
```

### 4. 除心編譯 `node-inspector` `v8` 模組給 Electron (變更 target 為你的 Electron 編號)

```bash
$ node_modules/.bin/node-pre-gyp --target=0.36.2 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/electron reinstall
$ node_modules/.bin/node-pre-gyp --target=0.36.2 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/electron reinstall
```

參閱 [如何安裝原生模組](how-to-install-native-modules).

### 5. 給 Electron 啟用除錯模式

你可以啟動 Electron 並帶有一個除錯 flag ，例如:

```bash
$ electron --debug=5858 your/app
```

或者，讓你的腳本暫停在第一行：

```bash
$ electron --debug-brk=5858 your/app
```

### 6. 使用啟動 [node-inspector][node-inspector] 伺服器

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. 載入除錯介面

在你的 Chrome 瀏覽器打開 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858，你可能需要點擊暫停假如你是透過使用 debug-brk 啟動並停在進入行(entry line)的話。

[node-inspector]: https://github.com/node-inspector/node-inspector
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#how-to-install-native-modules

