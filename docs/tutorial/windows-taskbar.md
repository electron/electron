# Windows Taskbar

Electron has APIs to configure the app's icon in the Windows taskbar. Supported
are the [creation of a `JumpList`](#jumplist),
[custom thumbnails and toolbars](#thumbnail-toolbars),
[icon overlays](#icon-overlays-in-taskbar), and the so-called
["Flash Frame" effect](#flash-frame), but
Electron also uses the app's dock icon to implement cross-platform features
like [recent documents][recent-documents] and
[application progress][progress-bar].

## JumpList

Windows allows apps to define a custom context menu that shows up when users
right-click the app's icon in the task bar. That context menu is called
`JumpList`. You specify custom actions in the `Tasks` category of JumpList,
as quoted from MSDN:

> Applications define tasks based on both the program's features and the key
> things a user is expected to do with them. Tasks should be context-free, in
> that the application does not need to be running for them to work. They
> should also be the statistically most common actions that a normal user would
> perform in an application, such as compose an email message or open the
> calendar in a mail program, create a new document in a word processor, launch
> an application in a certain mode, or launch one of its subcommands. An
> application should not clutter the menu with advanced features that standard
> users won't need or one-time actions such as registration. Do not use tasks
> for promotional items such as upgrades or special offers.
>
> It is strongly recommended that the task list be static. It should remain the
> same regardless of the state or status of the application. While it is
> possible to vary the list dynamically, you should consider that this could
> confuse the user who does not expect that portion of the destination list to
> change.

__Tasks of Internet Explorer:__

![IE](http://i.msdn.microsoft.com/dynimg/IC420539.png)

Unlike the dock menu in macOS which is a real menu, user tasks in Windows work
like application shortcuts such that when user clicks a task, a program will be
executed with specified arguments.

To set user tasks for your application, you can use
[app.setUserTasks][setusertaskstasks] API:

```javascript
const { app } = require('electron')
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

To clean your tasks list, call `app.setUserTasks` with an empty array:

```javascript
const { app } = require('electron')
app.setUserTasks([])
```

The user tasks will still show even after your application closes, so the icon
and program path specified for a task should exist until your application is
uninstalled.


## Thumbnail Toolbars

On Windows you can add a thumbnail toolbar with specified buttons in a taskbar
layout of an application window. It provides users a way to access to a
particular window's command without restoring or activating the window.

From MSDN, it's illustrated:

> This toolbar is the familiar standard toolbar common control. It has a
> maximum of seven buttons. Each button's ID, image, tooltip, and state are defined
> in a structure, which is then passed to the taskbar. The application can show,
> enable, disable, or hide buttons from the thumbnail toolbar as required by its
> current state.
>
> For example, Windows Media Player might offer standard media transport controls
> such as play, pause, mute, and stop.

__Thumbnail toolbar of Windows Media Player:__

![player](https://i-msdn.sec.s-msft.com/dynimg/IC420540.png)

You can use [BrowserWindow.setThumbarButtons][setthumbarbuttons] to set
thumbnail toolbar in your application:

```javascript
const { BrowserWindow } = require('electron')
const path = require('path')

const win = new BrowserWindow()

win.setThumbarButtons([
  {
    tooltip: 'button1',
    icon: path.join(__dirname, 'button1.png'),
    click () { console.log('button1 clicked') }
  }, {
    tooltip: 'button2',
    icon: path.join(__dirname, 'button2.png'),
    flags: ['enabled', 'dismissonclick'],
    click () { console.log('button2 clicked.') }
  }
])
```

To clean thumbnail toolbar buttons, just call `BrowserWindow.setThumbarButtons`
with an empty array:

```javascript
const { BrowserWindow } = require('electron')

const win = new BrowserWindow()
win.setThumbarButtons([])
```


## Icon Overlays in Taskbar

On Windows a taskbar button can use a small overlay to display application
status, as quoted from MSDN:

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

To set the overlay icon for a window, you can use the
[BrowserWindow.setOverlayIcon][setoverlayicon] API:

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow()
win.setOverlayIcon('path/to/overlay.png', 'Description for overlay')
```


## Flash Frame

On Windows you can highlight the taskbar button to get the user's attention.
This is similar to bouncing the dock icon on macOS.
From the MSDN reference documentation:

> Typically, a window is flashed to inform the user that the window requires
> attention but that it does not currently have the keyboard focus.

To flash the BrowserWindow taskbar button, you can use the
[BrowserWindow.flashFrame][flashframe] API:

```javascript
const { BrowserWindow } = require('electron')
let win = new BrowserWindow()
win.once('focus', () => win.flashFrame(false))
win.flashFrame(true)
```

Don't forget to call the `flashFrame` method with `false` to turn off the flash. In
the above example, it is called when the window comes into focus, but you might
use a timeout or some other event to disable it.

[setthumbarbuttons]: ../api/browser-window.md#winsetthumbarbuttonsbuttons-windows
[setusertaskstasks]: ../api/app.md#appsetusertaskstasks-windows
[setoverlayicon]: ../api/browser-window.md#winsetoverlayiconoverlay-description-windows
[flashframe]: ../api/browser-window.md#winflashframeflag
[recent-documents]: ./recent-documents.md
[progress-bar]: ./progress-bar.md
