# 桌面环境集成
不同的操作系统在各自的桌面应用上提供了不同的特性。例如，在 windows 上应用曾经打开的文件会出现在任务栏的跳转列表，在 Mac 上，应用可以把自定义菜单放在鱼眼菜单上。

本章将会说明怎样使用 Electron APIs 把你的应用和桌面环境集成到一块。

## Notifications (Windows, Linux, macOS)

这三个操作系统都为用户提供了发送通知的方法。Electron让开发人员通过
[HTML5 Notification API](https://notifications.spec.whatwg.org/)
便利的去发送通知，用操作系统自带的通知APIs去显示。

**Note:** 因为这是一个HTML5API，所以只在渲染进程中起作用

```javascript
var myNotification = new Notification('Title', {
  body: 'Lorem Ipsum Dolor Sit Amet'
})

myNotification.onclick = function () {
  console.log('Notification clicked')
}
```

尽管代码和用户体验在不同的操作系统中基本相同，但还是有一些差异。

### Windows

* 在Windows 10上, 通知"可以工作".
* 在Windows 8.1和Windows 8系统下，你需要将你的应用通过一个[Application User
Model ID][app-user-model-id]安装到开始屏幕上。需要注意的是，这不是将你的应用固定到开始屏幕。
* 在Windows 7以及更低的版本中，通知不被支持。不过你可以使用[Tray API][tray-balloon]发送一个"气泡通知"。

此外，通知支持的最大字符长队为250。Windows团队建议通知应该保持在200个字符以下。

### Linux

通知使用`libnotify`发送，它能在任何支持[Desktop Notifications
Specification][notification-spec]的桌面环境中显示，包括 Cinnamon, Enlightenment, Unity,
GNOME, KDE。

### macOS

在macOS系统中，通知是直接转发的，你应该了解[Apple's Human Interface guidelines regarding notifications](https://developer.apple.com/library/mac/documentation/UserExperience/Conceptual/OSXHIGuidelines/NotificationCenter.html)。

注意通知被限制在256个字节以内，如果超出，则会被截断。

## 最近文档 (Windows & macOS)
Windows 和 macOS 提供获取最近文档列表的便捷方式，那就是打开跳转列表或者鱼眼菜单。

跳转列表：
![JumpList][1]

鱼眼菜单：
![Dock Menu][2]

为了增加一个文件到最近文件列表，你可以使用 [app.addRecentDocument][3] API:

```javascript
var app = require('app')
app.addRecentDocument('/Users/USERNAME/Desktop/work.type')
```
或者你也可以使用 [app.clearRecentDocuments][4] API 来清空最近文件列表。
```javascript
app.clearRecentDocuments()
```
## Windows 需注意
为了这个特性在 Windows 上表现正常，你的应用需要被注册成为一种文件类型的句柄，否则，在你注册之前，文件不会出现在跳转列表。你可以在 [Application Registration][5] 里找到任何关于注册事宜的说明。

## macOS 需注意
当一个文件被最近文件列表请求时，`app` 模块里的 `open-file` 事件将会被发出。

## 自定义的鱼眼菜单(macOS)
macOS 可以让开发者定制自己的菜单，通常会包含一些常用特性的快捷方式。
### 菜单中的终端
![Dock menu of Terminal.app][6]

使用 `app.dock.setMenu` API 来设置你的菜单，这仅在 macOS 上可行：

```javascript
const {app, Menu} = require('electron')

const dockMenu = Menu.buildFromTemplate([
  {label: 'New Window', click () { console.log('New Window') }},
  {label: 'New Window with Settings',
  submenu: [
    {label: 'Basic'},
    {label: 'Pro'}
  ]},
  {label: 'New Command...'}
])
app.dock.setMenu(dockMenu)
```

## 用户任务(Windows)
在 Windows，你可以特别定义跳转列表的 `Tasks` 目录的行为，引用 MSDN 的文档：

> Applications define tasks based on both the program's features and the key things a user is expected to do with them. Tasks should be context-free, in that the application does not need to be running for them to work. They should also be the statistically most common actions that a normal user would perform in an application, such as compose an email message or open the calendar in a mail program, create a new document in a word processor, launch an application in a certain mode, or launch one of its subcommands. An application should not clutter the menu with advanced features that standard users won't need or one-time actions such as registration. Do not use tasks for promotional items such as upgrades or special offers.

> It is strongly recommended that the task list be static. It should remain the same regardless of the state or status of the application. While it is possible to vary the list dynamically, you should consider that this could confuse the user who does not expect that portion of the destination list to change.

### IE 的任务
![IE][7]
不同于 macOS 的鱼眼菜单，Windows 上的用户任务表现得更像一个快捷方式，比如当用户点击一个任务，一个程序将会被传入特定的参数并且运行。

你可以使用 [app.setUserTasks][8] API 来设置你的应用中的用户任务：
```javascript
var app = require('app')
app.setUserTasks([
  {
    program: process.execPath,
    arguments: '--new-window',
    iconPath: process.execPath,
    iconIndex: 0,
    title: 'New Window',
    description: 'Create a new window'
  }
])
```
调用 `app.setUserTasks` 并传入空数组就可以清除你的任务列表：
```javascript
app.setUserTasks([])
```
当你的应用关闭时，用户任务会仍然会出现，在你的应用被卸载前，任务指定的图标和程序的路径必须是存在的。

### 缩略图工具栏
在 Windows，你可以在任务栏上添加一个按钮来当作应用的缩略图工具栏。它将提供用户一种用户访问常用窗口的方式，并且不需要恢复或者激活窗口。

在 MSDN，它被如是说：
> This toolbar is simply the familiar standard toolbar common control. It has a maximum of seven buttons. Each button's ID, image, tooltip, and state are defined in a structure, which is then passed to the taskbar. The application can show, enable, disable, or hide buttons from the thumbnail toolbar as required by its current state.

> For example, Windows Media Player might offer standard media transport controls such as play, pause, mute, and stop.

### Windows Media Player 的缩略图工具栏
![Thumbnail toolbar of Windows Media Player][9]
你可以使用 [BrowserWindow.setThumbarButtons][10] 来设置你的应用的缩略图工具栏。
```javascript
var BrowserWindow = require('browser-window')
var path = require('path')
var win = new BrowserWindow({
  width: 800,
  height: 600
})
win.setThumbarButtons([
  {
    tooltip: 'button1',
    icon: path.join(__dirname, 'button1.png'),
    click: function () { console.log('button2 clicked') }
  },
  {
    tooltip: 'button2',
    icon: path.join(__dirname, 'button2.png'),
    flags: ['enabled', 'dismissonclick'],
    click: function () { console.log('button2 clicked.') }
  }
])
```
调用 `BrowserWindow.setThumbarButtons` 并传入空数组即可清空缩略图工具栏：
```javascript
win.setThumbarButtons([])
```

## Unity launcher 快捷方式(Linux)
在 Unity,你可以通过改变 `.desktop` 文件来增加自定义运行器的快捷方式，详情看 [Adding shortcuts to a launcher][11]。
### Audacious 运行器的快捷方式：
![Launcher shortcuts of Audacious][12]

## 任务栏的进度条(Windows & Unity)
在 Windows，进度条可以出现在一个任务栏按钮之上。这可以提供进度信息给用户而不需要用户切换应用窗口。

Unity DE 也具有同样的特性，在运行器上显示进度条。
### 在任务栏上的进度条：
![Progress bar in taskbar button][13]

### 在 Unity 运行器上的进度条
![Progress bar in Unity launcher][14]

给一个窗口设置进度条，你可以调用 [BrowserWindow.setProgressBar][15] API：
```javascript
var window = new BrowserWindow()
window.setProgressBar(0.5)
```
在 macOS，一个窗口可以设置它展示的文件，文件的图标可以出现在标题栏，当用户 Command-Click 或者 Control-Click 标题栏，文件路径弹窗将会出现。
### 展示文件弹窗菜单：
![Represented file popup menu][16]

你可以调用 [BrowserWindow.setRepresentedFilename][17] 和 [BrowserWindow.setDocumentEdited][18] APIs：
```javascript
var window = new BrowserWindow()
window.setRepresentedFilename('/etc/passwd')
window.setDocumentEdited(true)
```

 [1]:https://camo.githubusercontent.com/3310597e01f138b1d687e07aa618c50908a88dec/687474703a2f2f692e6d73646e2e6d6963726f736f66742e636f6d2f64796e696d672f49433432303533382e706e67
  [2]: https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png
  [3]: https://github.com/electron/electron/blob/master/docs-translations/zh-CN/api/app.md
  [4]: https://github.com/electron/electron/blob/master/docs/tutorial/clearrecentdocuments
  [5]: https://msdn.microsoft.com/en-us/library/windows/desktop/ee872121%28v=vs.85%29.aspx
  [6]: https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png
  [7]: https://camo.githubusercontent.com/30154e0cc36acfc968ac9ae076a8f0d6600dd736/687474703a2f2f692e6d73646e2e6d6963726f736f66742e636f6d2f64796e696d672f49433432303533392e706e67
  [8]: https://github.com/electron/electron/blob/master/docs/api/app.md#appsetusertaskstasks
  [9]: https://camo.githubusercontent.com/098cb0f52f27084a80ec6429e51a195df3d8c333/68747470733a2f2f692d6d73646e2e7365632e732d6d7366742e636f6d2f64796e696d672f49433432303534302e706e67
  [10]: https://github.com/electron/electron/blob/master/docs-translations/zh-CN/api/browser-window.md
  [11]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
  [12]: https://camo.githubusercontent.com/b6f54e2bc3206ebf8e08dd029529af9ec84d58ae/68747470733a2f2f68656c702e7562756e74752e636f6d2f636f6d6d756e6974792f556e6974794c61756e6368657273416e644465736b746f7046696c65733f616374696f6e3d41747461636846696c6526646f3d676574267461726765743d73686f7274637574732e706e67
  [13]: https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png
  [14]: https://cloud.githubusercontent.com/assets/639601/5081747/4a0a589e-6f0f-11e4-803f-91594716a546.png
  [15]: https://github.com/electron/electron/blob/master/docs-translations/zh-CN/api/browser-window.md
  [16]: https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png
  [17]: https://github.com/electron/electron/blob/master/docs-translations/zh-CN/api/browser-window.md
  [18]: https://github.com/electron/electron/blob/master/docs-translations/zh-CN/api/browser-window.md

[addrecentdocument]: ../api/app.md#appaddrecentdocumentpath-os-x-windows
[clearrecentdocuments]: ../api/app.md#appclearrecentdocuments-os-x-windows
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks-windows
[setprogressbar]: ../api/browser-window.md#winsetprogressbarprogress
[setoverlayicon]: ../api/browser-window.md#winsetoverlayiconoverlay-description-windows-7
[setrepresentedfilename]: ../api/browser-window.md#winsetrepresentedfilenamefilename-os-x
[setdocumentedited]: ../api/browser-window.md#winsetdocumenteditededited-os-x
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
[setthumbarbuttons]: ../api/browser-window.md#winsetthumbarbuttonsbuttons-windows-7
[tray-balloon]: ../api/tray.md#traydisplayballoonoptions-windows
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
[notification-spec]: https://developer.gnome.org/notification-spec/
