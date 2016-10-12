# Frameless Window

无边框窗口指的是不包含除页面本身以外任何其它可视部分的窗口([chrome](https://developer.mozilla.org/en-US/docs/Glossary/Chrome))。
像工具栏，是窗口的一部分，但并不属于页面。这些是[`BrowserWindow`](browser-window.md) 类的选项。

## 创建无边框窗口

为了创建一个无边框窗口，你需要设置[BrowserWindow](browser-window.md)的`frame`为`false`:



```javascript
const BrowserWindow = require('electron').BrowserWindow
var win = new BrowserWindow({ width: 800, height: 600, frame: false })
```

### macOS上的替代方案

在macOS 10.10 Yosemite或者更新的版本中，有一个替代方案去生成一个无边框窗口。
不同于设置`frame`为`false`会隐藏标题栏以及失去对窗口的控制，你可能想隐藏标题栏使你的页面内容显示在整个窗口上
，同时又想保持对窗口的控制("traffic lights")。你可以通过指定`titleBarStyle`这一新选项达到目标:

```javascript
var win = new BrowserWindow({ 'titleBarStyle': 'hidden' })
```

## 透明窗口

通过设置`transparent` 选项为 `true`，你能使无边框窗口透明:

```javascript
var win = new BrowserWindow({ transparent: true, frame: false })
```

### 限制

* 你无法点击透明的区域。我们正在采用一个新的API去设置窗口的外形以解决这个问题，
  详见[our issue](https://github.com/electron/electron/issues/1335)。
* 透明窗口是不可调整大小的。在某些平台上，设置`resizable`为`true`也许会造成这个透明窗口停止工作。
* `blur`滤光器器只适用于网页，所以没法将模糊效果用于窗口之下(i.e. 其它在用户的系统中打开的应用)。
* 在Windows系统中，当DWM被禁用时透明窗口不会工作。
* Linux用户需要在命令行中输入`--enable-transparent-visuals --disable-gpu`
  去禁用GPU以及允许ARGB去渲染透明窗口，这是由于一个Linux上的上游bug[alpha channel doesn't work on some
  NVidia drivers](https://code.google.com/p/chromium/issues/detail?id=369209)造成的
* 在Mac上，透明窗口的阴影不会显示出来.

## 可拖动区域

默认情况下，无边框窗口是不可拖动的。应用在CSS中设置`-webkit-app-region: drag`
告诉Electron哪个区域是可拖动的(比如系统标准的标题栏)，应用也可以设置`-webkit-app-region: no-drag`
在可拖动区域中排除不可拖动的区域。需要注意的是，目前只支持矩形区域。

为了让整个窗口可拖动，你可以在`body`的样式中添加`-webkit-app-region: drag`:

```html
<body style="-webkit-app-region: drag">
</body>
```

另外需要注意的是，如果你设置了整个窗口可拖动，你必须标记按钮为不可拖动的(non-draggable)，
否则用户不能点击它们:

```css
button {
  -webkit-app-region: no-drag;
}
```

如果你设置一个自定义的标题栏可拖动，你同样需要设置标题栏中所有的按钮为不可拖动(non-draggable)。

## 文本选择

在一个无边框窗口中，拖动动作会与文本选择发生冲突。比如，当你拖动标题栏，偶尔会选中标题栏上的文本。
为了防止这种情况发生，你需要向下面这样在一个可拖动区域中禁用文本选择:

```css
.titlebar {
  -webkit-user-select: none;
  -webkit-app-region: drag;
}
```

## 上下文菜单

在一些平台上，可拖动区域会被认为是非客户端框架(non-client frame)，所有当你点击右键时，一个系统菜单会弹出。
为了保证上下文菜单在所有平台下正确的显示，你不应该在可拖动区域使用自定义上下文菜单。

