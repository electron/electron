# Tray

用一个 `Tray` 来表示一个图标,这个图标处于正在运行的系统的通知区 ，通常被添加到一个 context menu 上.

```javascript
const electron = require('electron')
const app = electron.app
const Menu = electron.Menu
const Tray = electron.Tray

var appIcon = null
app.on('ready', function () {
  appIcon = new Tray('/path/to/my/icon')
  var contextMenu = Menu.buildFromTemplate([
    { label: 'Item1', type: 'radio' },
    { label: 'Item2', type: 'radio' },
    { label: 'Item3', type: 'radio', checked: true },
    { label: 'Item4', type: 'radio' }
  ])
  appIcon.setToolTip('This is my application.')
  appIcon.setContextMenu(contextMenu)
})

```

__平台限制:__

* 在 Linux， 如果支持应用指示器则使用它，否则使用 `GtkStatusIcon` 代替.
* 在 Linux ，配置了只有有了应用指示器的支持, 你必须安装 `libappindicator1` 来让 tray icon 执行.
* 应用指示器只有在它拥有 context menu 时才会显示.
* 当在linux 上使用了应用指示器，将忽略点击事件.
* 在 Linux，为了让单独的 `MenuItem` 起效，需要再次调用 `setContextMenu` .例如:

```javascript
contextMenu.items[2].checked = false
appIcon.setContextMenu(contextMenu)
```
如果想在所有平台保持完全相同的行为，不应该依赖点击事件，而是一直将一个 context menu 添加到 tray icon.

## Class: Tray

`Tray` 是一个 [事件发出者][event-emitter].

### `new Tray(image)`

* `image` [NativeImage](native-image.md)

创建一个与 `image` 相关的 icon.

## 事件

`Tray` 模块可发出下列事件:

**注意:** 一些事件只能在特定的os中运行，已经标明.

### Event: 'click'

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - tray icon 的 bounds.
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

当tray icon被点击的时候发出事件.

__注意:__ `bounds` 只在 macOS 和 Windows 上起效.

### Event: 'right-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - tray icon 的 bounds.
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

当tray icon被鼠标右键点击的时候发出事件.

### Event: 'double-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` Object - tray icon 的 bounds.
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

当tray icon被双击的时候发出事件.

### Event: 'balloon-show' _Windows_

当tray 气泡显示的时候发出事件.

### Event: 'balloon-click' _Windows_

当tray 气泡被点击的时候发出事件.

### Event: 'balloon-closed' _Windows_

当tray 气泡关闭的时候发出事件，因为超时或人为关闭.

### Event: 'drop' _macOS_

当tray icon上的任何可拖动项被删除的时候发出事件.

### Event: 'drop-files' _macOS_

* `event`
* `files` Array - 已删除文件的路径.

当tray icon上的可拖动文件被删除的时候发出事件.

### Event: 'drag-enter' _macOS_

当一个拖动操作进入tray icon的时候发出事件.

### Event: 'drag-leave' _macOS_

当一个拖动操作离开tray icon的时候发出事件.
Emitted when a drag operation exits the tray icon.

### Event: 'drag-end' _macOS_

当一个拖动操作在tray icon上或其它地方停止拖动的时候发出事件.

## 方法

`Tray` 模块有以下方法:

**Note:** 一些方法只能在特定的os中运行，已经标明.

### `Tray.destroy()`

立刻删除 tray icon.

### `Tray.setImage(image)`

* `image` [NativeImage](native-image.md)

让 `image` 与 tray icon 关联起来.

### `Tray.setPressedImage(image)` _macOS_

* `image` [NativeImage](native-image.md)

当在 macOS 上按压 tray icon 的时候， 让 `image` 与 tray icon 关联起来.

### `Tray.setToolTip(toolTip)`

* `toolTip` String

为 tray icon 设置 hover text.

### `Tray.setTitle(title)` _macOS_

* `title` String

在状态栏沿着 tray icon 设置标题.

### `Tray.setHighlightMode(highlight)` _macOS_

* `highlight` Boolean

当 tray icon 被点击的时候，是否设置它的背景色变为高亮(blue).默认为 true.

### `Tray.displayBalloon(options)` _Windows_

* `options` Object
  * `icon` [NativeImage](native-image.md)
  * `title` String
  * `content` String

展示一个 tray balloon.

### `Tray.popUpContextMenu([menu, position])` _macOS_ _Windows_

* `menu` Menu (optional)
* `position` Object (可选) - 上托位置.
  * `x` Integer
  * `y` Integer

从 tray icon 上托出 context menu . 当划过 `menu` 的时候， `menu` 显示，代替 tray 的 context menu .

`position` 只在 windows 上可用，默认为 (0, 0) .

### `Tray.setContextMenu(menu)`

* `menu` Menu

为这个 icon 设置 context menu .

[event-emitter]: http://nodejs.org/api/events.html#events_class_events_eventemitter