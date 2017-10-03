# 术语表

这篇文档说明了一些经常在 Electron 开发中使用的专业术语。

### ASAR

ASAR 代表了 Atom Shell Archive Format。一个 [asar][asar] 压缩包就是一个简单的 `tar` 文件-就像将那些有联系的文件格式化至一个单独的文件中。Electron 能够任意读取其中的文件并且不需要解压缩整个文件。

ASAR 格式主要是为了提升 Windows 平台上的性能。TODO

### Brightray

[Brightray][brightray] 是能够简单的将 [libchromiumcontent] 应用到应用中的一个静态库。它是专门开发给 Electron 使用，但是也能够使用在那些没有基于 Electron 的原生应用来启用 Chromium 的渲染引擎。

Brightray 是 Electron 中的一个低级别的依赖，大部分的 Electron 用户不用关心它。

### DMG

是指在 macOS 上使用的苹果系统的磁盘镜像打包格式。DMG 文件通常被用来分发应用的 "installers"（安装包）。[electron-builder] 支持使用 `dmg` 来作为编译目标。 

### IPC

IPC 代表 Inter-Process Communication。Electron 使用 IPC 来在 [主进程] 和 [渲染进程] 之间传递 JSON 信息。

### libchromiumcontent

一个单独的开源库，包含了 Chromium 的模块以及全部依赖（比如 Blink, [V8] 等）。

### main process

主进程，通常是值 `main.js` 文件，是每个 Electron 应用的入口文件。它控制着整个 APP 的生命周期，从打开到关闭。它也管理着原生元素比如菜单，菜单栏，Dock 栏，托盘等。主进程负责创建 APP 的每个渲染进程。而且整个 Node API 都集成在里面。

每个 app 的主进程文件都定义在 `package.json` 中的 `main` 属性当中，这也是为什么 `electron .` 能够知道应该使用哪个文件来启动。

参见： [process](#process), [renderer process](#renderer-process)

### MAS

是指苹果系统上的 Mac App Store 的缩略词。有关于如何提交你的 app 至 MAS ，详见 [Mac App Store Submission Guide] 。

### native modules

原生模块 （在 Node.js 里也叫 [addons]），是一些使用 C or C++ 编写的能够在 Node.js 中加载或者在 Electron 中使用 require() 方法来加载的模块，它使用起来就如同 Node.js 的模块。它主要用于桥接在 JavaScript 上运行 Node.js 和 C/C++ 的库。

Electron 支持了原生的 Node 模块，但是 Electron 非常可能安装一个不一样的 V8 引擎通过 Node 二进制编码，所以在打包原生模块的时候你需要在  指定具体的 Electron 本地头文件。 

参见： [Using Native Node Modules].

## NSIS

Nullsoft Scriptable Install System 是一个微软 Windows 平台上的脚本驱动的安装制作工具。它发布在免费软件许可证书下，是一个被广泛使用的替代商业专利产品类似于 InstallShield。[electron-builder] 支持使用 NSIS 作为编译目标。

### process

一个进程是计算机程序执行中的一个实例。Electron 应用同时使用了 [main]  (主进程) 和一个或者多个 [renderer]  （渲染进程）来运行多个程序。

在 Node.js 和 Electron 里面，每个运行的进程包含一个 `process` 对象。这个对象作为一个全局的提供当前进程的相关信息，操作方法。作为一个全局变量，它在应用内能够不用 require() 来随时取到。

参见： [main process](#main-process), [renderer process](#renderer-process)

### renderer process

渲染进程是你的应用内的一个浏览器窗口。与主进程不同的是，它能够同时存在多个而且运行在不一样的进程。而且它们也能够被隐藏。

在通常的浏览器内，网页通常运行在一个沙盒的环境挡住并且不能够使用原生的资源。然而 Electron 的用户在 Node.js 的 API 支持下可以在页面中和操作系统进行一些低级别的交互。

参见： [process](#process), [main process](#main-process)

### Squirrel

Squirrel 是一个开源的框架来让 Electron 的应用能够自动的更新到发布的新的版本。详见 [autoUpdater] API 了解如何开始使用 Squirrel。

### userland

"userland" 或者 "userspace" 术语起源于 Unix 社区，当程序运行在操作系统内核之外。最近这个术语被推广在 Node 和 npm 社区用于区分 "Node core" 与发布的包的功能，对于在 npm 上注册的广大 "user（用户）" 们。

就像 Node ，Electron 致力于使用一些少量的设置和 API 来提供所有的必须的支持给开发中的跨平台应用。这个设计理念让 Electron 能够保持灵活而不被过多的规定有关于如何应该被使用。Userland 让用户能够创造和分享一些工具来提额外的功能在这个能够使用的 "core（核心）"之上。

### V8

V8 是谷歌公司的开源的 JavaScript 引擎。它使用 C++ 编写并使用在谷歌公司开源的的浏览器 Google Chrome 上。V8 能够单独运行或者集成在任何一个 C++ 应用内。

### webview

`webview` 标签用于集成 'guest（访客）' 内容（比如外部的网页）在你的 Electron 应用内。它们类似于 `iframe`，但是不同的是每个 webview 运行在独立的进程中。 作为页面它拥有不一样的权限并且所有的嵌入的内容和你应用之间的交互都将是异步的。这将保证你的应用对于嵌入的内容的安全性。

[addons]: https://nodejs.org/api/addons.html
[asar]: https://github.com/electron/asar
[autoUpdater]: api/auto-updater.md
[brightray]: https://github.com/electron/brightray
[electron-builder]: https://github.com/electron-userland/electron-builder
[libchromiumcontent]: #libchromiumcontent
[Mac App Store Submission Guide]: tutorials/mac-app-store-submission-guide.md
[main]: #main-process
[renderer]: #renderer-process
[Using Native Node Modules]: tutorial/using-native-node-modules.md
[userland]: #userland
[V8]: #v8
