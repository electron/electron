# app

`app` 模块是为了控制整个应用的生命周期设计的。

下面的这个例子将会展示如何在最后一个窗口被关闭时退出应用：

```javascript
var app = require('app');
app.on('window-all-closed', function() {
  app.quit();
});
```

## 事件

`app` 对象会触发以下的事件：

### 事件： 'will-finish-launching'

当应用程序完成基础的启动的时候被触发。在 Windows 和 Linux 中，
`will-finish-launching` 事件与 `ready` 事件是相同的； 在 OS X 中，
这个时间相当于 `NSApplication` 中的 `applicationWillFinishLaunching` 提示。
你应该经常在这里为 `open-file` 和 `open-url` 设置监听器，并启动崩溃报告和自动更新。

在大多数的情况下，你应该只在 `ready` 事件处理器中完成所有的业务。

### 事件： 'ready'

当 Electron 完成初始化时被触发。

### 事件： 'window-all-closed'

当所有的窗口都被关闭时触发。

这个时间仅在应用还没有退出时才能触发。 如果用户按下了 `Cmd + Q`，
或者开发者调用了 `app.quit()` ，Electron 将会先尝试关闭所有的窗口再触发 `will-quit` 事件，
在这种情况下 `window-all-closed` 不会被触发。

### 事件： 'before-quit'

返回：

* `event` 事件

在应用程序开始关闭它的窗口的时候被触发。
调用 `event.preventDefault()` 将会阻止终止应用程序的默认行为。

### 事件： 'will-quit'

返回：

* `event` 事件

当所有的窗口已经被关闭，应用即将退出时被触发。
调用 `event.preventDefault()` 将会阻止终止应用程序的默认行为。

你可以在 `window-all-closed` 事件的描述中看到 `will-quit` 事件
和 `window-all-closed` 事件的区别。

### 事件： 'quit'

当应用程序正在退出时触发。

### 事件： 'open-file'

返回：

* `event` 事件
* `path` 字符串

当用户想要在应用中打开一个文件时触发。`open-file` 事件常常在应用已经打开并且系统想要再次使用应用打开文件时被触发。
 `open-file` 也会在一个文件被拖入 dock 且应用还没有运行的时候被触发。
请确认在应用启动的时候（甚至在 `ready` 事件被触发前）就对 `open-file` 事件进行监听，以处理这种情况。

如果你想处理这个事件，你应该调用 `event.preventDefault()` 。

### 事件： 'open-url'

返回：

* `event` 事件
* `url` 字符串

当用户想要在应用中打开一个url的时候被触发。URL格式必须要提前标识才能被你的应用打开。

如果你想处理这个事件，你应该调用 `event.preventDefault()` 。

### 事件： 'activate' _OS X_

返回：

* `event` 事件
* `hasVisibleWindows` 布尔值

当应用被激活时触发，常用于点击应用的 dock 图标的时候。

### 事件： 'browser-window-blur'

返回：

* `event` 事件
* `window` 浏览器窗口

当一个 [浏览器窗口](browser-window.md) 失去焦点的时候触发。

### 事件： 'browser-window-focus'

返回：

* `event` 事件
* `window` 浏览器窗口

当一个 [浏览器窗口](browser-window.md) 获得焦点的时候触发。

### 事件： 'browser-window-created'

返回：

* `event` 事件
* `window` 浏览器窗口

当一个 [浏览器窗口](browser-window.md) 被创建的时候触发。

### 事件： 'select-certificate'

当一个客户端认证被请求的时候被触发。

返回：

* `event` 事件
* `webContents` [web组件](browser-window.md#class-webcontents)
* `url` 字符串
* `certificateList` 对象
  * `data` PEM 编码数据
  * `issuerName` 发行者的公有名称
* `callback` 函数

```javascript
app.on('select-certificate', function(event, host, url, list, callback) {
  event.preventDefault();
  callback(list[0]);
})
```

The `url` corresponds to the navigation entry requesting the client certificate
and `callback` needs to be called with an entry filtered from the list. 
Using `event.preventDefault()` prevents the application from using the first
certificate from the store.

### 事件： 'gpu-process-crashed'

当GPU进程崩溃时触发。

## 方法

`app` 对象拥有以下的方法：

**提示:** 有的方法只能用于特定的操作系统。

### `app.quit()`

试图关掉所有的窗口。`before-quit` 事件将会被最先触发。如果所有的窗口都被成功关闭了，
`will-quit` 事件将会被触发，默认下应用将会被关闭。

这个方法保证了所有的 `beforeunload` 和 `unload` 事件处理器被正确执行。会存在一个窗口被 `beforeunload` 事件处理器返回 `false` 取消退出的可能性。

### `app.getAppPath()`

返回当前应用所在的文件路径。

### `app.getPath(name)`

* `name` 字符串

返回一个与 `name` 参数相关的特殊文件夹或文件路径。当失败时抛出一个 `Error` 。

你可以通过名称请求以下的路径：

* `home` 用户的 home 文件夹。
* `appData` 所有用户的应用数据文件夹，默认对应：
  * `%APPDATA%` Windows 中
  * `$XDG_CONFIG_HOME` or `~/.config` Linux 中
  * `~/Library/Application Support` OS X 中
* `userData` 储存你应用程序设置文件的文件夹，默认是 `appData` 文件夹附加应用的名称。
* `cache` 所有用户应用程序缓存的文件夹，默认对应：
  * `%APPDATA%` Windows 中 (没有一个通用的缓存位置)
  * `$XDG_CACHE_HOME` 或 `~/.cache` Linux 中
  * `~/Library/Caches` OS X 中
* `userCache` 用于存放应用程序缓存的文件夹，默认是 `cache` 文件夹附加应用的名称。
* `temp` 临时文件夹。
* `userDesktop` 当前用户的桌面文件夹。
* `exe` 当前的可执行文件。
* `module` `libchromiumcontent` 库。

### `app.setPath(name, path)`

* `name` 字符串
* `path` 字符串

重写 `path` 参数到一个特别的文件夹或者是一个和 `name` 参数有关系的文件。
如果这个路径指向的文件夹不存在，这个文件夹将会被这个方法创建。
如果错误则抛出 `Error` 。

你只可以指向 `app.getPath` 中定义过 `name` 的路径。You can only override paths of a `name` defined in `app.getPath`.

默认情况下，网页的 cookie 和缓存都会储存在 `userData` 文件夹。
如果你想要改变这个位置，你需要在 `app` 模块中的 `ready` 事件被触发之前重写 `userData` 的路径。

### `app.getVersion()`

返回加载应用程序的版本。如果应用程序的 `package.json` 文件中没有写版本号，
将会返回当前包或者可执行文件的版本。

### `app.getName()`

返回当前应用程序的 `package.json` 文件中的名称。

通常 `name` 字段是一个短的小写字符串，其命名规则按照 npm 中的模块命名规则。你应该单独列举一个
`productName` 字段，用于表示你的应用程序的完整名称，这个名称将会被 Electron 优先采用。

### `app.getLocale()`

返回当前应用程序的位置。

### `app.resolveProxy(url, callback)`

* `url` URL
* `callback` 函数

为 `url` 解析代理信息。 `callback`  在请求被执行之后将会被 `callback(proxy)` 调用。

### `app.addRecentDocument(path)`

* `path` 字符串

为最近访问的文档列表中添加 `path` 。

这个列表由操作系统进行管理。在 Windows 中您可以通过任务条进行访问，在 OS X 中你可以通过dock 菜单进行访问。

### `app.clearRecentDocuments()`

清除最近访问的文档列表。

### `app.setUserTasks(tasks)` _Windows_

* `tasks` 由 `Task` 对象构成的数组

将 `tasks` 添加到 Windows 中 JumpList 功能的 [Tasks][tasks] 分类中。

`tasks` 中的 `Task` 对象格式如下：

`Task` 对象
* `program` 字符串 - 执行程序的路径，通常你应该说明当前程序的路径为 `process.execPath` 字段。
* `arguments` 字符串 - 当 `program` 执行时的命令行参数。
* `title` 字符串 - JumpList 中显示的标题。
* `description` 字符串 - 对这个任务的描述。
* `iconPath` 字符串 - JumpList 中显示的 icon 的绝对路径，可以是一个任意包含一个icon的资源文件。你通常可以通过指明 `process.execPath` 来显示程序中的icon。
* `iconIndex` 整数 - icon文件中的icon目录。如果一个icon文件包括了两个或多个icon，就需要设置这个值以确定icon。如果一个文件仅包含一个icon，那么这个值为0。

### `app.commandLine.appendSwitch(switch[, value])`

通过可选的参数 `value` 给 Chromium 命令行中添加一个开关。
Append a switch (with optional `value`) to Chromium's command line.

**贴士:** 这不会影响 `process.argv` ，这个方法主要被开发者用于控制一些低层级的 Chromium 行为。

### `app.commandLine.appendArgument(value)`

给 Chromium 命令行中加入一个参数。这个参数是当前正在被引用的。

**贴士:** 这不会影响 `process.argv`。

### `app.dock.bounce([type])` _OS X_

* `type` 字符串 (可选的) - 可以是 `critical` 或 `informational`。默认下是 `informational`

当输入 `critical` 时，dock 中的 icon 将会开始弹跳直到应用被激活或者这个请求被取消。

当输入 `informational` 时，dock 中的 icon 只会弹跳一秒钟。
然而，这个请求仍然会激活，直到应用被激活或者请求被取消。

返回一个表示这个请求的 ID。

### `app.dock.cancelBounce(id)` _OS X_

* `id` 整数

取消这个 `id` 对应的请求。

### `app.dock.setBadge(text)` _OS X_

* `text` 字符串

设置 dock 中显示的字符。

### `app.dock.getBadge()` _OS X_

返回 dock 中显示的字符。

### `app.dock.hide()` _OS X_

隐藏 dock 中的 icon。

### `app.dock.show()` _OS X_

显示 dock 中的 icon。

### `app.dock.setMenu(menu)` _OS X_

* `menu` 菜单

设置应用的 [dock 菜单][dock-menu].

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks