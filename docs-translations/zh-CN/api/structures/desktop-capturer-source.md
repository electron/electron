# DesktopCapturerSource Object

* `id` String - 窗口或者屏幕的标识符，当调用 [`navigator.webkitGetUserMedia`] 时可以被当成 `chromeMediaSourceId` 使用。
标识符的格式为`window:XX` 或 `screen:XX`，`XX` 是一个随机生成的数字.
* `name` String - 窗口的来源将被命名为 `Entire Screen` 或 `Screen <index>`，而窗口来源的名字将会和窗口的标题匹配.
* `thumbnail` [NativeImage](../native-image.md) - 缩略图. **注:** 通过 `desktopCapturer.getSources` 方法，
不能保证缩略图的大小与 `options` 中指定的 `thumbnailSize` 相同。实际大小取决于窗口或者屏幕的比例。
