# app

`app` 模块是为了控制整个应用的生命周期设计的。

下面的这个例子将会展示如何在最后一个窗口被关闭时退出应用：

```javascript
const app = require('electron').app;
app.on('window-all-closed', function() {
  app.quit();
});
```

## 事件

`app` 对象会触发以下的事件：

### 事件： 'will-finish-launching'

当应用程序完成基础的启动的时候被触发。在 Windows 和 Linux 中，
`will-finish-launching` 事件与 `ready` 事件是相同的； 在 OS X 中，
这个事件相当于 `NSApplication` 中的 `applicationWillFinishLaunching` 提示。
你应该经常在这里为 `open-file` 和 `open-url` 设置监听器，并启动崩溃报告和自动更新。

在大多数的情况下，你应该只在 `ready` 事件处理器中完成所有的操作。

### 事件： 'ready'

当 Electron 完成初始化时被触发。

### 事件： 'window-all-closed'

当所有的窗口都被关闭时触发。

这个事件仅在应用还没有退出时才能触发。 如果用户按下了 `Cmd + Q`，
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
返回：

* `event` 事件
* `exitCode` 整数

当应用程序正在退出时触发。

### 事件： 'open-file' _OS X_

返回：

* `event` 事件
* `path` 字符串

当用户想要在应用中打开一个文件时触发。`open-file` 事件常常在应用已经打开并且操作系统想要再次使用应用打开文件时被触发。
 `open-file` 也会在一个文件被拖入 dock 且应用还没有运行的时候被触发。
请确认在应用启动的时候（甚至在 `ready` 事件被触发前）就对 `open-file` 事件进行监听，以处理这种情况。

如果你想处理这个事件，你应该调用 `event.preventDefault()` 。
在 Windows系统中, 你需要通过解析 process.argv 来获取文件路径。
### 事件： 'open-url' _OS X_

返回：

* `event` 事件
* `url` 字符串

当用户想要在应用中打开一个URL的时候被触发。URL格式必须要提前标识才能被你的应用打开。

如果你想处理这个事件，你应该调用 `event.preventDefault()` 。

### 事件： 'activate' _OS X_

返回：

* `event` 事件
* `hasVisibleWindows` 布尔值

当应用被激活时触发，通常在点击应用的 dock 图标的时候。

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

当一个新 [浏览器窗口](browser-window.md) 被创建的时候触发。

### 事件： 'certificate-error'

返回：

* `event` 事件
* `webContents` [web组件](web-contents.md)
* `url` 统一资源定位符
* `error` 字符串-错误代码
* `certificate` 对象
  * `data` 缓冲区-PEM 编码数据
  * `issuerName` 发行者的公有名称
* `callback` 函数

验证`url`的`certificate`失败时触发, 若要信任证书，你应该通过
`event.preventDefault()`阻止默认行为然后调用`callback(true)`.

```javascript
session.on('certificate-error', function(event, webContents, url, error, certificate, callback) {
  if (url == "https://github.com") {
    // Verification logic.
    event.preventDefault();
    callback(true);
  } else {
    callback(false);
  }
});
```

### 事件： 'select-client-certificate'


返回：

* `event` 事件
* `webContents` [web组件](web-contents.md)
* `url` 统一资源定位符
* `certificateList` 对象列表
  * `data` 缓冲区-PEM 编码数据
  * `issuerName` 字符串-发行者的公有名称
* `callback` 函数

当一个客户端认证被请求的时候被触发。

`url` 对应于导航入口请求的客户端证书， `callback` 需要以从列表中过滤出的入口调用. 
使用 `event.preventDefault()` 来阻止应用使用存储的第一个证书。
```javascript
app.on('select-client-certificate', function(event, webContents, url, list, callback) {
  event.preventDefault();
  callback(list[0]);
})
```
### 事件: 'login'

返回:

* `event` 事件
* `webContents` [Web组件](web-contents.md)
* `request` 对象
  * `method` 字符串
  * `url` 统一资源定位符
  * `referrer` 统一资源定位符
* `authInfo` 对象
  * `isProxy` 布尔值
  * `scheme` 字符串
  * `host` 字符串
  * `port` 整数
  * `realm` 字符串
* `callback` 函数

当 `webContents` 要做验证时被触发。

默认行为是取消所有授权, 如果要覆盖这个行为你应该用 `event.preventDefault()` 阻止默认行为并用个人资料调用
`callback(username, password)`.

```javascript
app.on('login', function(event, webContents, request, authInfo, callback) {
  event.preventDefault();
  callback('username', 'secret');
})
```
### 事件： 'gpu-process-crashed'

当GPU进程崩溃时触发。

## 方法

`app` 对象拥有以下的方法：

**贴士:** 有的方法只能用于特定的操作系统。

### `app.quit()`

试图关掉所有的窗口。`before-quit` 事件将会被最先触发。如果所有的窗口都被成功关闭了，
`will-quit` 事件将会被触发，默认下应用将会终止。

这个方法保证了所有的 `beforeunload` 和 `unload` 事件处理器被正确执行。存在一个窗口因为 `beforeunload` 事件处理器返回 `false` 而取消退出的可能性。

### `app.hide()` _OS X_

隐藏所有的应用窗口，不是最小化.

### `app.show()` _OS X_

隐藏后重新显示所有的窗口，不会自动选中他们。

### `app.exit(exitCode)`

* `exitCode` 整数

立即以`exitCode`退出应用.

所有的窗口会被立刻关闭，不会询问用户且`before-quit` 和 `will-quit` 这2个事件不会被触发


### `app.getAppPath()`

返回当前应用所在的文件路径。

### `app.getPath(name)`

* `name` 字符串

返回一个与 `name` 参数相关的特殊文件夹或文件路径。当失败时抛出一个 `Error` 。

你可以通过名称请求以下的路径：

* `home` 用户的 home 文件夹。
* `appData` 所有的用户应用的数据文件夹，默认指向：
  * `%APPDATA%` Windows 中
  * `$XDG_CONFIG_HOME` 或 `~/.config` Linux 中
  * `~/Library/Application Support` OS X 中
* `userData` 储存你应用程序设置文件的文件夹，默认是 `appData` 文件夹附加应用的名称。
* `temp` 临时文件夹。
* `exe` 当前的可执行文件
* `module`  `libchromiumcontent` 库.
* `desktop` 当前用户的桌面文件夹。
* `documents` 用户"我的文档"的路径.
* `downloads` 用户下载目录的路径.
* `music` 用户音乐目录的路径.
* `pictures` 用户图片目录的路径.
* `videos` 用户视频目录的路径.

### `app.setPath(name, path)`

* `name` 字符串
* `path` 字符串

覆盖与`name`相关联的特殊文件夹或文件路径的`path`。如果这个路径指向不存在的文件夹，则会创建该文件夹。如果错误则抛出 `Error` 。

你只可以覆盖 `app.getPath` 中定义过 `name` 的路径。

默认情况下，网页的 cookie 和缓存都会储存在 `userData` 文件夹。
如果你想要改变这个位置，你必须在 `app` 模块中的 `ready` 事件被触发之前重写 `userData` 的路径。

### `app.getVersion()`

返回加载应用程序的版本。如果应用程序的 `package.json` 文件中没有写版本号，
将会返回当前包或者可执行文件的版本。

### `app.getName()`

返回当前应用程序的 `package.json` 文件中的名称。

通常情况下，`package.json` 中的 `name` 字段是一个短的小写字母名称，其命名规则按照 npm 中的模块命名规则。你应该单独列举一个
`productName` 字段，用于表示你的应用程序的完整大写名称，这个名称将会被 Electron 优先采用。

### `app.getLocale()`

返回当前应用程序的语言种类。



### `app.addRecentDocument(path)`  _OS X_ _Windows_

* `path` 字符串

为最近访问的文档列表中添加 `path` 。

这个列表由操作系统进行管理。在 Windows 中您可以通过任务栏进行访问，在 OS X 中你可以通过dock 菜单进行访问。

### `app.clearRecentDocuments()` _OS X_ _Windows_

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
* `iconIndex` 整数 - icon文件中的icon索引。如果一个icon文件包括了两个或多个icon，就需要设置这个值以确定icon。如果一个文件仅包含一个icon，那么这个值为0。

### `app.allowNTLMCredentialsForAllDomains(allow)`

* `allow` 布尔值

Dynamically sets whether to always send credentials for HTTP NTLM or Negotiate
authentication - normally, Electron will only send NTLM/Kerberos credentials for
URLs that fall under "Local Intranet" sites (i.e. are in the same domain as you).
However, this detection often fails when corporate networks are badly configured,
so this lets you co-opt this behavior and enable it for all URLs.
### `app.makeSingleInstance(callback)`

* `callback` 函数

这个方法时你的应用变为单实例应用 - 而不是允许你的应用运行多个实例, 这能保证你的应用始终只运行着单个实例, 其它实例通知该实例并退出。

当第二个实例被执行时，`callback` 将会以`callback(argv, workingDirectory)` 被调用。 `argv` 是第二个实例的命令行参数数组,  `workingDirectory` 是它的当前工作目录。通常情况下，应用通过使初级窗口最前并不可最小化来对此进行响应。

`callback` 要保证在 `app` 中的 `ready` 事件触发后才被执行。

如果你的进程是该应用的初级实例，这个方法返回 `false` 并且你的应用应该继续加载。而如果你的进程已经将它的参数发送给其它实例，则会返回`true`, 你应该立即退出。

在 OS X 上，当用户尝试在 Finder 中打开应用的第二个实例时，系统会自动强制单实例，而且 `open-file` 和 `open-url`
事件会被触发。但是当用户从命令行启动应用时，系统的单实例运作机制被规避，这时你必须使用该方法来确保单实例。

当第二个实例启动时激活初级实例窗口的例子:

```js
var myWindow = null;

var shouldQuit = app.makeSingleInstance(function(commandLine, workingDirectory) {
  // Someone tried to run a second instance, we should focus our window.
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore();
    myWindow.focus();
  }
  return true;
});

if (shouldQuit) {
  app.quit();
  return;
}

// Create myWindow, load the rest of the app, etc...
app.on('ready', function() {
});
```
### `app.setAppUserModelId(id)` _Windows_

* `id` 字符串

将 [Application User Model ID][app-user-model-id] 改为 `id`.

### `app.isAeroGlassEnabled()` _Windows_

如果 [DWM composition](https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx)
(Aero Glass) 设为允许, 这个方法返回 `true` ，否则返回 `false`。你可以用这个方法来决定是否创造一个透明窗口 (当 DWM composition 设为禁止时，透明窗口不会正确工作)。

用法示例:

```js
let browserOptions = {width: 1000, height: 800};

// Make the window transparent only if the platform supports it.
if (process.platform !== 'win32' || app.isAeroGlassEnabled()) {
  browserOptions.transparent = true;
  browserOptions.frame = false;
}

// Create the window.
win = new BrowserWindow(browserOptions);

// Navigate.
if (browserOptions.transparent) {
  win.loadURL('file://' + __dirname + '/index.html');
} else {
  // No transparency, so we load a fallback that uses basic styles.
  win.loadURL('file://' + __dirname + '/fallback.html');
}
```
### `app.commandLine.appendSwitch(switch[, value])`

通过可选的参数 `value` 给 Chromium 命令行中添加一个开关。

**贴士:** 这不会影响 `process.argv` ，这个方法主要被开发者用于控制一些低层级的 Chromium 行为。

### `app.commandLine.appendArgument(value)`

给 Chromium 命令行中加入一个参数。这个参数是当前正在被引用的。

**贴士:** 这不会影响 `process.argv`。

### `app.dock.bounce([type])` _OS X_

* `type` 字符串 (可选的) - 可以是 `critical` 或 `informational`。默认下是 `informational`

当传入 `critical` 时，dock 中的 icon 将会开始弹跳直到应用被激活或者这个请求被取消。

当传入 `informational` 时，dock 中的 icon 只会弹跳一秒钟。
然而，这个请求保留激活状态，直到应用被激活或者请求被取消。

返回一个表示这个请求的 ID。

### `app.dock.cancelBounce(id)` _OS X_

* `id` 整数

取消这个 `id` 的弹跳。

### `app.dock.setBadge(text)` _OS X_

* `text` 字符串

设置 dock 标记区域中显示的字符串。

### `app.dock.getBadge()` _OS X_

返回 dock 标记区域中显示的字符串。

### `app.dock.hide()` _OS X_

隐藏 dock 中的 icon。

### `app.dock.show()` _OS X_

显示 dock 中的 icon。

### `app.dock.setMenu(menu)` _OS X_

* `menu` 菜单

设置应用的 [dock 菜单][dock-menu].

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
