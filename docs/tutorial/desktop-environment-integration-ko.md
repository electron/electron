# 데스크톱 환경 통합

Different operating systems provide different features on integrating desktop
applications into their desktop environments. For example, on Windows
applications can put shortcuts in the JumpList of task bar, and on Mac
applications can put a custom menu in the dock menu.

This guide explains how to integrate your application into those desktop
environments with Electron APIs.

## 최근 사용한 문서 (Windows & OS X)

Windows and OS X provide easy access to a list of recent documents opened by
the application via JumpList and dock menu.

__JumpList:__

![JumpList Recent Files](http://i.msdn.microsoft.com/dynimg/IC420538.png)

__Application dock menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069610/2aa80758-6e97-11e4-8cfb-c1a414a10774.png" height="353" width="428" >

To add a file to recent documents, you can use
[app.addRecentDocument][addrecentdocument] API:

```javascript
var app = require('app');
app.addRecentDocument('/Users/USERNAME/Desktop/work.type');
```

And you can use [app.clearRecentDocuments](clearrecentdocuments) API to empty
the recent documents list:

```javascript
app.clearRecentDocuments();
```

### Windows에서 주의할 점

In order to be able to use this feature on Windows, your application has to be
registered as a handler of the file type of the document, otherwise the file
won't appear in JumpList even after you have added it. You can find everything
on registering your application in [Application Registration][app-registration].

When a user clicks a file from JumpList, a new instance of your application will
be started with the path of the file added as a command line argument.

### OS X에서 주의할 점

When a file is requested from the recent documents menu, the `open-file` event
of `app` module would be emitted for it.

## 커스텀 독 메뉴 (OS X)

OS X enables developers to specify a custom menu for the dock, which usually
contains some shortcuts for commonly used features of your application:

__Dock menu of Terminal.app:__

<img src="https://cloud.githubusercontent.com/assets/639601/5069962/6032658a-6e9c-11e4-9953-aa84006bdfff.png" height="354" width="341" >

To set your custom dock menu, you can use the `app.dock.setMenu` API, which is
only available on OS X:

```javascript
var app = require('app');
var Menu = require('menu');
var dockMenu = Menu.buildFromTemplate([
  { label: 'New Window', click: function() { console.log('New Window'); } },
  { label: 'New Window with Settings', submenu: [
    { label: 'Basic' },
    { label: 'Pro'}
  ]},
  { label: 'New Command...'}
]);
app.dock.setMenu(dockMenu);
```

## 사용자 작업 (Windows)

On Windows you can specify custom actions in the `Tasks` category of JumpList,
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

Unlike the dock menu in OS X which is a real menu, user tasks in Windows work
like application shortcuts that when user clicks a task, a program would be
executed with specified arguments.

To set user tasks for your application, you can use
[app.setUserTasks][setusertaskstasks] API:

```javascript
var app = require('app');
app.setUserTasks([
  {
    program: process.execPath,
    arguments: '--new-window',
    iconPath: process.execPath,
    iconIndex: 0,
    title: 'New Window',
    description: 'Create a new window'
  }
]);
```

To clean your tasks list, just call `app.setUserTasks` with empty array:

```javascript
app.setUserTasks([]);
```

The user tasks will still show even after your application closes, so the icon
and program path specified for a task should exist until your application is
uninstalled.

## Unity 런처 숏컷 기능 (Linux)

In Unity, you can add custom entries to its launcher via modifying `.desktop`
file, see [Adding shortcuts to a launcher][unity-launcher].

__Launcher shortcuts of Audacious:__

![audacious](https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles?action=AttachFile&do=get&target=shortcuts.png)

## Taskbar progress 기능 (Windows & Unity)

On Windows, a taskbar button can be used to display a progress bar. This enables
a window to provide progress information to the user without the user having to
switch to the window itself.

The Unity DE also has a similar feature that allows you to specify the progress
bar in the launcher.

__Progress bar in taskbar button:__

![Taskbar Progress Bar](https://cloud.githubusercontent.com/assets/639601/5081682/16691fda-6f0e-11e4-9676-49b6418f1264.png)

__Progress bar in Unity launcher:__

![Unity Launcher](https://cloud.githubusercontent.com/assets/639601/5081747/4a0a589e-6f0f-11e4-803f-91594716a546.png)

To set the progress bar for a Window, you can use the
[BrowserWindow.setProgressBar][setprogressbar] API:

```javascript
var window = new BrowserWindow({...});
window.setProgressBar(0.5);
```

## 윈도우 파일 제시 (OS X)

On OS X a window can set its represented file, so the file's icon can show in
the title bar, and when users Command-Click or Control-Click on the tile a path
popup will show.

You can also set the edited state of a window so that the file icon can indicate
whether the document in this window has been modified.

__Represented file popup menu:__

<img src="https://cloud.githubusercontent.com/assets/639601/5082061/670a949a-6f14-11e4-987a-9aaa04b23c1d.png" height="232" width="663" >

To set the represented file of window, you can use the
[BrowserWindow.setRepresentedFilename][setrepresentedfilename] and
[BrowserWindow.setDocumentEdited][setdocumentedited] APIs:

```javascript
var window = new BrowserWindow({...});
window.setRepresentedFilename('/etc/passwd');
window.setDocumentEdited(true);
```

[addrecentdocument]: ../api/app-ko.md#appaddrecentdocumentpath
[clearrecentdocuments]: ../api/app-ko.md#appclearrecentdocuments
[setusertaskstasks]: ../api/app-ko.md#appsetusertaskstasks
[setprogressbar]: ../api/browser-window-ko.md#browserwindowsetprogressbarprogress
[setrepresentedfilename]: ../api/browser-window-ko.md#browserwindowsetrepresentedfilenamefilename
[setdocumentedited]: ../api/browser-window-ko.md#browserwindowsetdocumenteditededited
[app-registration]: http://msdn.microsoft.com/en-us/library/windows/desktop/ee872121(v=vs.85).aspx
[unity-launcher]: https://help.ubuntu.com/community/UnityLaunchersAndDesktopFiles#Adding_shortcuts_to_a_launcher
