# BrowserWindow

 `BrowserWindow` 类让你有创建一个浏览器窗口的权力。例如:

```javascript
// In the main process.
const BrowserWindow = require('electron').BrowserWindow

// Or in the renderer process.
// const BrowserWindow = require('electron').remote.BrowserWindow

var win = new BrowserWindow({ width: 800, height: 600, show: false })
win.on('closed', function () {
  win = null
})

win.loadURL('https://github.com')
win.show()
```

你也可以不通过chrome创建窗口，使用
[Frameless Window](frameless-window.md) API.

## Class: BrowserWindow

`BrowserWindow` 是一个
[EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter).

通过 `options` 可以创建一个具有本质属性的 `BrowserWindow` .

### `new BrowserWindow([options])`

* `options` Object
  * `width` Integer - 窗口宽度,单位像素. 默认是 `800`.
  * `height` Integer - 窗口高度,单位像素. 默认是 `600`.
  * `x` Integer - 窗口相对于屏幕的左偏移位置.默认居中.
  * `y` Integer - 窗口相对于屏幕的顶部偏移位置.默认居中.
  * `useContentSize` Boolean - `width` 和 `height` 使用web网页size, 这意味着实际窗口的size应该包括窗口框架的size，稍微会大一点，默认为 `false`.
  * `center` Boolean - 窗口屏幕居中.
  * `minWidth` Integer - 窗口最小宽度，默认为 `0`.
  * `minHeight` Integer - 窗口最小高度，默认为 `0`.
  * `maxWidth` Integer - 窗口最大宽度，默认无限制.
  * `maxHeight` Integer - 窗口最大高度，默认无限制.
  * `resizable` Boolean - 是否可以改变窗口size，默认为 `true`.
  * `movable` Boolean - 窗口是否可以拖动. 在 Linux 上无效. 默认为 `true`.
  * `minimizable` Boolean - 窗口是否可以最小化. 在 Linux 上无效. 默认为 `true`.
  * `maximizable` Boolean - 窗口是否可以最大化. 在 Linux 上无效. 默认为 `true`.
  * `closable` Boolean - 窗口是否可以关闭. 在 Linux 上无效. 默认为 `true`.
  * `alwaysOnTop` Boolean - 窗口是否总是显示在其他窗口之前. 在 Linux 上无效. 默认为 `false`.
  * `fullscreen` Boolean - 窗口是否可以全屏幕. 当明确设置值为 `false` ，全屏化按钮将会隐藏，在 macOS 将禁用. 默认 `false`.
  * `fullscreenable` Boolean - 在 macOS 上，全屏化按钮是否可用，默认为 `true`.
  * `skipTaskbar` Boolean - 是否在任务栏中显示窗口. 默认是`false`.
  * `kiosk` Boolean - kiosk 方式. 默认为 `false`.
  * `title` String - 窗口默认title. 默认 `"Electron"`.
  * `icon` [NativeImage](native-image.md) - 窗口图标, 如果不设置，窗口将使用可用的默认图标.
  * `show` Boolean - 窗口创建的时候是否显示. 默认为 `true`.
  * `frame` Boolean - 指定 `false` 来创建一个
    [Frameless Window](frameless-window.md). 默认为 `true`.
  * `acceptFirstMouse` Boolean - 是否允许单击web view来激活窗口 . 默认为 `false`.
  * `disableAutoHideCursor` Boolean - 当 typing 时是否隐藏鼠标.默认 `false`.
  * `autoHideMenuBar` Boolean - 除非点击 `Alt`，否则隐藏菜单栏.默认为 `false`.
  * `enableLargerThanScreen` Boolean - 是否允许改变窗口大小大于屏幕. 默认是 `false`.
  * `backgroundColor` String -窗口的 background color 值为十六进制,如 `#66CD00` 或 `#FFF` 或 `#80FFFFFF` (支持透明度). 默认为在 Linux 和 Windows 上为
    `#000` (黑色) , Mac上为 `#FFF`(或透明).
  * `hasShadow` Boolean - 窗口是否有阴影. 只在 macOS 上有效. 默认为 `true`.
  * `darkTheme` Boolean - 为窗口使用 dark 主题, 只在一些拥有 GTK+3 桌面环境上有效. 默认为 `false`.
  * `transparent` Boolean - 窗口 [透明](frameless-window.md).
    默认为 `false`.
  * `type` String - 窗口type, 默认普通窗口. 下面查看更多.
  * `titleBarStyle` String - 窗口标题栏样式. 下面查看更多.
  * `webPreferences` Object - 设置界面特性. 下面查看更多.

`type` 的值和效果不同平台展示效果不同，具体:

* Linux, 可用值为 `desktop`, `dock`, `toolbar`, `splash`,
  `notification`.
* macOS, 可用值为 `desktop`, `textured`.
  * `textured` type 添加金属梯度效果
    (`NSTexturedBackgroundWindowMask`).
  * `desktop` 设置窗口在桌面背景窗口水平
    (`kCGDesktopWindowLevel - 1`). 注意桌面窗口不可聚焦, 不可不支持键盘和鼠标事件, 但是可以使用 `globalShortcut` 来解决输入问题.

`titleBarStyle` 只在 macOS 10.10 Yosemite 或更新版本上支持.
可用值:

* `default` 以及无值, 显示在 Mac 标题栏上为不透明的标准灰色.
* `hidden` 隐藏标题栏，内容充满整个窗口, 然后它依然在左上角，仍然受标准窗口控制.
* `hidden-inset`主体隐藏，显示小的控制按钮在窗口边缘.

`webPreferences` 参数是个对象，它的属性:

* `nodeIntegration` Boolean - 是否完整支持node. 默认为 `true`.
* `preload` String - 界面的其它脚本运行之前预先加载一个指定脚本. 这个脚本将一直可以使用 node APIs 无论 node integration 是否开启. 脚本路径为绝对路径.
  当 node integration 关闭, 预加载的脚本将从全局范围重新引入node的全局引用标志. 查看例子
  [here](process.md#event-loaded).
* `session` [Session](session.md#class-session) - 设置界面session. 而不是直接忽略session对象 , 也可用 `partition` 来代替, 它接受一个 partition 字符串. 当同时使用 `session` 和 `partition`, `session` 优先级更高.
  默认使用默认 session.
* `partition` String - 通过session的partition字符串来设置界面session. 如果 `partition` 以 `persist:` 开头, 这个界面将会为所有界面使用相同的 `partition`. 如果没有 `persist:` 前缀, 界面使用历史session. 通过分享同一个 `partition`, 所有界面使用相同的session. 默认使用默认 session.
* `zoomFactor` Number - 界面默认缩放值, `3.0` 表示
  `300%`. 默认 `1.0`.
* `javascript` Boolean - 开启javascript支持. 默认为`true`.
* `webSecurity` Boolean - 当设置为 `false`, 它将禁用同源策略 (通常用来测试网站), 并且如果有2个非用户设置的参数，就设置
  `allowDisplayingInsecureContent` 和 `allowRunningInsecureContent` 的值为
  `true`. 默认为 `true`.
* `allowDisplayingInsecureContent` Boolean -允许一个使用 https的界面来展示由 http URLs 传过来的资源. 默认`false`.
* `allowRunningInsecureContent` Boolean - Boolean -允许一个使用 https的界面来渲染由 http URLs 提交的html,css,javascript. 默认为 `false`.
* `images` Boolean - 开启图片使用支持. 默认 `true`.
* `textAreasAreResizable` Boolean - textArea 可以编辑. 默认为 `true`.
* `webgl` Boolean - 开启 WebGL 支持. 默认为 `true`.
* `webaudio` Boolean - 开启 WebAudio 支持. 默认为 `true`.
* `plugins` Boolean - 是否开启插件支持. 默认为 `false`.
* `experimentalFeatures` Boolean - 开启 Chromium 的 可测试 特性.
  默认为 `false`.
* `experimentalCanvasFeatures` Boolean - 开启 Chromium 的 canvas 可测试特性. 默认为 `false`.
* `directWrite` Boolean - 开启窗口的 DirectWrite font 渲染系统. 默认为 `true`.
* `blinkFeatures` String - 以 `,` 分隔的特性列表, 如
  `CSSVariables,KeyboardEventKey`. 被支持的所有特性可在 [setFeatureEnabledFromString][blink-feature-string]
  中找到.
* `defaultFontFamily` Object - 设置 font-family 默认字体.
  * `standard` String - 默认为 `Times New Roman`.
  * `serif` String - 默认为 `Times New Roman`.
  * `sansSerif` String - 默认为 `Arial`.
  * `monospace` String - 默认为 `Courier New`.
* `defaultFontSize` Integer - 默认为 `16`.
* `defaultMonospaceFontSize` Integer - 默认为 `13`.
* `minimumFontSize` Integer - 默认为 `0`.
* `defaultEncoding` String - 默认为 `ISO-8859-1`.

## 事件

 `BrowserWindow` 对象可触发下列事件:

**注意:** 一些事件只能在特定os环境中触发，已经尽可能地标出.

### Event: 'page-title-updated'

返回:

* `event` Event

当文档改变标题时触发,使用 `event.preventDefault()` 可以阻止原窗口的标题改变.

### Event: 'close'

返回:

* `event` Event

在窗口要关闭的时候触发. 它在DOM的 `beforeunload` and `unload` 事件之前触发.使用 `event.preventDefault()` 可以取消这个操作

通常你想通过 `beforeunload` 处理器来决定是否关闭窗口，但是它也会在窗口重载的时候被触发. 在 Electron 中，返回一个空的字符串或 `false` 可以取消关闭.例如:

```javascript
window.onbeforeunload = function (e) {
  console.log('I do not want to be closed')

  // Unlike usual browsers, in which a string should be returned and the user is
  // prompted to confirm the page unload, Electron gives developers more options.
  // Returning empty string or false would prevent the unloading now.
  // You can also use the dialog API to let the user confirm closing the application.
  e.returnValue = false
}
```

### Event: 'closed'

当窗口已经关闭的时候触发.当你接收到这个事件的时候，你应当删除对已经关闭的窗口的引用对象和避免再次使用它.

### Event: 'unresponsive'

在界面卡死的时候触发事件.

### Event: 'responsive'

在界面恢复卡死的时候触发.

### Event: 'blur'

在窗口失去焦点的时候触发.

### Event: 'focus'

在窗口获得焦点的时候触发.

### Event: 'maximize'

在窗口最大化的时候触发.

### Event: 'unmaximize'

在窗口退出最大化的时候触发.

### Event: 'minimize'

在窗口最小化的时候触发.

### Event: 'restore'

在窗口从最小化恢复的时候触发.

### Event: 'resize'

在窗口size改变的时候触发.

### Event: 'move'

在窗口移动的时候触发.

注意：在 macOS 中别名为 `moved`.

### Event: 'moved' _macOS_

在窗口移动的时候触发.

### Event: 'enter-full-screen'

在的窗口进入全屏状态时候触发.

### Event: 'leave-full-screen'

在的窗口退出全屏状态时候触发.

### Event: 'enter-html-full-screen'

在的窗口通过 html api 进入全屏状态时候触发.

### Event: 'leave-html-full-screen'

在的窗口通过 html api 退出全屏状态时候触发.

### Event: 'app-command' _Windows_

在请求一个[App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)的时候触发.
典型的是键盘媒体或浏览器命令, Windows上的 "Back" 按钮用作鼠标也会触发.

```javascript
someWindow.on('app-command', function (e, cmd) {
  // Navigate the window back when the user hits their mouse back button
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack()
  }
})
```

### Event: 'scroll-touch-begin' _macOS_

在滚动条事件开始的时候触发.

### Event: 'scroll-touch-end' _macOS_

在滚动条事件结束的时候触发.

## 方法

`BrowserWindow` 对象有如下方法:

### `BrowserWindow.getAllWindows()`

返回一个所有已经打开了窗口的对象数组.

### `BrowserWindow.getFocusedWindow()`

返回应用当前获得焦点窗口,如果没有就返回 `null`.

### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](web-contents.md)

根据 `webContents` 查找窗口.

### `BrowserWindow.fromId(id)`

* `id` Integer

根据 id 查找窗口.

### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

添加位于 `path` 的开发者工具栏扩展,并且返回扩展项的名字.

这个扩展会被添加到历史，所以只需要使用这个API一次，这个api不可用作编程使用.

### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

删除开发者工具栏名为 `name` 的扩展.

## 实例属性

使用 `new BrowserWindow` 创建的实例对象，有如下属性:

```javascript
// In this example `win` is our instance
var win = new BrowserWindow({ width: 800, height: 600 })
```

### `win.webContents`

这个窗口的 `WebContents` 对象，所有与界面相关的事件和方法都通过它完成的.

查看 [`webContents` documentation](web-contents.md) 的方法和事件.

### `win.id`

窗口的唯一id.

## 实例方法

使用 `new BrowserWindow` 创建的实例对象，有如下方法:

**注意:** 一些方法只能在特定os环境中调用，已经尽可能地标出.

### `win.destroy()`

强制关闭窗口, `unload` and `beforeunload` 不会触发，并且 `close` 也不会触发, 但是它保证了 `closed` 触发.

### `win.close()`

尝试关闭窗口，这与用户点击关闭按钮的效果一样. 虽然网页可能会取消关闭，查看 [close event](#event-close).

### `win.focus()`

窗口获得焦点.

### `win.isFocused()`

返回 boolean, 窗口是否获得焦点.

### `win.show()`

展示并且使窗口获得焦点.

### `win.showInactive()`

展示窗口但是不获得焦点.

### `win.hide()`

隐藏窗口.

### `win.isVisible()`

返回 boolean, 窗口是否可见.

### `win.maximize()`

窗口最大化.

### `win.unmaximize()`

取消窗口最大化.

### `win.isMaximized()`

返回 boolean, 窗口是否最大化.

### `win.minimize()`

窗口最小化. 在一些os中，它将在dock中显示.

### `win.restore()`

将最小化的窗口恢复为之前的状态.

### `win.isMinimized()`

返回 boolean, 窗口是否最小化.

### `win.setFullScreen(flag)`

* `flag` Boolean

设置是否全屏.

### `win.isFullScreen()`

返回 boolean, 窗口是否全屏化.

### `win.setAspectRatio(aspectRatio[, extraSize])` _macOS_

* `aspectRatio` 维持部分视图内容窗口的高宽比值.
* `extraSize` Object (可选) - 维持高宽比值时不包含的额外size.
  * `width` Integer
  * `height` Integer

由一个窗口来维持高宽比值. `extraSize` 允许开发者使用它，它的单位为像素，不包含在 `aspectRatio` 中.这个 API 可用来区分窗口的size和内容的size .

想象一个普通可控的HD video 播放器窗口. 假如左边缘有15控制像素，右边缘有25控制像素，在播放器下面有50控制像素.为了在播放器内保持一个 16:9 的高宽比例，我们可以调用这个api传入参数16/9 and
[ 40, 50 ].第二个参数不管网页中的额外的宽度和高度在什么位置，只要它们存在就行.只需要把网页中的所有额外的高度和宽度加起来就行.

### `win.setBounds(options[, animate])`

* `options` Object
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `animate` Boolean (可选) _macOS_

重新设置窗口的宽高值，并且移动到指定的 `x`, `y` 位置.

### `win.getBounds()`

返回一个对象，它包含了窗口的宽，高，x坐标，y坐标.

### `win.setSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (可选) _macOS_

重新设置窗口的宽高值.

### `win.getSize()`

返回一个数组，它包含了窗口的宽，高.

### `win.setContentSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (可选) _macOS_

重新设置窗口客户端的宽高值（例如网页界面）.

### `win.getContentSize()`

返回一个数组，它包含了窗口客户端的宽，高.

### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

设置窗口最小化的宽高值.

### `win.getMinimumSize()`

返回一个数组，它包含了窗口最小化的宽，高.

### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

设置窗口最大化的宽高值.

### `win.getMaximumSize()`

返回一个数组，它包含了窗口最大化的宽，高.

### `win.setResizable(resizable)`

* `resizable` Boolean

设置窗口是否可以被用户改变size.

### `win.isResizable()`

返回 boolean,窗口是否可以被用户改变size.

### `win.setMovable(movable)` _macOS_ _Windows_

* `movable` Boolean

设置窗口是否可以被用户拖动. Linux 无效.

### `win.isMovable()` _macOS_ _Windows_

返回 boolean,窗口是否可以被用户拖动. Linux 总是返回 `true`.

### `win.setMinimizable(minimizable)` _macOS_ _Windows_

* `minimizable` Boolean

设置窗口是否可以最小化. Linux 无效.

### `win.isMinimizable()` _macOS_ _Windows_

返回 boolean,窗口是否可以最小化. Linux 总是返回 `true`.

### `win.setMaximizable(maximizable)` _macOS_ _Windows_

* `maximizable` Boolean

设置窗口是否可以最大化. Linux 无效.

### `win.isMaximizable()` _macOS_ _Windows_

返回 boolean,窗口是否可以最大化. Linux 总是返回 `true`.

### `win.setFullScreenable(fullscreenable)`

* `fullscreenable` Boolean

设置点击最大化按钮是否可以全屏或最大化窗口.

### `win.isFullScreenable()`

返回 boolean,点击最大化按钮是否可以全屏或最大化窗口.

### `win.setClosable(closable)` _macOS_ _Windows_

* `closable` Boolean

设置窗口是否可以人为关闭. Linux 无效.

### `win.isClosable()` _macOS_ _Windows_

返回 boolean,窗口是否可以人为关闭. Linux 总是返回 `true`.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

是否设置这个窗口始终在其他窗口之上.设置之后，这个窗口仍然是一个普通的窗口，不是一个不可以获得焦点的工具箱窗口.

### `win.isAlwaysOnTop()`

返回 boolean,当前窗口是否始终在其它窗口之前.

### `win.center()`

窗口居中.

### `win.setPosition(x, y[, animate])`

* `x` Integer
* `y` Integer
* `animate` Boolean (可选) _macOS_

移动窗口到对应的 `x` and `y` 坐标.

### `win.getPosition()`

返回一个包含当前窗口位置的数组.

### `win.setTitle(title)`

* `title` String

改变原窗口的title.

### `win.getTitle()`

返回原窗口的title.

**注意:** 界面title可能和窗口title不相同.

### `win.flashFrame(flag)`

* `flag` Boolean

开始或停止显示窗口来获得用户的关注.

### `win.setSkipTaskbar(skip)`

* `skip` Boolean

让窗口不在任务栏中显示.

### `win.setKiosk(flag)`

* `flag` Boolean

进入或离开 kiosk 模式.

### `win.isKiosk()`

返回 boolean,是否进入或离开 kiosk 模式.

### `win.getNativeWindowHandle()`

以 `Buffer` 形式返回这个具体平台的窗口的句柄.

windows上句柄类型为 `HWND` ，macOS `NSView*` ， Linux `Window`.

### `win.hookWindowMessage(message, callback)` _Windows_

* `message` Integer
* `callback` Function

拦截windows 消息，在 WndProc 接收到消息时触发 `callback`函数.

### `win.isWindowMessageHooked(message)` _Windows_

* `message` Integer

返回 `true` or `false` 来代表是否拦截到消息.

### `win.unhookWindowMessage(message)` _Windows_

* `message` Integer

不拦截窗口消息.

### `win.unhookAllWindowMessages()` _Windows_

窗口消息全部不拦截.

### `win.setRepresentedFilename(filename)` _macOS_

* `filename` String

设置窗口当前文件路径，并且将这个文件的图标放在窗口标题栏上.

### `win.getRepresentedFilename()` _macOS_

获取窗口当前文件路径.

### `win.setDocumentEdited(edited)` _macOS_

* `edited` Boolean

明确指出窗口文档是否可以编辑，如果可以编辑则将标题栏的图标变成灰色.

### `win.isDocumentEdited()` _macOS_

返回 boolean,当前窗口文档是否可编辑.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (可选) - 捕获Page位置
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

捕获 `rect` 中的page 的快照.完成后将调用回调函数 `callback` 并返回 `image` . `image` 是存储了快照信息的[NativeImage](native-image.md)实例.如果不设置 `rect` 则将捕获所有可见page.

### `win.loadURL(url[, options])`

类似 `webContents.loadURL(url[, options])`.

### `win.reload()`

类似 `webContents.reload`.

### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

设置菜单栏的 `menu` ，设置它为 `null` 则表示不设置菜单栏.

### `win.setProgressBar(progress)`

* `progress` Double

在进度条中设置进度值，有效范围 [0, 1.0].

当进度小于0时则不显示进度;
当进度大于0时显示结果不确定.

在libux上，只支持Unity桌面环境，需要指明 `*.desktop` 文件并且在 `package.json` 中添加文件名字.默认它为 `app.getName().desktop`.

### `win.setOverlayIcon(overlay, description)` _Windows 7+_

* `overlay` [NativeImage](native-image.md) - 在底部任务栏右边显示图标.
* `description` String - 描述.

向当前任务栏添加一个 16 x 16 像素的图标，通常用来覆盖一些应用的状态，或者直接来提示用户.

### `win.setHasShadow(hasShadow)` _macOS_

* `hasShadow` (Boolean)

设置窗口是否应该有阴影.在Windows和Linux系统无效.

### `win.hasShadow()` _macOS_

返回 boolean,设置窗口是否有阴影.在Windows和Linux系统始终返回
`true`.

### `win.setThumbarButtons(buttons)` _Windows 7+_

* `buttons` Array

在窗口的任务栏button布局出为缩略图添加一个有特殊button的缩略图工具栏. 返回一个 `Boolean` 对象来指示是否成功添加这个缩略图工具栏.

因为空间有限，缩略图工具栏上的 button 数量不应该超过7个.一旦设置了，由于平台限制，就不能移动它了.但是你可使用一个空数组来调用api来清除 buttons .

所有 `buttons` 是一个 `Button` 对象数组:

* `Button` Object
  * `icon` [NativeImage](native-image.md) - 在工具栏上显示的图标.
  * `click` Function
  * `tooltip` String (可选) - tooltip 文字.
  * `flags` Array (可选) - 控制button的状态和行为. 默认它是 `['enabled']`.

`flags` 是一个数组，它包含下面这些 `String`s:

* `enabled` - button 为激活状态并且开放给用户.
* `disabled` -button 不可用. 目前它有一个可见的状态来表示它不会响应你的行为.
* `dismissonclick` - 点击button，这个缩略窗口直接关闭.
* `nobackground` - 不绘制边框，仅仅使用图像.
* `hidden` - button 对用户不可见.
* `noninteractive` - button 可用但是不可响应; 也不显示按下的状态. 它的值意味着这是一个在通知单使用 button 的实例.

### `win.showDefinitionForSelection()` _macOS_

在界面查找选中文字时显示弹出字典.

### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

设置窗口的菜单栏是否可以自动隐藏. 一旦设置了，只有当用户按下 `Alt` 键时则显示.

如果菜单栏已经可见，调用 `setAutoHideMenuBar(true)` 则不会立刻隐藏.

### `win.isMenuBarAutoHide()`

返回 boolean,窗口的菜单栏是否可以自动隐藏.

### `win.setMenuBarVisibility(visible)` _Windows_ _Linux_

* `visible` Boolean

设置菜单栏是否可见.如果菜单栏自动隐藏，用户仍然可以按下 `Alt` 键来显示.

### `win.isMenuBarVisible()`

返回 boolean,菜单栏是否可见.

### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

设置窗口是否在所有地方都可见.

**注意:** 这个api 在windows无效.

### `win.isVisibleOnAllWorkspaces()`

返回 boolean,窗口是否在所有地方都可见.

**注意:** 在 windows上始终返回 false.

### `win.setIgnoreMouseEvents(ignore)` _macOS_

* `ignore` Boolean

忽略窗口的所有鼠标事件.

[blink-feature-string]: https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/blink/platform/RuntimeEnabledFeatures.cpp&sq=package:chromium&type=cs&l=527
