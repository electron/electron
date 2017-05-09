# Class：Tray

> 将图标和上下文菜单添加到系统的通知区域。

可使用的进程: [主进程](../tutorial/quick-start.md#main-process)

`Tray` 是一个 [事件发出者][event-emitter]。

```javascript
const {app, Menu, Tray} = require('electron')

let tray = null
app.on('ready', () => {
  tray = new Tray('/path/to/my/icon')
  const contextMenu = Menu.buildFromTemplate([
    {label: 'Item1', type: 'radio'},
    {label: 'Item2', type: 'radio'},
    {label: 'Item3', type: 'radio', checked: true},
    {label: 'Item4', type: 'radio'}
  ])
  tray.setToolTip('This is my application.')
  tray.setContextMenu(contextMenu)
})
```

__平台限制:__

* 在 Linux， 如果支持应用指示器则使用它，否则使用 `GtkStatusIcon` 代替。
* 在 Linux ，配置了只有有了应用指示器的支持, 你必须安装 `libappindicator1` 来让 tray icon 执行。
* 应用指示器只有在它拥有 context menu 时才会显示。
* 当在linux 上使用了应用指示器，将忽略点击事件。
* 在 Linux，为了让单独的 `MenuItem` 起效，需要再次调用 `setContextMenu` 。例如：

```javascript
const {app, Menu, Tray} = require('electron')

let appIcon = null
app.on('ready', () => {
  appIcon = new Tray('/path/to/my/icon')
  const contextMenu = Menu.buildFromTemplate([
    {label: 'Item1', type: 'radio'},
    {label: 'Item2', type: 'radio'}
  ])

  // Make a change to the context menu
  contextMenu.items[1].checked = false

  // Call this again for Linux because we modified the context menu
  appIcon.setContextMenu(contextMenu)
})
```

如果想在所有平台保持完全相同的行为，不应该依赖点击事件，而是一直将一个 context menu 添加到 tray icon。

### `new Tray(image)`

* `image` ([NativeImage](native-image.md) | String)

创建一个与 `image` 相关的 icon。

### Instance Events

`Tray` 模块可发出下列事件：

#### Event: 'click'

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` [Rectangle](structures/rectangle.md) - tray icon 的 bounds

当 tray icon 被点击的时候发出事件。

#### Event: 'right-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` [Rectangle](structures/rectangle.md) - tray icon 的 bounds

当 tray icon 被鼠标右键点击的时候发出事件。

#### Event: 'double-click' _macOS_ _Windows_

* `event` Event
  * `altKey` Boolean
  * `shiftKey` Boolean
  * `ctrlKey` Boolean
  * `metaKey` Boolean
* `bounds` [Rectangle](structures/rectangle.md) - tray icon 的 bounds

当 tray icon 被双击的时候发出事件。

#### Event: 'balloon-show' _Windows_

当 tray 气泡显示的时候发出事件。

#### Event: 'balloon-click' _Windows_

当 tray 气泡被点击的时候发出事件。

#### Event: 'balloon-closed' _Windows_

当 tray 气泡关闭的时候发出事件，因为超时或人为关闭。

#### Event: 'drop' _macOS_

当 tray icon 上的任何可拖动项被删除的时候发出事件。

#### Event: 'drop-files' _macOS_

* `event`
* `files` Array - 已删除文件的路径.

当 tray icon 上的可拖动文件被删除的时候发出事件。

#### Event: 'drag-enter' _macOS_

当一个拖动操作进入 tray icon 的时候发出事件。

#### Event: 'drag-leave' _macOS_

当一个拖动操作离开 tray icon 的时候发出事件。

#### Event: 'drag-end' _macOS_

当一个拖动操作在 tray icon 上或其它地方停止拖动的时候发出事件。

### 方法

`Tray` 模块有以下方法：

#### `tray.destroy()`

立刻删除 tray icon。

#### `tray.setImage(image)`

* `image` ([NativeImage](native-image.md) | String)

让 `image` 与 tray icon 关联起来。

#### `tray.setPressedImage(image)` _macOS_

* `image` [NativeImage](native-image.md)

当在 macOS 上按压 tray icon 的时候，让 `image` 与 tray icon 关联起来。

#### `tray.setToolTip(toolTip)`

* `toolTip` String

为 tray icon 设置 hover text。

#### `tray.setTitle(title)` _macOS_

* `title` String

在状态栏沿着 tray icon 设置标题。

#### `tray.setHighlightMode(mode)` _macOS_

* `mode` String - Highlight mode with one of the following values:
  * `selection` - Highlight the tray icon when it is clicked and also when
    its context menu is open. This is the default.
  * `always` - Always highlight the tray icon.
  * `never` - Never highlight the tray icon.

设置 tray icon 的背景色变为高亮（blue）。

**注意：** 你可以使用 `highlightMode` 和一个 [`BrowserWindow`](browser-window.md)
通过在窗口可见性时切换 `'never'` 和 `'always'` 模式变化。

```javascript
const {BrowserWindow, Tray} = require('electron')

const win = new BrowserWindow({width: 800, height: 600})
const tray = new Tray('/path/to/my/icon')

tray.on('click', () => {
  win.isVisible() ? win.hide() : win.show()
})
win.on('show', () => {
  tray.setHighlightMode('always')
})
win.on('hide', () => {
  tray.setHighlightMode('never')
})
```

#### `tray.displayBalloon(options)` _Windows_

* `options` Object
  * `icon` ([NativeImage](native-image.md) | String) - (可选)
  * `title` String - (可选)
  * `content` String - (可选)

展示一个 tray balloon。

#### `tray.popUpContextMenu([menu, position])` _macOS_ _Windows_

* `menu` Menu (optional)
* `position` Object (可选) - 上托位置.
  * `x` Integer
  * `y` Integer

从 tray icon 上托出 context menu。当划过 `menu` 的时候， `menu` 显示，代替 tray 的 context menu。

`position` 只在 windows 上可用，默认为 (0, 0)。

#### `tray.setContextMenu(menu)`

* `menu` Menu

为这个 icon 设置 context menu。

#### `tray.getBounds()` _macOS_ _Windows_

返回 [`Rectangle`](structures/rectangle.md)

这个 tray icon 的 `bounds` 对象。

#### `tray.isDestroyed()`

返回 `Boolean` - tray icon 是否销毁。

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
