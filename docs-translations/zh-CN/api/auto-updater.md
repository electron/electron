# autoUpdater

> 使应用程序能够自动更新。

进程: [Main](../glossary.md#main-process)

`autoUpdater` 模块提供了来自
[Squirrel](https://github.com/Squirrel) 框架的一个接口。

你可以快速启动多平台发布服务器来分发您的
应用程序通过使用下列项目中的一个：

- [nuts][nuts]: *针对你的应用程序的智能发布服务器，使用 GitHub 作为后端。 使用 Squirrel 自动更新（Mac 和 Windows）*
- [electron-release-server][electron-release-server]: *一个功能齐全，
  electron 应用的自托管发布服务器，兼容 auto-updater*
- [squirrel-updates-server][squirrel-updates-server]: *对于使用 GitHub 版本的 Squirrel.Mac 和 Squirrel.Windows 的一个简单的 node.js 服务器*
- [squirrel-release-server][squirrel-release-server]: *一个简单的 Squirrel.Windows 的 PHP 应用程序，它从文件夹读取更新，并支持增量更新*

## 平台相关的提示

虽然 `autoUpdater` 模块提供了一套各平台通用的接口，但是在每个平台间依然会有一些微小的差异。

### macOS

在 macOS 上，`autoUpdater` 模块依靠的是内置的 [Squirrel.Mac][squirrel-mac]，这意味着你不需要依靠其他的设置就能使用。关于
更新服务器的配置，你可以通过阅读 [Server Support][server-support] 这篇文章来了解。注意 [App
Transport Security](https://developer.apple.com/library/content/documentation/General/Reference/InfoPlistKeyReference/Articles/CocoaKeys.html#//apple_ref/doc/uid/TP40009251-SW35) (ATS) 适用于所有请求作为的一部分
更新过程。需要禁用 ATS 的应用程序可以在应用程序的 plist 添加
`NSAllowsArbitraryLoads` 属性。

**注意：** 你的应用程序必须签署 macOS 自动更新。
这是 `Squirrel.Mac` 的要求。

### Windows

在 Windows 上，你必须使用安装程序将你的应用装到用户的计算机上，所以比较推荐的方法是用 [electron-winstaller][installer-lib], [electron-builder][electron-builder-lib] 或者 [grunt-electron-installer][installer] 模块来自动生成一个 Windows 安装向导。

当使用 [electron-winstaller][installer-lib] 或者 [electron-builder][electron-builder-lib]

当使用 [electron-winstaller][installer-lib] 或者 [electron-builder][electron-builder-lib] 确保你不尝试更新你的应用程序 [the first time it runs](https://github.com/electron/windows-installer#handling-squirrel-events)（另见 [this issue for more info](https://github.com/electron/electron/issues/7155)）。 建议使用 [electron-squirrel-startup](https://github.com/mongodb-js/electron-squirrel-startup) 获取应用程序的桌面快捷方式。

Squirrel 自动生成的安装向导会生成一个带 [Application User Model ID][app-user-model-id] 的快捷方式。
Application User Model ID 的格式是 `com.squirrel.PACKAGE_ID.YOUR_EXE_WITHOUT_DOT_EXE`, 比如
像 `com.squirrel.slack.Slack` 和 `com.squirrel.code.Code` 这样的。你应该在自己的应用中使用 `app.setAppUserModelId` 方法设置相同的 API，不然 Windows 将不能正确地把你的应用固定在任务栏上。

服务器端的配置和 macOS 也是不一样的，你可以阅读 [Squirrel.Windows][squirrel-windows] 这个文档来获得详细信息。

### Linux

Linux 下没有任何的自动更新支持，所以我们推荐用各个 Linux 发行版的包管理器来分发你的应用。

## 事件列表

`autoUpdater` 对象会触发以下的事件：

### 事件：'error'

返回：

* `error` Error

当更新发生错误的时候触发。

### 事件：'checking-for-update'

当开始检查更新的时候触发。

### 事件：'update-available'

当发现一个可用更新的时候触发，更新包下载会自动开始。

### 事件：'update-not-available'

当没有可用更新的时候触发。

### 事件：'update-downloaded'

返回：

* `event` Event
* `releaseNotes` String - 新版本更新公告
* `releaseName` String - 新的版本号
* `releaseDate` Date - 新版本发布的日期
* `updateURL` String - 更新地址

在更新下载完成的时候触发。

在 Windows 上只有 `releaseName` 是有效的。

## 方法列表

`autoUpdater` 对象有以下的方法：

### `autoUpdater.setFeedURL(url[, requestHeaders])`

* `url` String
* `requestHeaders` Object _macOS_ (optional) - HTTP请求头。

设置检查更新的 `url`，并且初始化自动更新。

### `autoUpdater.getFeedURL()`

返回 `String` - 当前更新提要 URL。

### `autoUpdater.checkForUpdates()`

向服务端查询现在是否有可用的更新。在调用这个方法之前，必须要先调用 `setFeedURL`。

### `autoUpdater.quitAndInstall()`

在下载完成后，重启当前的应用并且安装更新。这个方法应该仅在 `update-downloaded` 事件触发后被调用。

**注意：** `autoUpdater.quitAndInstall()` 将先关闭所有应用程序窗口
并且只在 `app` 上发出 `before-quit` 事件。这不同于
从正常退出的事件序列。

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[squirrel-windows]: https://github.com/Squirrel/Squirrel.Windows
[installer]: https://github.com/electron/grunt-electron-installer
[installer-lib]: https://github.com/electron/windows-installer
[electron-builder-lib]: https://github.com/electron-userland/electron-builder
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[electron-release-server]: https://github.com/ArekSredzki/electron-release-server
[squirrel-updates-server]: https://github.com/Aluxian/squirrel-updates-server
[nuts]: https://github.com/GitbookIO/nuts
[squirrel-release-server]: https://github.com/Arcath/squirrel-release-server
