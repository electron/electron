# 环境变量

一些 Electron 的行为受到环境变量的控制，因为他们的初始化比命令行和应用代码更早.

POSIX shells 的例子:

```bash
$ export ELECTRON_ENABLE_LOGGING=true
$ electron
```

Windows 控制台:

```powershell
> set ELECTRON_ENABLE_LOGGING=true
> electron
```

## `ELECTRON_RUN_AS_NODE`

类似node.js普通进程启动方式.

## `ELECTRON_ENABLE_LOGGING`

打印 Chrome 的内部日志到控制台.

## `ELECTRON_LOG_ASAR_READS`

当 Electron 读取 ASA 文档，把 read offset 和文档路径做日志记录到系统 `tmpdir`.结果文件将提供给 ASAR 模块来优化文档组织.

## `ELECTRON_ENABLE_STACK_DUMPING`

当 Electron 崩溃的时候，打印堆栈记录到控制台.

如果 `crashReporter` 已经启动那么这个环境变量实效.

## `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

当 Electron 崩溃的时候，显示windows的崩溃对话框.

如果 `crashReporter` 已经启动那么这个环境变量实效.

## `ELECTRON_NO_ATTACH_CONSOLE` _Windows_

不可使用当前控制台.

## `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

不可再 Linux 上使用全局菜单栏.

## `ELECTRON_HIDE_INTERNAL_MODULES`

关闭旧的内置模块如 `require('ipc')` 的通用模块.