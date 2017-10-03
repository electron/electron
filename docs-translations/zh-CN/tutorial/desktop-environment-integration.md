# 桌面环境集成
不同的操作系统在各自的桌面应用上提供了不同的特性。例如，在 windows 上应用曾经打开的文件会出现在任务栏的跳转列表，在 Mac 上，应用可以把自定义菜单放在鱼眼菜单上。

本章将会说明怎样使用 Electron APIs 把你的应用和桌面环境集成到一块。

## Notifications (Windows, Linux, macOS)

这三个操作系统都为用户提供了发送通知的方法。Electron 让开发人员通过
[HTML5 Notification API](https://notifications.spec.whatwg.org/)
便利的去发送通知，用操作系统自带的通知 APIs 去显示。

**Note:** 因为这是一个 HTML5 API，所以只在渲染进程中起作用.

```javascript
let myNotification = new Notification('Title', {
  body: 'Lorem Ipsum Dolor Sit Amet'
})

myNotification.onclick = () => {
  console.log('Notification clicked')
}
```

尽管代码和用户体验在不同的操作系统中基本相同，但还是有一些差异。

### Windows

* 在 Windows 10 上, 通知"可以工作"。
* 在 Windows 8.1 和 Windows 8 系统下，你需要将你的应用通过一个[Application User
Model ID][app-user-model-id]安装到开始屏幕上。需要注意的是，这不是将你的应用固定到开始屏幕。
* 在 Windows 7 以及更低的版本中，通知不被支持。不过你可以使用 [Tray API][tray-balloon] 发送一个"气泡通知"。

此外，通知支持的最大字符长度为250。Windows 团队建议通知应该保持在200个字符以下。

### Linux

通知使用 `libnotify` 发送，它能在任何支持[Desktop Notifications
Specification][notification-spec]的桌面环境中显示，包括 Cinnamon, Enlightenment, Unity,
GNOME, KDE。

### macOS

在 macOS 系统中，通知是直接转发的，你应该了解 [Apple's Human Interface guidelines regarding notifications](https://developer.apple.com/library/mac/documentation/UserExperience/Conceptual/OSXHIGuidelines/NotificationCenter.html)。

注意通知被限制在256个字节以内，如果超出，则会被截断。

## 最近文档 (Windows & macOS)

Windows 和 macOS 提供获取最近文档列表的便捷方式，那就是打开跳转列表或者鱼眼菜单。

__跳转列表：__

![JumpList Recent Files](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__鱼眼菜单：__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

为了增加一个文件到最近文件列表，你可以使用 [app.addRecentDocument][3] API:

```javascript
const {app} = require('electron')
app.addRecentDocument('/Users/USERNAME/Desktop/work.type')
```

或者你也可以使用 [app.clearRecentDocuments][4] API 来清空最近文件列表。

```javascript
const {app} = require('electron')
app.clearRecentDocuments()
```

## Windows 需注意
为了这个特性在 Windows 上表现正常，你的应用需要被注册成为一种文件类型的句柄，否则，在你注册之前，文件不会出现在跳转列表。你可以在 [Application Registration][app-registration] 里找到任何关于注册事宜的说明。

当用户点击从“跳转列表”点击一个文件，你的应用程序的新实例
将以添加为命令行参数的文件的路径启动。

## macOS 需注意

当一个文件被最近文件列表请求时，`app` 模块里的 `open-file` 事件将会被发出。

## 自定义的鱼眼菜单(macOS)

macOS 可以让开发者定制自己的菜单，通常会包含一些常用特性的快捷方式。

### 菜单中的终端

__Dock menu of Terminal.app:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

使用 `app.dock.setMenu` API 来设置你的菜单，这仅在 macOS 上可行：

```javascript
const {app, Menu} = require('electron')

const dockMenu = Menu.buildFromTemplate([
  {label: 'New Window', click () { console.log('New Window') }},
  {label: 'New Window with Settings',
    submenu: [
      {label: 'Basic'},
      {label: 'Pro'}
    ]
  },
  {label: 'New Command...'}
])
app.dock.setMenu(dockMenu)
```

## 用户任务(Windows)

在 Windows，你可以特别定义跳转列表的 `Tasks` 目录的行为，引用 MSDN 的文档：

> Applications define tasks based on both the program's features and the key things a user is expected to do with them. Tasks should be context-free, in that the application does not need to be running for them to work. They should also be the statistically most common actions that a normal user would perform in an application, such as compose an email message or open the calendar in a mail program, create a new document in a word processor, launch an application in a certain mode, or launch one of its subcommands. An application should not clutter the menu with advanced features that standard users won't need or one-time actions such as registration. Do not use tasks for promotional items such as upgrades or special offers.

> It is strongly recommended that the task list be static. It should remain the same regardless of the state or status of the application. While it is possible to vary the list dynamically, you should consider that this could confuse the user who does not expect that portion of the destination list to change.

### IE 的任务

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

不同于 macOS 的鱼眼菜单，Windows 上的用户任务表现得更像一个快捷方式，比如当用户点击一个任务，一个程序将会被传入特定的参数并且运行。

你可以使用 [app.setUserTasks][setusertaskstasks] API 来设置你的应用中的用户任务：

```javascript
const {app} = require('electron')
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
const {app} = require('electron')
app.setUserTasks([])
```

当你的应用关闭时，用户任务会仍然会出现，在你的应用被卸载前，任务指定的图标和程序的路径必须是存在的。

### 缩略图工具栏

在 Windows，你可以在任务栏上添加一个按钮来当作应用的缩略图工具栏。它将提供用户一种用户访问常用窗口的方式，并且不需要恢复或者激活窗口。

在 MSDN，它被如是说：
> This toolbar is simply the familiar standard toolbar common control. It has a maximum of seven buttons. Each button's ID, image, tooltip, and state are defined in a structure, which is then passed to the taskbar. The application can show, enable, disable, or hide buttons from the thumbnail toolbar as required by its current state.

> For example, Windows Media Player might offer standard media transport controls such as play, pause, mute, and stop.

### Windows Media Player 的缩略图工具栏

![Thumbnail toolbar of Windows Media Player](https://i-msdn.sec.s-msft.com/dynimg/IC420540.png)

你可以使用 [BrowserWindow.setThumbarButtons][setthumbarbuttons] 来设置你的应用的缩略图工具栏。

```javascript
const {BrowserWindow} = require('electron')
const path = require('path')

let win = new BrowserWindow({
  width: 800,
  height: 600
})

win.setThumbarButtons([
  {
    tooltip: 'button1',
    icon: path.join(__dirname, 'button1.png'),
    click () { console.log('button1 clicked') }
  },
  {
    tooltip: 'button2',
    icon: path.join(__dirname, 'button2.png'),
    flags: ['enabled', 'dismissonclick'],
    click () { console.log('button2 clicked.') }
  }
])
```

调用 `BrowserWindow.setThumbarButtons` 并传入空数组即可清空缩略图工具栏：

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.setThumbarButtons([])
```

## Unity launcher 快捷方式(Linux)

在 Unity,你可以通过改变 `.desktop` 文件来增加自定义运行器的快捷方式，详情看 [Adding Shortcuts to a Launcher][unity-launcher]。

__Launcher shortcuts of Audacious:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

### Audacious 运行器的快捷方式：

![Launcher shortcuts of Audacious][12]

## 任务栏的进度条(Windows, macOS, Unity)

在 Windows，进度条可以出现在一个任务栏按钮之上。这可以提供进度信息给用户而不需要用户切换应用窗口。

在 macOS，进度条将显示为 dock 图标的一部分。

Unity DE 也具有同样的特性，在运行器上显示进度条。

__Progress bar in taskbar button:__

![Taskbar Progress Bar](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

给一个窗口设置进度条，你可以调用 [BrowserWindow.setProgressBar][setprogressbar] API：

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.setProgressBar(0.5)
```


## 任务栏中的叠加层图标 (Windows)

在 Windows，任务栏按钮可以使用小型叠加层显示应用程序
状态，引用 MSDN 的文档：

> Icon overlays serve as a contextual notification of status, and are intended
> to negate the need for a separate notification area status icon to communicate
> that information to the user. For instance, the new mail status in Microsoft
> Outlook, currently shown in the notification area, can now be indicated
> through an overlay on the taskbar button. Again, you must decide during your
> development cycle which method is best for your application. Overlay icons are
> intended to supply important, long-standing status or notifications such as
> network status, messenger status, or new mail. The user should not be
> presented with constantly changing overlays or animations.

__Overlay on taskbar button:__

![Overlay on taskbar button](https://i-msdn.sec.s-msft.com/dynimg/IC420441.png)

要设置窗口的叠加层图标，可以使用
[BrowserWindow.setOverlayIcon][setoverlayicon] API:

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.setOverlayIcon('path/to/overlay.png', 'Description for overlay')
```

## 突出显示框架 (Windows)

在 Windows，你可以突出显示任务栏按钮，以获得用户的关注。
这类似于在 macOS 上弹出 dock 图标。
在 MSDN，它如是说：

> Typically, a window is flashed to inform the user that the window requires
> attention but that it does not currently have the keyboard focus.

要在 BrowserWindow 的任务栏按钮突出显示，可以使用
[BrowserWindow.flashFrame][flashframe] API:

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.once('focus', () => win.flashFrame(false))
win.flashFrame(true)
```

不要忘记在调用 `flashFrame` 方法后，设置 `false` 来关闭突出显示。 在
上面的例子，它是在窗口进入焦点时调用的，但你可能会
使用超时或某些其他事件来禁用它。

## 展示文件窗口 (macOS)

在 macOS，一个窗口可以设置它展示的文件，文件的图标可以出现在标题栏，当用户 Command-Click 或者 Control-Click 标题栏，文件路径弹窗将会出现。

您还可以设置窗口的编辑状态，以便文件图标可以指示
该窗口中的文档是否已修改。

__Represented file popup menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png" height="232" width="663" >

要设置展示文件窗口，可以使用
[BrowserWindow.setRepresentedFilename][setrepresentedfilename] 和
[BrowserWindow.setDocumentEdited][setdocumentedited] APIs：

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.setRepresentedFilename('/etc/passwd')
win.setDocumentEdited(true)
```

## 将文件拖出窗口

对于某些操作文件的应用程序，
将文件从 Electron 拖动到其他应用程序是很重要的能力。要在 app 的实现此功能
，你需要在 `ondragstart` 事件上调用 `webContents.startDrag(item)` API。

在网页端：

```html
<a href="#" id="drag">item</a>
<script type="text/javascript" charset="utf-8">
  document.getElementById('drag').ondragstart = (event) => {
    event.preventDefault()
    ipcRenderer.send('ondragstart', '/path/to/item')
  }
</script>
```

在主进程：

```javascript
const {ipcMain} = require('electron')
ipcMain.on('ondragstart', (event, filePath) => {
  event.sender.startDrag({
    file: filePath,
    icon: '/path/to/icon.png'
  })
})
```

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
[flashframe]: ../api/browser-window.md#winflashframeflag
