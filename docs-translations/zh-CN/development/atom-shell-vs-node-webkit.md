# Electron 和 NW.js (原名 node-webkit) 在技术上的差异

__备注：Electron 的原名是 Atom Shell。__

与 NW.js 相似，Electron 提供了一个能通过 JavaScript 和 HTML 创建桌面应用的平台，同时集成 Node 来授予网页访问底层系统的权限。

但是这两个项目也有本质上的区别，使得 Electron 和 NW.js 成为两个相互独立的产品。

__1. 应用的入口__

在 NW.js 中，一个应用的主入口是一个页面。你在 `package.json` 中指定一个主页面，它会作为应用的主窗口被打开。

在 Electron 中，入口是一个 JavaScript 脚本。不同于直接提供一个URL，你需要手动创建一个浏览器窗口，然后通过 API 加载 HTML 文件。你还可以监听窗口事件，决定何时让应用退出。

Electron 的工作方式更像 Node.js 运行时。 Electron 的 APIs 更加底层，因此你可以它替代 [PhantomJS](http://phantomjs.org/) 做浏览器测试。

__2. 构建系统__

为了避免构建整个 Chromium 带来的复杂度，Electron 通过 [`libchromiumcontent`](https://github.com/brightray/libchromiumcontent) 来访问 Chromium 的 Content API。`libchromiumcontent` 是一个独立的、引入了 Chromium Content 模块及其所有依赖的共享库。用户不需要一个强劲的机器来构建 Electron。

__3. Node 集成__

在 NW.js，网页中的 Node 集成需要通过给 Chromium 打补丁来实现。但在 Electron 中，我们选择了另一种方式：通过各个平台的消息循环与 libuv 的循环集成，避免了直接在 Chromium 上做改动。你可以看 [`node_bindings`](../../atom/common/) 来了解这是如何完成的。

__4. 多上下文__

如果你是有经验的 NW.js 用户，你应该会熟悉 Node 上下文和 web 上下文的概念。这些概念的产生源于 NW.js 的实现方式。

通过使用 Node 的[多上下文](http://strongloop.com/strongblog/whats-new-node-js-v0-12-multiple-context-execution/)特性，Electron不需要在网页中引入新的 JavaScript 上下文。
