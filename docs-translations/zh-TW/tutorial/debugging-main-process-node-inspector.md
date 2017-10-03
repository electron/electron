# 在 node-inspector 裡為主程序除錯

[`node-inspector`][node-inspector] 提供一個大家熟悉的圖形介面開發工具，可以在 Chrome 使用這個工具為 Electron 的主程序除錯。然而因為 `node-inspector` 依靠依些原生的 Node 模組，他必須為你想除錯的 Electron 版本重新建制。你可以自己重新建置 `node-inspector` 的相依檔案，或是由
[`electron-inspector`][electron-inspector] 幫你完成，這兩種方法都包含在這篇文件中。

**注意**：在這篇文件撰寫時，最新版的 `node-inspector`
(0.12.8) 不能在沒有更新其中一個相依檔案的情形下，為 Electron 1.3.0 或更新的版本重新建置，你在使用 `electron-inspector` 的時候必須注意這一點。


## 使用 `electron-inspector` 除錯

### 1. 安裝 [node-gyp required tools][node-gyp-required-tools]

### 2. 安裝 [`electron-rebuild`][electron-rebuild]

```shell
npm install electron-rebuild --save-dev
```

### 3. 安裝 [`electron-inspector`][electron-inspector]

```shell
npm install electron-inspector --save-dev
```

### 4. 啟動 Electron

搭配 `--debug` 參數開啟 Electron

```shell
electron --debug=5858 your/app
```

或者在第1行 JavaScript 暫停執行：

```shell
electron --debug-brk=5858 your/app
```

### 5. 開啟 electron-inspector

在 macOS / Linux 上:

```shell
node_modules/.bin/electron-inspector
```

在 Windows 上:

```shell
node_modules\\.bin\\electron-inspector
```

`electron-inspector` 在第一次執行，以及你每次更換 Electron 版本時需要重新建置 `node-inspector` 的相依檔案。重新建置的過程可能需要透過網路連線下載 Node 標頭檔和函式庫，且需要一段時間。

### 6. 載入除錯工具介面

在 Chrome 瀏覽器中打開 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858。如果有使用 `--debug-brk` 選項，你有可能需要按暫停鍵強制更新介面。

## 使用 `node-inspector` 除錯

### 1. 安裝 [node-gyp required tools][node-gyp-required-tools]

### 2. 安裝 [`node-inspector`][node-inspector]

```bash
$ npm install node-inspector
```

### 3. 安裝 [`node-pre-gyp`][node-pre-gyp]

```bash
$ npm install node-pre-gyp
```

### 4. 為 Electron 重新編譯 `node-inspector` `v8` 模組

**注意:** 請把 `target` 參數改成你的 Electron 版本號碼

```bash
$ node_modules/.bin/node-pre-gyp --target=1.2.5 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/atom-shell reinstall
$ node_modules/.bin/node-pre-gyp --target=1.2.5 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/atom-shell reinstall
```

請參考 [How to install native modules][how-to-install-native-modules].

### 5. 啟用 Electron 除錯模式

你可以使用 debug 標籤啟動 Electron：

```bash
$ electron --debug=5858 your/app
```

或是在第1行暫停程式：

```bash
$ electron --debug-brk=5858 your/app
```

### 6. 使用 Electron 開啟 [`node-inspector`][node-inspector] 伺服器

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. 載入除錯工具介面

在 Chrome 瀏覽器打開 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 。如果有使用 `--debug-brk` 啟動 Electron ，你有可能要點擊暫停鍵才能看到進入點。

[electron-inspector]: https://github.com/enlight/electron-inspector
[electron-rebuild]: https://github.com/electron/electron-rebuild
[node-inspector]: https://github.com/node-inspector/node-inspector
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#how-to-install-native-modules
