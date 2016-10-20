# 菜单

`menu` 类可以用来创建原生菜单，它可用作应用菜单和
[context 菜单](https://developer.mozilla.org/en-US/docs/Mozilla/Tech/XUL/PopupGuide/ContextMenus).

这个模块是一个主进程的模块，并且可以通过 `remote` 模块给渲染进程调用.

每个菜单有一个或几个菜单项 [menu items](menu-item.md)，并且每个菜单项可以有子菜单.

下面这个例子是在网页(渲染进程)中通过 [remote](remote.md) 模块动态创建的菜单，并且右键显示:

```html
<!-- index.html -->
<script>
const remote = require('electron').remote;
const Menu = remote.Menu;
const MenuItem = remote.MenuItem;

var menu = new Menu();
menu.append(new MenuItem({ label: 'MenuItem1', click: function() { console.log('item 1 clicked'); } }));
menu.append(new MenuItem({ type: 'separator' }));
menu.append(new MenuItem({ label: 'MenuItem2', type: 'checkbox', checked: true }));

window.addEventListener('contextmenu', function (e) {
  e.preventDefault();
  menu.popup(remote.getCurrentWindow());
}, false);
</script>
```

例子，在渲染进程中使用模板api创建应用菜单:

```javascript
var template = [
  {
    label: 'Edit',
    submenu: [
      {
        label: 'Undo',
        accelerator: 'CmdOrCtrl+Z',
        role: 'undo'
      },
      {
        label: 'Redo',
        accelerator: 'Shift+CmdOrCtrl+Z',
        role: 'redo'
      },
      {
        type: 'separator'
      },
      {
        label: 'Cut',
        accelerator: 'CmdOrCtrl+X',
        role: 'cut'
      },
      {
        label: 'Copy',
        accelerator: 'CmdOrCtrl+C',
        role: 'copy'
      },
      {
        label: 'Paste',
        accelerator: 'CmdOrCtrl+V',
        role: 'paste'
      },
      {
        label: 'Select All',
        accelerator: 'CmdOrCtrl+A',
        role: 'selectall'
      }
    ]
  },
  {
    label: 'View',
    submenu: [
      {
        label: 'Reload',
        accelerator: 'CmdOrCtrl+R',
        click: function (item, focusedWindow) {
          if (focusedWindow) focusedWindow.reload()
        }
      },
      {
        label: 'Toggle Full Screen',
        accelerator: (function () {
          return (process.platform === 'darwin') ? 'Ctrl+Command+F' : 'F11'
        })(),
        click: function (item, focusedWindow) {
          if (focusedWindow) focusedWindow.setFullScreen(!focusedWindow.isFullScreen())
        }
      },
      {
        label: 'Toggle Developer Tools',
        accelerator: (function () {
          if (process.platform === 'darwin') {
            return 'Alt+Command+I'
          } else {
            return 'Ctrl+Shift+I'
          }
        })(),
        click: function (item, focusedWindow) {
          if (focusedWindow) focusedWindow.toggleDevTools()
        }
      }
    ]
  },
  {
    label: 'Window',
    role: 'window',
    submenu: [
      {
        label: 'Minimize',
        accelerator: 'CmdOrCtrl+M',
        role: 'minimize'
      },
      {
        label: 'Close',
        accelerator: 'CmdOrCtrl+W',
        role: 'close'
      }
    ]
  },
  {
    label: 'Help',
    role: 'help',
    submenu: [
      {
        label: 'Learn More',
        click: function () { require('electron').shell.openExternal('http://electron.atom.io') }
      }
    ]
  }
]

if (process.platform === 'darwin') {
  var name = require('electron').remote.app.getName()
  template.unshift({
    label: name,
    submenu: [
      {
        label: 'About ' + name,
        role: 'about'
      },
      {
        type: 'separator'
      },
      {
        label: 'Services',
        role: 'services',
        submenu: []
      },
      {
        type: 'separator'
      },
      {
        label: 'Hide ' + name,
        accelerator: 'Command+H',
        role: 'hide'
      },
      {
        label: 'Hide Others',
        accelerator: 'Command+Alt+H',
        role: 'hideothers'
      },
      {
        label: 'Show All',
        role: 'unhide'
      },
      {
        type: 'separator'
      },
      {
        label: 'Quit',
        accelerator: 'Command+Q',
        click: function () { app.quit() }
      }
    ]
  })
  // Window menu.
  template[3].submenu.push(
    {
      type: 'separator'
    },
    {
      label: 'Bring All to Front',
      role: 'front'
    }
  )
}

var menu = Menu.buildFromTemplate(template)
Menu.setApplicationMenu(menu)
```

## 类: Menu

### `new Menu()`

创建一个新的菜单.

## 方法

`菜单` 类有如下方法:

### `Menu.setApplicationMenu(menu)`

* `menu` Menu

在 macOS 上设置应用菜单 `menu` .
在windows 和 linux，是为每个窗口都在其顶部设置菜单 `menu`.

### `Menu.sendActionToFirstResponder(action)` _macOS_

* `action` String

发送 `action` 给应用的第一个响应器.这个用来模仿 Cocoa 菜单的默认行为，通常你只需要使用 `MenuItem` 的属性 `role`.

查看更多 macOS 的原生 action [macOS Cocoa Event Handling Guide](https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/EventOverview/EventArchitecture/EventArchitecture.html#//apple_ref/doc/uid/10000060i-CH3-SW7) .

### `Menu.buildFromTemplate(template)`

* `template` Array

一般来说，`template` 只是用来创建 [MenuItem](menu-item.md) 的数组 `参数` .

你也可以向 `template` 元素添加其它东西，并且他们会变成已经有的菜单项的属性.

##实例方法

`menu` 对象有如下实例方法

### `menu.popup([browserWindow, x, y, positioningItem])`

* `browserWindow` BrowserWindow (可选) - 默认为 `null`.
* `x` Number (可选) - 默认为 -1.
* `y` Number (**必须** 如果x设置了) - 默认为 -1.
* `positioningItem` Number (可选) _macOS_ - 在指定坐标鼠标位置下面的菜单项的索引. 默认为
  -1.

在 `browserWindow` 中弹出 context menu .你可以选择性地提供指定的 `x, y` 来设置菜单应该放在哪里,否则它将默认地放在当前鼠标的位置.

### `menu.append(menuItem)`

* `menuItem` MenuItem

添加菜单项.

### `menu.insert(pos, menuItem)`

* `pos` Integer
* `menuItem` MenuItem

在制定位置添加菜单项.

### `menu.items()`

获取一个菜单项数组.

## macOS Application 上的菜单的注意事项

相对于windows 和 linux, macOS 上的应用菜单是完全不同的style，这里是一些注意事项，来让你的菜单项更原生化.

### 标准菜单

在 macOS 上，有很多系统定义的标准菜单，例如  `Services` and
`Windows` 菜单.为了让你的应用更标准化，你可以为你的菜单的 `role` 设置值，然后 electron 将会识别他们并且让你的菜单更标准:

* `window`
* `help`
* `services`

### 标准菜单项行为

macOS 为一些菜单项提供了标准的行为方法，例如 `About xxx`,
`Hide xxx`, and `Hide Others`. 为了让你的菜单项的行为更标准化，你应该为菜单项设置 `role` 属性.

### 主菜单名

在 macOS ，无论你设置的什么标签，应用菜单的第一个菜单项的标签始终未你的应用名字.想要改变它的话，你必须通过修改应用绑定的 `Info.plist` 文件来修改应用名字.更多信息参考[About Information
Property List Files][AboutInformationPropertyListFiles] .

## 为制定浏览器窗口设置菜单 (*Linux* *Windows*)

浏览器窗口的[`setMenu` 方法][setMenu] 能够设置菜单为特定浏览器窗口的类型.

## 菜单项位置

当通过 `Menu.buildFromTemplate` 创建菜单的时候，你可以使用 `position` and `id` 来放置菜单项.

`MenuItem` 的属性  `position` 格式为 `[placement]=[id]`，`placement` 取值为 `before`, `after`, 或 `endof` 和 `id`， `id` 是菜单已经存在的菜单项的唯一 ID:

* `before` - 在对应引用id菜单项之前插入. 如果引用的菜单项不存在，则将其插在菜单末尾.
* `after` - 在对应引用id菜单项之后插入. 如果引用的菜单项不存在，则将其插在菜单末尾.
* `endof` - 在逻辑上包含对应引用id菜单项的集合末尾插入. 如果引用的菜单项不存在, 则将使用给定的id创建一个新的集合，并且这个菜单项将插入.

当一个菜档项插入成功了，所有的没有插入的菜单项将一个接一个地在后面插入.所以如果你想在同一个位置插入一组菜单项，只需要为这组菜单项的第一个指定位置.

### 例子

模板:

```javascript
[
  {label: '4', id: '4'},
  {label: '5', id: '5'},
  {label: '1', id: '1', position: 'before=4'},
  {label: '2', id: '2'},
  {label: '3', id: '3'}
]
```

菜单:

```
- 1
- 2
- 3
- 4
- 5
```

模板:

```javascript
[
  {label: 'a', position: 'endof=letters'},
  {label: '1', position: 'endof=numbers'},
  {label: 'b', position: 'endof=letters'},
  {label: '2', position: 'endof=numbers'},
  {label: 'c', position: 'endof=letters'},
  {label: '3', position: 'endof=numbers'}
]
```

菜单:

```
- ---
- a
- b
- c
- ---
- 1
- 2
- 3
```

[AboutInformationPropertyListFiles]: https://developer.apple.com/library/ios/documentation/general/Reference/InfoPlistKeyReference/Articles/AboutInformationPropertyListFiles.html
[setMenu]:
https://github.com/electron/electron/blob/master/docs/api/browser-window.md#winsetmenumenu-linux-windows
