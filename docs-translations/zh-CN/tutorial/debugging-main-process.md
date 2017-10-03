# 主进程调试

浏览器窗口的开发工具仅能调试渲染器的进程脚本（比如 web 页面）。为了提供一个可以调试主进程
的方法，Electron 提供了 `--debug` 和 `--debug-brk` 开关。

## 命令行开关

使用如下的命令行开关来调试 Electron 的主进程：

### `--debug=[port]`

当这个开关用于 Electron 时，它将会监听 V8 引擎中有关 `port` 的调试器协议信息。
默认的 `port` 是 `5858`。

### `--debug-brk=[port]`

就像 `--debug` 一样，但是会在第一行暂停脚本运行。

## 外部调试器

你将需要使用一个支持 V8 调试器的调试协议，
下面的指南将会帮助你开始：

- [使用 VSCode 进行主进程调试](debugging-main-process-vscode.md)
- [使用 node-inspector 进行主进程调试](debugging-main-process-node-inspector.md)
