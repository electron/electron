# 主进程调试

浏览器窗口的开发工具仅能调试渲染器的进程脚本（比如web 页面）。为了提供一个可以调试主进程
的方法，Electron 提供了 `--debug` 和 `--debug-brk` 开关。

## 命令行开关

使用如下的命令行开关来调试 Electron 的主进程：

### `--debug=[port]`

当这个开关用于 Electron 时，它将会监听 V8 引擎中有关 `port` 的调试器协议信息。
默认的 `port` 是 `5858`。

### `--debug-brk=[port]`

就像 `--debug` 一样，但是会在第一行暂停脚本运行。

## 使用 node-inspector 来调试

__备注：__ Electron 使用 node v0.11.13 版本，目前对 node-inspector支持的不是特别好，
如果你通过 node-inspector 的 console 来检查 `process` 对象，主进程就会崩溃。

### 1. 开始 [node-inspector][node-inspector] 服务

```bash
$ node-inspector
```

### 2. 打开 Electron 的调试模式

你也可以用调试参数来运行 Electron ：

```bash
$ electron --debug=5858 your/app
```

或者，在第一行暂停你的脚本：

```bash
$ electron --debug-brk=5858 your/app
```

### 3. 加载调试器界面

在 Chrome 中打开 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858

[node-inspector]: https://github.com/node-inspector/node-inspector
