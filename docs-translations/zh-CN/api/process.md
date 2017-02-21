# 进程

> process 对象扩展。

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

Electron 的 `process` 对象是
[Node.js `process` 对象](https://nodejs.org/api/process.html) 的扩展。
它添加了以下事件、属性和方法：

## 事件

### 事件: 'loaded'

在Electron已经加载了其内部预置脚本和它准备加载网页或者主进程的时候触发。

当node被完全关闭的时候，它可以被预加载脚本使用来添加(原文: removed)与node无关的全局符号来回退到全局范围：

```javascript
// preload.js
const _setImmediate = setImmediate
const _clearImmediate = clearImmediate
process.once('loaded', () => {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})
```

## 属性

### `process.noAsar`

设置它为 `true` 可以使 `asar` 文件在node的内置模块中失效。

### `process.type`

当前 `process` 的类型，值为`"browser"` (即主进程) 或 `"renderer"`。

### `process.versions.electron`

Electron的版本号。

### `process.versions.chrome`

Chrome的版本号。

### `process.resourcesPath`

资源文件夹的路径。

### `process.mas`

在 Mac App Store 的构建中，该属性为 `true`, 其他平台的构建均为 `undefined`。

### `process.windowsStore`

如果 app 是运行在 Windows Store app (appx) 中，该属性为 `true`, 其他情况均为 `undefined`。

### `process.defaultApp`

当 app 在启动时，被作为参数传递给默认应用程序，在主进程中该属性为 `true`, 其他情况均为 `undefined`。

## 方法

`process` 对象有如下方法：

### `process.crash()`

使当前进程的主线程崩溃。

### `process.hang()`

使当前进程的主线程挂起。

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

设置文件描述符软限制于 `maxDescriptors` 或硬限制于OS, 无论它是否低于当前进程。

### `process.getProcessMemoryInfo()`

返回 `Object`：

* `workingSetSize` Integer - 当前固定到实际物理内存的内存量。
* `peakWorkingSetSize` Integer - 被固定在实际物理内存上的最大内存量。
* `privateBytes` Integer - 不被其他进程共享的内存量，如JS堆或HTML内容。
* `sharedBytes` Integer - 进程之间共享的内存量，通常是 Electron 代码本身所消耗的内存。

返回当前进程的内存使用统计信息的对象。请注意，所有数据的单位都是KB。

### `process.getSystemMemoryInfo()`

返回 `Object`：

* `total` Integer - 系统的物理内存总量。
* `free` Integer - 未被应用程序或磁盘缓存使用的物理内存总量。
* `swapTotal` Integer - 系统 swap 分区(虚拟内存)总量。  _Windows_ _Linux_
* `swapFree` Integer - 系统剩余可用的 swap 分区(虚拟内存)量。  _Windows_ _Linux_

返回系统的内存使用统计信息的对象。请注意，所有数据的单位都是KB。
