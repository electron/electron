# 使用 node-inspector 进行主进程调试

[`node-inspector`][node-inspector] 提供了一个熟悉的 DevTools GUI 即可
在 Chrome 中来调试 Electron 的主进程，但是，因为根据你希望调试的 Electron 版本，
`node-inspector` 依赖于一些必须重新编译的原生 Node 模块
。你可以重新编译 `node-inspector` 依赖自己，或者让
[`electron-inspector`][electron-inspector] 为你做，两种方法都
涵盖在本文档中。

**备注**: 在编写的 `node-inspector` 最新版本
（0.12.8）无法重新编译目标版本为 Electron 1.3.0 或者以上，没有修补
它的一个依赖关系。在使用 `electron-inspector`时，需要注意这些。

## 使用 `electron-inspector` 来调试

### 1. 安装 [node-gyp required tools][node-gyp-required-tools]

### 2. 安装 [`electron-rebuild`][electron-rebuild]，如果你还没有这样做。

```shell
npm install electron-rebuild --save-dev
```

### 3. 安装 [`electron-inspector`][electron-inspector]

```shell
npm install electron-inspector --save-dev
```

### 4. 启动 Electron

用 `--debug` 参数来运行 Electron：

```shell
electron --debug=5858 your/app
```

或者，在第一行暂停你的脚本：

```shell
electron --debug-brk=5858 your/app
```

### 5. 启动 electron-inspector

在 macOS / Linux:

```shell
node_modules/.bin/electron-inspector
```

在 Windows:

```shell
node_modules\\.bin\\electron-inspector
```
`electron-inspector` 在首次运行时，或者更改了 Electron 的版本时
需要重新编译 `node-inspector` 的依赖关系，
重新编译的过程可能需要互联网连接，并花费一些时间才能下载 Node headers 和lib。

### 6. 加载 debugger UI

在 Chrome 浏览器中打开 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 。
如果以 `--debug-brk` 开始强制UI更新，你可能需要点击 pause。

## 使用 `node-inspector` 来调试

### 1. 安装 [node-gyp required tools][node-gyp-required-tools]

### 2. 安装 [`node-inspector`][node-inspector]

```bash
$ npm install node-inspector
```

### 3. 安装 [`node-pre-gyp`][node-pre-gyp]

```bash
$ npm install node-pre-gyp
```

### 4. 为 Electron 重新编译 `node-inspector` `v8` 模块

**注意：** 将 target 参数修改为你的 Electron 的版本号

```bash
$ node_modules/.bin/node-pre-gyp --target=1.2.5 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/atom-shell reinstall
$ node_modules/.bin/node-pre-gyp --target=1.2.5 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/atom-shell reinstall
```

详见 [如何下载原生模块][how-to-install-native-modules]。

### 5. 打开 Electron 的调试模式

你也可以用调试参数来运行 Electron ：

```bash
$ electron --debug=5858 your/app
```

或者，在第一行暂停你的脚本：

```bash
$ electron --debug-brk=5858 your/app
```

### 6. 使用 Electron 开启 [`node-inspector`][node-inspector] 服务

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. 加载调试器界面

在 Chrome 浏览器中打开 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 。
如果以 `--debug-brk` 开始看到输入行，你可能需要点击 pause。

[electron-inspector]: https://github.com/enlight/electron-inspector
[electron-rebuild]: https://github.com/electron/electron-rebuild
[node-inspector]: https://github.com/node-inspector/node-inspector
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#how-to-install-native-modules
