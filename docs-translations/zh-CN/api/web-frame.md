# webFrame

`web-frame` 模块允许你自定义如何渲染当前网页 .

例子，放大当前页到 200%.

```javascript
var webFrame = require('electron').webFrame;

webFrame.setZoomFactor(2);
```

## 方法

`web-frame` 模块有如下方法:

### `webFrame.setZoomFactor(factor)`

* `factor` Number - 缩放参数.

将缩放参数修改为指定的参数值.缩放参数是百分制的，所以 300% = 3.0.

### `webFrame.getZoomFactor()`

返回当前缩放参数值.

### `webFrame.setZoomLevel(level)`

* `level` Number - 缩放水平

将缩放水平修改为指定的水平值. 原始 size 为 0 ，并且每次增长都表示放大 20% 或缩小 20%，默认限制为原始 size 的 300% 到 50% 之间 .

### `webFrame.getZoomLevel()`

返回当前缩放水平值.

### `webFrame.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

设置缩放水平的最大值和最小值.

### `webFrame.setSpellCheckProvider(language, autoCorrectWord, provider)`

* `language` String
* `autoCorrectWord` Boolean
* `provider` Object

为输入框或文本域设置一个拼写检查 provider .

`provider` 必须是一个对象，它有一个 `spellCheck` 方法，这个方法返回扫过的单词是否拼写正确 .

例子，使用 [node-spellchecker][spellchecker] 作为一个 provider:

```javascript
webFrame.setSpellCheckProvider("en-US", true, {
  spellCheck: function(text) {
    return !(require('spellchecker').isMisspelled(text));
  }
});
```

### `webFrame.registerURLSchemeAsSecure(scheme)`

* `scheme` String

注册 `scheme` 为一个安全的 scheme.


安全的 schemes 不会引发混合内容 warnings.例如, `https` 和
`data` 是安全的 schemes ，因为它们不能被活跃网络攻击而失效.

### `webFrame.registerURLSchemeAsBypassingCSP(scheme)`

* `scheme` String

忽略当前网页内容的安全策略，直接从 `scheme` 加载.

### `webFrame.registerURLSchemeAsPrivileged(scheme)`

* `scheme` String

通过资源的内容安全策略，注册 `scheme` 为安全的 scheme，允许注册 ServiceWorker并且支持 fetch API.

### `webFrame.insertText(text)`

* `text` String

向获得焦点的原色插入内容 .

### `webFrame.executeJavaScript(code[, userGesture])`

* `code` String
* `userGesture` Boolean (可选) - 默认为 `false`.

评估页面代码 .

在浏览器窗口中，一些 HTML APIs ，例如 `requestFullScreen`，只可以通过用户手势来使用.设置`userGesture` 为 `true` 可以突破这个限制 .

[spellchecker]: https://github.com/atom/node-spellchecker