# autoUpdater

这个模块提供了一个到 `Squirrel` 自动更新框架的接口。

## 平台相关的提示

虽然 `autoUpdater` 模块提供了一套各平台通用的接口，但是在每个平台间依然会有一些微小的差异。

### macOS

在 macOS 上，`autoUpdater` 模块依靠的是内置的 [Squirrel.Mac][squirrel-mac]，这意味着你不需要依靠其他的设置就能使用。关于
更新服务器的配置，你可以通过阅读 [Server Support][server-support] 这篇文章来了解。

### Windows

在 Windows 上，你必须使用安装程序将你的应用装到用户的计算机上，所以比较推荐的方法是用 [grunt-electron-installer][installer] 这个模块来自动生成一个 Windows 安装向导。

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

### `autoUpdater.setFeedURL(url)`

* `url` String

设置检查更新的 `url`，并且初始化自动更新。这个 `url` 一旦设置就无法更改。

### `autoUpdater.checkForUpdates()`

向服务端查询现在是否有可用的更新。在调用这个方法之前，必须要先调用 `setFeedURL`。

### `autoUpdater.quitAndInstall()`

在下载完成后，重启当前的应用并且安装更新。这个方法应该仅在 `update-downloaded` 事件触发后被调用。

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[squirrel-windows]: https://github.com/Squirrel/Squirrel.Windows
[installer]: https://github.com/atom/grunt-electron-installer
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
