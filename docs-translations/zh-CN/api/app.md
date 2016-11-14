# app

> `app` 模块是为了控制整个应用的生命周期设计的。

进程: [主进程](../tutorial/quick-start.md#main-process)

下面的这个例子将会展示如何在最后一个窗口被关闭时退出应用：

```javascript
const {app} = require('electron')
app.on('window-all-closed', () => {
  app.quit()
})
```

## 事件列表

`app` 对象会触发以下的事件：

### 事件：'will-finish-launching'

当应用程序完成基础的启动的时候被触发。在 Windows 和 Linux 中，
`will-finish-launching` 事件与 `ready` 事件是相同的； 在 macOS 中，
这个事件相当于 `NSApplication` 中的 `applicationWillFinishLaunching` 提示。
你应该经常在这里为 `open-file` 和 `open-url` 设置监听器，并启动崩溃报告和自动更新。

在大多数的情况下，你应该只在 `ready` 事件处理器中完成所有的业务。

### 事件：'ready'

返回:

* `launchInfo` Object _macOS_

当 Electron 完成初始化时被触发。在 macOs 中， 如果从通知中心中启动，那么`launchInfo` 中的`userInfo`包含
用来打开应用程序的 `NSUserNotification` 信息。你可以通过调用 `app.isReady()` 
方法来检查此事件是否已触发。

### 事件：'window-all-closed'

当所有的窗口都被关闭时触发。

如果您没有监听此事件，当所有窗口都已关闭时，默认值行为是退出应用程序。但如果你监听此事件，
将由你来控制应用程序是否退出。 如果用户按下了 `Cmd + Q`，或者开发者调用了 `app.quit()` ，
Electron 将会先尝试关闭所有的窗口再触发 `will-quit` 事件，在这种情况下 `window-all-closed`
 不会被触发。

### 事件：'before-quit'

返回：

* `event` Event

在应用程序开始关闭它的窗口的时候被触发。
调用 `event.preventDefault()` 将会阻止终止应用程序的默认行为。

### 事件：'will-quit'

返回：

* `event` Event

当所有的窗口已经被关闭，应用即将退出时被触发。
调用 `event.preventDefault()` 将会阻止终止应用程序的默认行为。

你可以在 `window-all-closed` 事件的描述中看到 `will-quit` 事件
和 `window-all-closed` 事件的区别。

### 事件：'quit'
返回：

* `event` Event
* `exitCode` Integer

当应用程序正在退出时触发。

### 事件：'open-file' _macOS_

返回：

* `event` Event
* `path` String

当用户想要在应用中打开一个文件时触发。`open-file` 事件常常在应用已经打开并且系统想要再次使用应用打开文件时被触发。
 `open-file` 也会在一个文件被拖入 dock 且应用还没有运行的时候被触发。
请确认在应用启动的时候（甚至在 `ready` 事件被触发前）就对 `open-file` 事件进行监听，以处理这种情况。

如果你想处理这个事件，你应该调用 `event.preventDefault()` 。
在 Windows系统中, 你需要通过解析 process.argv 来获取文件路径。

### 事件：'open-url' _macOS_

返回：

* `event` Event
* `url` String

当用户想要在应用中打开一个url的时候被触发。URL格式必须要提前标识才能被你的应用打开。

如果你想处理这个事件，你应该调用 `event.preventDefault()` 。

### 事件：'activate' _macOS_

返回：

* `event` Event
* `hasVisibleWindows` Boolean

当应用被激活时触发，常用于点击应用的 dock 图标的时候。

### 事件: 'continue-activity' _macOS_

返回:

* `event` Event
* `type` String - 标识当前状态的字符串。 映射到[`NSUserActivity.activityType`] [activity-type]。
* `userInfo` Object - 包含由另一个设备上的活动所存储的应用程序特定的状态。

当来自不同设备的活动通过 [Handoff][handoff] 想要恢复时触发。如果需要处理这个事件，
调用 `event.preventDefault()` 方法。
只有具有支持相应的活动类型并且相同的开发团队ID作为启动程序时，用户行为才会进行。
所支持活动类型已在应用的`Info.plist`中的`NSUserActivityTypes`明确定义。

### 事件：'browser-window-blur'

返回：

* `event` Event
* `window` BrowserWindow

当一个 [BrowserWindow](browser-window.md) 失去焦点的时候触发。

### 事件：'browser-window-focus'

返回：

* `event` Event
* `window` BrowserWindow

当一个 [BrowserWindow](browser-window.md) 获得焦点的时候触发。

### 事件：'browser-window-created'

返回：

* `event` Event
* `window` BrowserWindow

当一个 [BrowserWindow](browser-window.md) 被创建的时候触发。

### 事件: 'web-contents-created'

Returns:

* `event` Event
* `webContents` WebContents

在新的 [webContents](web-contents.md) 创建后触发.


### 事件：'certificate-error'

返回：

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` String - URL 地址
* `error` String - 错误码
* `certificate` Object
  * `data` Buffer - PEM 编码数据
  * `issuerName` String - 发行者的公有名称
* `callback` Function

当对 `url` 验证 `certificate` 证书失败的时候触发，如果需要信任这个证书，你需要阻止默认行为 `event.preventDefault()` 并且
调用 `callback(true)`。

```javascript
const {app} = require('electron')

app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
  if (url === 'https://github.com') {
    // Verification logic.
    event.preventDefault()
    callback(true)
  } else {
    callback(false)
  }
})
```

### 事件：'select-client-certificate'


返回：

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` String - URL 地址
* `certificateList` [Object]
  * `data` Buffer - PEM 编码数据
  * `issuerName` String - 发行者的公有名称
* `callback` Function

当一个客户端认证被请求的时候被触发。

`url` 指的是请求客户端认证的网页地址，调用 `callback` 时需要传入一个证书列表中的证书。

需要通过调用 `event.preventDefault()` 来防止应用自动使用第一个证书进行验证。如下所示：

```javascript
app.on('select-certificate', function (event, host, url, list, callback) {
  event.preventDefault()
  callback(list[0])
})
```
### 事件: 'login'

返回：

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

当 `webContents` 要做进行一次 HTTP 登陆验证时被触发。

默认情况下，Electron 会取消所有的验证行为，如果需要重写这个行为，你需要用 `event.preventDefault()` 来阻止默认行为，并且
用 `callback(username, password)` 来进行验证。

```javascript
const {app} = require('electron')

app.on('login', (event, webContents, request, authInfo, callback) => {
  event.preventDefault()
  callback('username', 'secret')
})
```
### 事件：'gpu-process-crashed'

返回:

* `event` Event
* `killed` Boolean

当 GPU 进程崩溃时触发。

### 事件: 'accessibility-support-changed' _macOS_ _Windows_

返回:

* `event` Event
* `accessibilitySupportEnabled` Boolean - 当启用Chrome的辅助功能时候为`true`, 其他情况为 `false`.

当 Chrome 的辅助功能状态改变时触发，比如屏幕阅读被启用或被禁用。
点此 https://www.chromium.org/developers/design-documents/accessibility 查看更多详情。


## 方法列表

`app` 对象拥有以下的方法：

**请注意** 有的方法只能用于特定的操作系统，并被标注。

### `app.quit()`

试图关掉所有的窗口。`before-quit` 事件将会最先被触发。如果所有的窗口都被成功关闭了，
`will-quit` 事件将会被触发，默认下应用将会被关闭。

这个方法保证了所有的 `beforeunload` 和 `unload` 事件处理器被正确执行。假如一个窗口的 `beforeunload` 事件处理器返回 `false`，那么整个应用可能会取消退出。

### `app.exit(exitCode)`

* `exitCode` 整数

带着`exitCode`退出应用，`exitCode` 默认为0

所有的窗口会被立刻关闭，不会询问用户。`before-quit` 和 `will-quit` 这2个事件不会被触发

### `app.relaunch([options])`

* `options` Object (optional)
  * `args` String[] (optional)
  * `execPath` String (optional)

当前实例退出，重启应用。

默认情况下，新的实例会和当前实例使用相同的工作目录以及命令行参数。指定 `args` 参数，
`args` 将会被作为替换的命令行参数。指定 `execPath` 参数，`execPath` 将会作为执行的目录。

记住，这个方法不会退出正在执行的应用。你需要在调用`app.relaunch`方法后再执行`app.quit`或者`app.exit`
来让应用重启。


调用多次`app.relaunch`方法，当前实例退出后多个实例会被启动。

例子：立即重启当前实例并向新的实例添加一个新的命令行参数

```javascript
const {app} = require('electron')

app.relaunch({args: process.argv.slice(1).concat(['--relaunch'])})
app.exit(0)
```

### `app.isReady()`

返回 `Boolean` - `true` 如果Electron 初始化完成, `false` 其他情况.

### `app.focus()`

在Linux系统中, 使第一个可见窗口获取焦点. macOS, 让该应用成为活动应用程序。
Windows, 使应用的第一个窗口获取焦点.

### `app.hide()` _macOS_

隐藏所有的应用窗口，不是最小化.

### `app.show()` _macOS_

隐藏后重新显示所有的窗口，不会自动选中他们。

### `app.getAppPath()`

返回当前应用所在的文件路径。

### `app.getPath(name)`

* `name` String

返回一个与 `name` 参数相关的特殊文件夹或文件路径。当失败时抛出一个 `Error` 。

你可以通过名称请求以下的路径：

* `home` 用户的 home 文件夹（主目录）
* `appData` 当前用户的应用数据文件夹，默认对应：
  * `%APPDATA%` Windows 中
  * `$XDG_CONFIG_HOME` or `~/.config` Linux 中
  * `~/Library/Application Support` macOS 中
* `userData` 储存你应用程序设置文件的文件夹，默认是 `appData` 文件夹附加应用的名称
* `temp` 临时文件夹
* `exe` 当前的可执行文件
* `module`  `libchromiumcontent` 库
* `desktop` 当前用户的桌面文件夹
* `documents` 用户文档目录的路径
* `downloads` 用户下载目录的路径.
* `music` 用户音乐目录的路径.
* `pictures` 用户图片目录的路径.
* `videos` 用户视频目录的路径.

### `app.setPath(name, path)`

* `name` String
* `path` String

重写某个 `name` 的路径为 `path`，`path` 可以是一个文件夹或者一个文件，这个和 `name` 的类型有关。
如果这个路径指向的文件夹不存在，这个文件夹将会被这个方法创建。
如果错误则会抛出 `Error`。

`name` 参数只能使用 `app.getPath` 中定义过 `name`。

默认情况下，网页的 cookie 和缓存都会储存在 `userData` 文件夹。
如果你想要改变这个位置，你需要在 `app` 模块中的 `ready` 事件被触发之前重写 `userData` 的路径。

### `app.getVersion()`

返回加载应用程序的版本。如果应用程序的 `package.json` 文件中没有写版本号，
将会返回当前包或者可执行文件的版本。

### `app.getName()`

返回当前应用程序的 `package.json` 文件中的名称。

由于 npm 的命名规则，通常 `name` 字段是一个短的小写字符串。但是应用名的完整名称通常是首字母大写的，你应该单独使用一个
`productName` 字段，用于表示你的应用程序的完整名称。Electron 会优先使用这个字段作为应用名。

### `app.setName(name)`

* `name` String

重写当前应用的名字

### `app.getLocale()`

返回当前应用程序的语言。

### `app.addRecentDocument(path)`  _macOS_ _Windows_

* `path` String

在最近访问的文档列表中添加 `path`。

这个列表由操作系统进行管理。在 Windows 中您可以通过任务条进行访问，在 macOS 中你可以通过 dock 菜单进行访问。

### `app.clearRecentDocuments()` _macOS_ _Windows_

清除最近访问的文档列表。

### `app.setAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - 协议的名字, 不包含 `://`.如果你希望你的应用处理链接 `electron://` ,
 将 `electron` 作为该方法的参数.
* `path` String (optional) _Windows_ - Defaults to `process.execPath`
* `args` String[] (optional) _Windows_ - Defaults to an empty array

返回 `Boolean` - 调用是否成功.

此方法将当前可执行程序设置为协议(亦称 URI scheme)的默认处理程序。 
这允许您将应用程序更深入地集成到操作系统中. 一旦注册成功,
所有 `your-protocol://` 格式的链接都会使用你的程序打开。整个链接（包括协议）将作为参数传递到应用程序中。

在Windows系统中，你可以提供可选参数path，到执行文件的地址；args,一个在启动时传递给可执行文件的参数数组

**注意:** 在macOS上，您只能注册已添加到应用程序的`info.plist`的协议，该协议不能在运行时修改。 
但是，您可以在构建时使用简单的文本编辑器或脚本更改文件。 有关详细信息，请参阅 [Apple's documentation][CFBundleURLTypes] 

该API在内部使用Windows注册表和lssetdefaulthandlerforurlscheme。

### `app.removeAsDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - 协议的名字, 不包含 `://`.
* `path` String (optional) _Windows_ - 默认为 `process.execPath`
* `args` String[] (optional) _Windows_ - 默认为空数组

返回 `Boolean` - 调用是否成功.

此方法检查当前程序是否为协议（也称为URI scheme）的默认处理程序。
如果是，它会移除程序默认处理该协议。

### `app.isDefaultProtocolClient(protocol[, path, args])` _macOS_ _Windows_

* `protocol` String - 协议的名字, 不包含 `://`.
* `path` String (optional) _Windows_ - 默认值  `process.execPath`
* `args` String[] (optional) _Windows_ - 默认为空数组

返回 `Boolean`

此方法检查当前程序是否为协议（也称为URI scheme）的默认处理程序。
是则返回true 否则返回false

**提示:** 在 macOS 系统中, 您可以使用此方法检查应用程序是否已注册为协议的默认处理程序。
同时可以通过查看 `~/Library/Preferences/com.apple.LaunchServices.plist` 来确认。 
有关详细信息，请参阅 [Apple's documentation][LSCopyDefaultHandlerForURLScheme] 。

该API在内部使用Windows注册表和lssetdefaulthandlerforurlscheme。

### `app.setUserTasks(tasks)` _Windows_

* `tasks` [Task] - 一个由 Task 对象构成的数组

将 `tasks` 添加到 Windows 中 JumpList 功能的 [Tasks][tasks] 分类中。

`tasks` 中的 `Task` 对象格式如下：

`Task` Object
* `program` String - 执行程序的路径，通常你应该说明当前程序的路径为 `process.execPath` 字段。
* `arguments` String - 当 `program` 执行时的命令行参数。
* `title` String - JumpList 中显示的标题。
* `description` String - 对这个任务的描述。
* `iconPath` String - JumpList 中显示的图标的绝对路径，可以是一个任意包含一个图标的资源文件。通常来说，你可以通过指明 `process.execPath` 来显示程序中的图标。
* `iconIndex` Integer - 图标文件中的采用的图标位置。如果一个图标文件包括了多个图标，就需要设置这个值以确定使用的是哪一个图标。
如果这个图标文件中只包含一个图标，那么这个值为 0。

返回 `Boolean` - 执行是否成功.

**提示:** 如果希望更多的定制任务栏跳转列表，请使用 `app.setJumpList(categories)` 。

### `app.getJumpListSettings()` _Windows_

返回 `Object`:

* `minItems` Integer - 将在跳转列表中显示项目的最小数量 (有关此值的更详细描述，请参阅
  [MSDN docs][JumpListBeginListMSDN]).
* `removedItems` [JumpListItem[]](structures/jump-list-item.md) -  `JumpListItem` 对象数组，对应用户在跳转列表中明确删除的项目。
这些项目不能在 **接下来**调用`app.setJumpList()` 时重新添加到跳转列表中,
Windows不会显示任何包含已删除项目的自定义类别.

### `app.setJumpList(categories)` _Windows_

* `categories` [JumpListCategory[]](structures/jump-list-category.md) 或者 `null` - `JumpListCategory` 对象的数组.

设置或删除应用程序的自定义跳转列表，并返回以下字符串之一：

* `ok` - 没有出现错误。
* `error` - 发生一个或多个错误，启用运行日志记录找出可能的原因。
* `invalidSeparatorError` -尝试向跳转列表中的自定义跳转列表添加分隔符。 分隔符只允许在标准的 `Tasks` 类别中。
* `fileTypeRegistrationError` - 尝试向自定义跳转列表添加一个文件链接，但是该应用未注册处理该应用类型。
* `customCategoryAccessDeniedError` - 由于用户隐私或策略组设置，自定义类别无法添加到跳转列表。

如果`categories` 值为 `null` ，之前设定的自定义跳转列表(如果存在)将被替换为
标准的应用跳转列表(由windows生成)

`JumpListCategory` 对象需要包含以下属性：

* `type` String - 以下其中一个：
  * `tasks` - 此类别中的项目将被放置到标准的`Tasks`类别中。只能有一个这样的类别，
    将总是显示在跳转列表的底部。
  * `frequent` - 显示应用常用文件列表，类别的名称及其项目由Windows设置。
  * `recent` - 显示应用最近打开的文件的列表，类别的名称及其项目由Windows设置。 
    可以使用`app.addRecentDocument（path）`间接添加到项目到此类别。
  * `custom` - 显示任务或文件链接，`name`必须由应用程序设置。
* `name` String - 当`type` 为 `custom` 时此值为必填项,否则应省略。
* `items` Array - `JumpListItem` 对象数组，如果 `type` 值为 `tasks` 或
  `custom` 时必填，否则应省略。

**Note:** 如果`JumpListCategory`对象没有设置`type`和`name`属性，
那么`type`默认为`tasks`。 如果设置`name`属性，省略`type`属性，
则`type`默认为`custom`。

**Note:** Users can remove items from custom categories, and Windows will not
allow a removed item to be added back into a custom category until **after**
the next successful call to `app.setJumpList(categories)`. Any attempt to
re-add a removed item to a custom category earlier than that will result in the
entire custom category being omitted from the Jump List. The list of removed
items can be obtained using `app.getJumpListSettings()`.

`JumpListItem` objects should have the following properties:

* `type` String - One of the following:
  * `task` - A task will launch an app with specific arguments.
  * `separator` - Can be used to separate items in the standard `Tasks`
    category.
  * `file` - A file link will open a file using the app that created the
    Jump List, for this to work the app must be registered as a handler for
    the file type (though it doesn't have to be the default handler).
* `path` String - Path of the file to open, should only be set if `type` is
  `file`.
* `program` String - Path of the program to execute, usually you should
  specify `process.execPath` which opens the current program. Should only be
  set if `type` is `task`.
* `args` String - The command line arguments when `program` is executed. Should
  only be set if `type` is `task`.
* `title` String - The text to be displayed for the item in the Jump List.
  Should only be set if `type` is `task`.
* `description` String - Description of the task (displayed in a tooltip).
  Should only be set if `type` is `task`.
* `iconPath` String - The absolute path to an icon to be displayed in a
  Jump List, which can be an arbitrary resource file that contains an icon
  (e.g. `.ico`, `.exe`, `.dll`). You can usually specify `process.execPath` to
  show the program icon.
* `iconIndex` Integer - The index of the icon in the resource file. If a
  resource file contains multiple icons this value can be used to specify the
  zero-based index of the icon that should be displayed for this task. If a
  resource file contains only one icon, this property should be set to zero.

以下是一个创建一个自定义跳转列表的简单例子

```javascript
const {app} = require('electron')

app.setJumpList([
  {
    type: 'custom',
    name: 'Recent Projects',
    items: [
      { type: 'file', path: 'C:\\Projects\\project1.proj' },
      { type: 'file', path: 'C:\\Projects\\project2.proj' }
    ]
  },
  { // has a name so `type` is assumed to be "custom"
    name: 'Tools',
    items: [
      {
        type: 'task',
        title: 'Tool A',
        program: process.execPath,
        args: '--run-tool-a',
        icon: process.execPath,
        iconIndex: 0,
        description: 'Runs Tool A'
      },
      {
        type: 'task',
        title: 'Tool B',
        program: process.execPath,
        args: '--run-tool-b',
        icon: process.execPath,
        iconIndex: 0,
        description: 'Runs Tool B'
      }
    ]
  },
  { type: 'frequent' },
  { // has no name and no type so `type` is assumed to be "tasks"
    items: [
      {
        type: 'task',
        title: 'New Project',
        program: process.execPath,
        args: '--new-project',
        description: 'Create a new project.'
      },
      { type: 'separator' },
      {
        type: 'task',
        title: 'Recover Project',
        program: process.execPath,
        args: '--recover-project',
        description: 'Recover Project'
      }
    ]
  }
])
```

### `app.makeSingleInstance(callback)`

* `callback` Function

这个方法可以让你的应用在同一时刻最多只会有一个实例，否则你的应用可以被运行多次并产生多个实例。你可以利用这个接口保证只有一个实例正
常运行，其余的实例全部会被终止并退出。

如果多个实例同时运行，那么第一个被运行的实例中 `callback` 会以 `callback(argv, workingDirectory)` 的形式被调用。其余的实例
会被终止。
`argv` 是一个包含了这个实例的命令行参数列表的数组，`workingDirectory` 是这个实例目前的运行目录。通常来说，我们会用通过将应用在
主屏幕上激活，并且取消最小化，来提醒用户这个应用已经被打开了。

在 `app` 的 `ready` 事件后，`callback` 才有可能被调用。

如果当前实例为第一个实例，那么在这个方法将会返回 `false` 来保证它继续运行。否则将会返回 `true` 来让它立刻退出。

在 macOS 中，如果用户通过 Finder、`open-file` 或者 `open-url` 打开应用，系统会强制确保只有一个实例在运行。但是如果用户是通过
命令行打开，这个系统机制会被忽略，所以你仍然需要靠这个方法来保证应用为单实例运行的。

下面是一个简单的例子。我们可以通过这个例子了解如何确保应用为单实例运行状态。

```javascript
const {app} = require('electron')
let myWindow = null

const shouldQuit = app.makeSingleInstance((commandLine, workingDirectory) => {
  // Someone tried to run a second instance, we should focus our window.
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore()
    myWindow.focus()
  }
})

if (shouldQuit) {
  app.quit()
}

// Create myWindow, load the rest of the app, etc...
app.on('ready', () => {
})
```

### `app.releaseSingleInstance()`

释放所有由 `makeSingleInstance` 创建的限制. 
这将允许应用程序的多个实例依次运行.

### `app.setUserActivity(type, userInfo[, webpageURL])` _macOS_

* `type` String - Uniquely identifies the activity. Maps to
  [`NSUserActivity.activityType`][activity-type].
* `userInfo` Object - App-specific state to store for use by another device.
* `webpageURL` String - The webpage to load in a browser if no suitable app is
  installed on the resuming device. The scheme must be `http` or `https`.

Creates an `NSUserActivity` and sets it as the current activity. The activity
is eligible for [Handoff][handoff] to another device afterward.

### `app.getCurrentActivityType()` _macOS_

Returns `String` - The type of the currently running activity.

### `app.setAppUserModelId(id)` _Windows_

* `id` String

改变当前应用的 [Application User Model ID][app-user-model-id] 为 `id`.

### `app.importCertificate(options, callback)` _LINUX_

* `options` Object
  * `certificate` String - Path for the pkcs12 file.
  * `password` String - Passphrase for the certificate.
* `callback` Function
  * `result` Integer - Result of import.

Imports the certificate in pkcs12 format into the platform certificate store.
`callback` is called with the `result` of import operation, a value of `0`
indicates success while any other value indicates failure according to chromium [net_error_list](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).

### `app.disableHardwareAcceleration()`

Disables hardware acceleration for current app.

This method can only be called before app is ready.

### `app.setBadgeCount(count)` _Linux_ _macOS_

* `count` Integer

Returns `Boolean` - Whether the call succeeded.

Sets the counter badge for current app. Setting the count to `0` will hide the
badge.

On macOS it shows on the dock icon. On Linux it only works for Unity launcher,

**Note:** Unity launcher requires the exsistence of a `.desktop` file to work,
for more information please read [Desktop Environment Integration][unity-requiremnt].

### `app.getBadgeCount()` _Linux_ _macOS_

Returns `Integer` - The current value displayed in the counter badge.

### `app.isUnityRunning()` _Linux_

Returns `Boolean` - Whether the current desktop environment is Unity launcher.

### `app.getLoginItemSettings()` _macOS_ _Windows_

Returns `Object`:

* `openAtLogin` Boolean - `true` if the app is set to open at login.
* `openAsHidden` Boolean - `true` if the app is set to open as hidden at login.
  This setting is only supported on macOS.
* `wasOpenedAtLogin` Boolean - `true` if the app was opened at login
  automatically. This setting is only supported on macOS.
* `wasOpenedAsHidden` Boolean - `true` if the app was opened as a hidden login
  item. This indicates that the app should not open any windows at startup.
  This setting is only supported on macOS.
* `restoreState` Boolean - `true` if the app was opened as a login item that
  should restore the state from the previous session. This indicates that the
  app should restore the windows that were open the last time the app was
  closed. This setting is only supported on macOS.

**Note:** This API has no effect on
[MAS builds][mas-builds].

### `app.setLoginItemSettings(settings)` _macOS_ _Windows_

* `settings` Object
  * `openAtLogin` Boolean - `true` to open the app at login, `false` to remove
    the app as a login item. Defaults to `false`.
  * `openAsHidden` Boolean - `true` to open the app as hidden. Defaults to
    `false`. The user can edit this setting from the System Preferences so
    `app.getLoginItemStatus().wasOpenedAsHidden` should be checked when the app
    is opened to know the current value. This setting is only supported on
    macOS.

Set the app's login item settings.

**Note:** This API has no effect on
[MAS builds][mas-builds].

### `app.isAccessibilitySupportEnabled()` _macOS_ _Windows_

Returns `Boolean` - `true` if Chrome's accessibility support is enabled,
`false` otherwise. This API will return `true` if the use of assistive
technologies, such as screen readers, has been detected. See
https://www.chromium.org/developers/design-documents/accessibility for more
details.

### `app.setAboutPanelOptions(options)` _macOS_

* `options` Object
  * `applicationName` String (optional) - The app's name.
  * `applicationVersion` String (optional) - The app's version.
  * `copyright` String (optional) - Copyright information.
  * `credits` String (optional) - Credit information.
  * `version` String (optional) - The app's build version number.

Set the about panel options. This will override the values defined in the app's
`.plist` file. See the [Apple docs][about-panel-options] for more details.

### `app.commandLine.appendSwitch(switch[, value])`

通过可选的参数 `value` 给 Chromium 中添加一个命令行开关。

**注意** 这个方法不会影响 `process.argv`，我们通常用这个方法控制一些底层 Chromium 行为。

### `app.commandLine.appendArgument(value)`

给 Chromium 中直接添加一个命令行参数，这个参数 `value` 的引号和格式必须正确。

**注意** 这个方法不会影响 `process.argv`。

### `app.dock.bounce([type])` _macOS_

* `type` String - 可选参数，可以是 `critical` 或 `informational`。默认为 `informational`。

当传入的是 `critical` 时，dock 中的应用将会开始弹跳，直到这个应用被激活或者这个请求被取消。

当传入的是 `informational` 时，dock 中的图标只会弹跳一秒钟。但是，这个请求仍然会激活，直到应用被激活或者请求被取消。

这个方法返回的返回值表示这个请求的 ID。

### `app.dock.cancelBounce(id)` _macOS_

* `id` Integer

取消这个 `id` 对应的请求。

### `app.dock.downloadFinished(filePath)` _macOS_

* `filePath` String

Bounces the Downloads stack if the filePath is inside the Downloads folder.

### `app.dock.setBadge(text)` _macOS_

* `text` String

设置应用在 dock 中显示的字符串。

### `app.dock.getBadge()` _macOS_

返回应用在 dock 中显示的字符串。

### `app.dock.hide()` _macOS_

隐藏应用在 dock 中的图标。

### `app.dock.show()` _macOS_

显示应用在 dock 中的图标。

### `app.dock.isVisible()` _macOS_

Returns `Boolean` - dock 图标是否可见.
`app.dock.show()` 是异步方法，因此此方法可能无法在调用之后立即返回true.

### `app.dock.setMenu(menu)` _macOS_

* `menu` [Menu](menu.md)

设置应用的 [dock 菜单][dock-menu].

### `app.dock.setIcon(image)` _macOS_

* `image` [NativeImage](native-image.md)

设置应用在 dock 中显示的图标。


[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[CFBundleURLTypes]: https://developer.apple.com/library/ios/documentation/General/Reference/InfoPlistKeyReference/Articles/CoreFoundationKeys.html#//apple_ref/doc/uid/TP40009249-102207-TPXREF115
[LSCopyDefaultHandlerForURLScheme]: https://developer.apple.com/library/mac/documentation/Carbon/Reference/LaunchServicesReference/#//apple_ref/c/func/LSCopyDefaultHandlerForURLScheme
[handoff]: https://developer.apple.com/library/ios/documentation/UserExperience/Conceptual/Handoff/HandoffFundamentals/HandoffFundamentals.html
[activity-type]: https://developer.apple.com/library/ios/documentation/Foundation/Reference/NSUserActivity_Class/index.html#//apple_ref/occ/instp/NSUserActivity/activityType
[unity-requiremnt]: ../tutorial/desktop-environment-integration.md#unity-launcher-shortcuts-linux
[mas-builds]: ../tutorial/mac-app-store-submission-guide.md
[JumpListBeginListMSDN]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378398(v=vs.85).aspx
[about-panel-options]: https://developer.apple.com/reference/appkit/nsapplication/1428479-orderfrontstandardaboutpanelwith?language=objc
