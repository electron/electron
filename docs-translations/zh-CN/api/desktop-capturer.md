# desktopCapturer

`desktopCapturer` 模块可用来获取可用资源，这个资源可通过 `getUserMedia` 捕获得到.

```javascript
// 在渲染进程中.
var desktopCapturer = require('electron').desktopCapturer

desktopCapturer.getSources({types: ['window', 'screen']}, function (error, sources) {
  if (error) throw error
  for (var i = 0; i < sources.length; ++i) {
    if (sources[i].name == 'Electron') {
      navigator.webkitGetUserMedia({
        audio: false,
        video: {
          mandatory: {
            chromeMediaSource: 'desktop',
            chromeMediaSourceId: sources[i].id,
            minWidth: 1280,
            maxWidth: 1280,
            minHeight: 720,
            maxHeight: 720
          }
        }
      }, gotStream, getUserMediaError)
      return
    }
  }
})

function gotStream (stream) {
  document.querySelector('video').src = URL.createObjectURL(stream)
}

function getUserMediaError (e) {
  console.log('getUserMediaError')
}
```

当调用 `navigator.webkitGetUserMedia` 时创建一个约束对象，如果使用 `desktopCapturer` 的资源，必须设置 `chromeMediaSource` 为 `"desktop"` ，并且 `audio` 为 `false`.

如果你想捕获整个桌面的 audio 和 video，你可以设置 `chromeMediaSource` 为 `"screen"` ，和 `audio` 为 `true`.
当使用这个方法的时候，不可以指定一个 `chromeMediaSourceId`.

## 方法

`desktopCapturer` 模块有如下方法:

### `desktopCapturer.getSources(options, callback)`

* `options` Object
  * `types` Array - 一个 String 数组，列出了可以捕获的桌面资源类型, 可用类型为 `screen` 和 `window`.
  * `thumbnailSize` Object (可选) - 建议缩略可被缩放的 size, 默认为 `{width: 150, height: 150}`.
* `callback` Function

发起一个请求，获取所有桌面资源，当请求完成的时候使用 `callback(error, sources)` 调用  `callback` .

`sources` 是一个 `Source` 对象数组, 每个 `Source` 表示了一个捕获的屏幕或单独窗口，并且有如下属性 :
* `id` String - 在 `navigator.webkitGetUserMedia` 中使用的捕获窗口或屏幕的 id . 格式为 `window:XX` 祸
  `screen:XX`，`XX` 是一个随机数.
* `name` String - 捕获窗口或屏幕的描述名 . 如果资源为屏幕，名字为 `Entire Screen` 或 `Screen <index>`; 如果资源为窗口, 名字为窗口的标题.
* `thumbnail` [NativeImage](NativeImage.md) - 缩略图.

**注意:** 不能保证 `source.thumbnail` 的 size 和 `options` 中的 `thumnbailSize` 一直一致. 它也取决于屏幕或窗口的缩放比例.