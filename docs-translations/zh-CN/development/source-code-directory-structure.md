# 源码目录结构

Electron 的源代码主要依据 Chromium 的拆分约定被拆成了许多部分。

为了更好地理解源代码，您可能需要了解一下
[Chromium 的多进程架构](http://dev.chromium.org/developers/design-documents/multi-process-architecture)。

## 源代码的结构

```
Electron
├──atom - Electron 的源代码
|  ├── app - 系统入口代码
|  ├── browser - 包含了主窗口、UI 和其他所有与主进程有关的东西，它会告诉渲染进程如何管理页面
|  |   ├── lib - 主进程初始化代码中 JavaScript 部分的代码
|  |   ├── ui - 不同平台上 UI 部分的实现
|  |   |   ├── cocoa - Cocoa 部分的源代码
|  |   |   ├── gtk - GTK+ 部分的源代码
|  |   |   └── win - Windows GUI 部分的源代码
|  |   ├── default_app - 在没有指定 app 的情况下 Electron 启动时默认显示的页面
|  |   ├── api - 主进程 API 的实现
|  |   |   └── lib - API 实现中 Javascript 部分的代码
|  |   ├── net - 网络相关的代码
|  |   ├── mac - 与 Mac 有关的 Objective-C 代码
|  |   └── resources - 图标，平台相关的文件等
|  ├── renderer - 运行在渲染进程中的代码
|  |   ├── lib - 渲染进程初始化代码中 JavaScript 部分的代码
|  |   └── api - 渲染进程 API 的实现
|  |       └── lib - API 实现中 Javascript 部分的代码
|  └── common - 同时被主进程和渲染进程用到的代码，包括了一些用来将 node 的事件循环
|      |        整合到 Chromium 的事件循环中时用到的工具函数和代码
|      ├── lib - 同时被主进程和渲染进程使用到的 Javascript 初始化代码
|      └── api - 同时被主进程和渲染进程使用到的 API 的实现以及 Electron 内置模块的基础设施
|          └── lib - API 实现中 Javascript 部分的代码
├── chromium_src - 从 Chromium 项目中拷贝来的代码
├── docs - 英语版本的文档
├── docs-translations - 各种语言版本的文档翻译
├── spec - 自动化测试
├── atom.gyp - Electron 的构建规则
└── common.gypi - 为诸如 `node` 和 `breakpad` 等其他组件准备的编译设置和构建规则
```

## 其他目录的结构

* **script** - 用于诸如构建、打包、测试等开发用途的脚本
* **tools** - 在 gyp 文件中用到的工具脚本，但与 `script` 目录不同，
  该目录中的脚本不应该被用户直接调用
* **vendor** - 第三方依赖项的源代码，为了防止人们将它与 Chromium 源码中的同名目录相混淆，
  在这里我们不使用 `third_party` 作为目录名
* **node_modules** - 在构建中用到的第三方 node 模块
* **out** - `ninja` 的临时输出目录
* **dist** - 由脚本 `script/create-dist.py` 创建的临时发布目录
* **external_binaries** - 下载的不支持通过 `gyp` 构建的预编译第三方框架
