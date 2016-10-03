# screen

`screen` 模块检索屏幕的 size，显示，鼠标位置等的信息.在 `app` 模块的`ready` 事件触发之前不可使用这个模块.

`screen` 是一个 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

**注意:** 在渲染进程 / 开发者工具栏, `window.screen` 是一个预设值的 DOM
属性, 所以这样写 `var screen = require('electron').screen` 将不会工作.
在我们下面的例子, 我们取代使用可变名字的 `electronScreen`.
一个例子，创建一个充满整个屏幕的窗口 :

```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow

var mainWindow

app.on('ready', function () {
  var electronScreen = electron.screen
  var size = electronScreen.getPrimaryDisplay().workAreaSize
  mainWindow = new BrowserWindow({ width: size.width, height: size.height })
})
```

另一个例子，在此页外创建一个窗口:

```javascript
const electron = require('electron')
const app = electron.app
const BrowserWindow = electron.BrowserWindow

var mainWindow

app.on('ready', function () {
  var electronScreen = electron.screen
  var displays = electronScreen.getAllDisplays()
  var externalDisplay = null
  for (var i in displays) {
    if (displays[i].bounds.x != 0 || displays[i].bounds.y != 0) {
      externalDisplay = displays[i]
      break
    }
  }

  if (externalDisplay) {
    mainWindow = new BrowserWindow({
      x: externalDisplay.bounds.x + 50,
      y: externalDisplay.bounds.y + 50
    })
  }
})
```

## `Display` 对象

`Display` 对象表示一个连接到系统的物理显示. 一个虚设的 `Display` 或许存在于一个无头系统(headless system)中，或者一个 `Display` 对应一个远程的、虚拟的display.

* `display` object
  * `id` Integer - 与display 相关的唯一性标志.
  * `rotation` Integer - 可以是 0, 1, 2, 3, 每个代表了屏幕旋转的度数 0, 90, 180, 270.
  * `scaleFactor` Number - Output device's pixel scale factor.
  * `touchSupport` String - 可以是 `available`, `unavailable`, `unknown`.
  * `bounds` Object
  * `size` Object
  * `workArea` Object
  * `workAreaSize` Object

## 事件

`screen` 模块有如下事件:

### Event: 'display-added'

返回:

* `event` Event
* `newDisplay` Object

当添加了 `newDisplay` 时发出事件

### Event: 'display-removed'

返回:

* `event` Event
* `oldDisplay` Object

当移出了 `oldDisplay` 时发出事件

### Event: 'display-metrics-changed'

返回:

* `event` Event
* `display` Object
* `changedMetrics` Array

当一个 `display` 中的一个或更多的 metrics 改变时发出事件.
`changedMetrics` 是一个用来描述这个改变的数组.可能的变化为  `bounds`,
`workArea`, `scaleFactor` 和 `rotation`.

## 方法

`screen` 模块有如下方法:

### `screen.getCursorScreenPoint()`

返回当前鼠标的绝对路径 .

### `screen.getPrimaryDisplay()`

返回最主要的 display.

### `screen.getAllDisplays()`

返回一个当前可用的 display 数组.

### `screen.getDisplayNearestPoint(point)`

* `point` Object
  * `x` Integer
  * `y` Integer

返回离指定点最近的 display.

### `screen.getDisplayMatching(rect)`

* `rect` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

返回与提供的边界范围最密切相关的 display.
