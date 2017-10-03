# 使用原生 node 模組

原生的 Node 模組 Electron 都有支援，但自從 Electron 使用了與 Node 官方不同的 V8 版本後，當你要建置原生模組的時候，你需要手動指定 Electron 標頭的位置。

## 原生 Node 模組的相容性

原生模組可能在 Node 開始使用一個新版本的 V8 時毀損，為了確保你想要用的模組能正確與 Electron 一起運行，你應該檢查是否支援 Electron 內部 Node 版本，你可以查看 [releases](https://github.com/electron/electron/releases) 或是使用 `process.version` (範例請見 [Quick Start](https://github.com/electron/electron/blob/master/docs/tutorial/quick-start.md)) 來檢查哪個 Node 版本是現在的 Electron 使用的。

你可以考慮給你自己的模組使用 [NAN](https://github.com/nodejs/nan/)，因為它可以較輕易的支援多種版本的 Node，它對於移植舊的模組到新版本的 Node 以便與 Electron 一起運作也是很有用的。

## 如何安裝原生模組

三種安裝原生模組的方式：

### 簡單的方法

最直接重新建置原生模組的方法是透過 [`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild) 套件，這個套件處理了手動下載標頭和建制原生模組的步驟：

```sh
npm install --save-dev electron-rebuild

# 每次你執行 "npm install", 執行這句
./node_modules/.bin/electron-rebuild

# 在 Windows 上如果你有狀況，試試看：
.\node_modules\.bin\electron-rebuild.cmd
```

### 使用 npm

你也可以使用 `npm` 安裝模組，步驟與 Node 模組的安裝相同，除了你需要設定一些環境變數：

```bash
export npm_config_disturl=https://atom.io/download/electron
export npm_config_target=0.33.1
export npm_config_arch=x64
export npm_config_runtime=electron
HOME=~/.electron-gyp npm install module-name
```

### 使用 node-gyp 

要把 Node 模組與 Eletron 的標頭一起建置，你需要告訴 `node-gyp` 下載標頭到哪裡，以及要使用哪個版本：

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/electron
```

`HOME=~/.electron-gyp` 改變了尋找開發標頭的地方，`--target=0.29.1` 是 Eletron 的版本， `--dist-url=...` 指定了下載標頭到哪， `--arch=x64` 指出模組要建置在 64 位元系統。
