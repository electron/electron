# 进程

Electron 中的 `process` 对象 与 upstream node 中的有以下的不同点:

* `process.type` String - 进程类型, 可以是 `browser` (i.e. main process)
  或 `renderer`.
* `process.versions.electron` String - Electron的版本.
* `process.versions.chrome` String - Chromium的版本.
* `process.resourcesPath` String - JavaScript源代码路径.
* `process.mas` Boolean - 在Mac App Store 创建, 它的值为 `true`, 在其它的地方值为 `undefined`.

## 事件

### 事件: 'loaded'

在Electron已经加载了其内部预置脚本和它准备加载主进程或渲染进程的时候触发.

当node被完全关闭的时候，它可以被预加载脚本使用来添加(原文: removed)与node无关的全局符号来回退到全局范围:

```javascript
// preload.js
var _setImmediate = setImmediate
var _clearImmediate = clearImmediate
process.once('loaded', function () {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})
```

## 属性

### `process.noAsar`

设置它为 `true` 可以使 `asar` 文件在node的内置模块中失效.

## 方法

`process` 对象有如下方法:

### `process.hang()`

使当前进程的主线程挂起.

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

设置文件描述符软限制于 `maxDescriptors` 或硬限制与os, 无论它是否低于当前进程.
