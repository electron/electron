# desktopCapturer

`desktopCapturer`モジュールは`getUserMedia`でキャプチャーするのに使える利用可能なソースを取得するのに使われます。

```javascript
// In the renderer process.
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

`navigator.webkitGetUserMedia`用の制限されたオブジェクトコールを作成し、`desktopCapturer`からソースを使用するのなら、`chromeMediaSource`は`"desktop"`を設定し、`audio`は`false`を設定しなければなりません。

全てのデスクトップから音とビデオをキャプチャーしたいなら、`chromeMediaSource``"screen"`、`audio` に `true`.を設定します。このメソッドを使うとき、`chromeMediaSourceId`は指定できません。

## メソッド

`desktopCapturer`モジュールは次のメソッドを持ちます。

### `desktopCapturer.getSources(options, callback)`

* `options` Object
  * `types` Array - キャプチャーされるデスクトップソースの種類一覧の文字列配列で、 提供される種類は`screen` と `window`です。
  * `thumbnailSize` Object (オプション) - サムネイルがスケールすべきサイズの指定で、既定では`{width: 150, height: 150}` です。
* `callback` Function

全てのデスクトップソールを取得するためのリクエストを開始し、リクエストが完了すると、`callback`は`callback(error, sources)` でコールされます。

`sources`は、`Source`オブジェクトの配列で、それぞれの`Source`はキャプチャーしたスクリーンか、1つのウィンドウを示し、次のプロパティを持ちます。


* `id` String - The id of the captured window or screen used in
  `navigator.webkitGetUserMedia`で使われるキャプチャーしたウィンドウか画面のidです。`window:XX` か`screen:XX`のようなフォーマットで、`XX`はランダムに生成された数字です。
* `name` String - キャプチャーする画面かウィンドウの説明名ソースが画面なら名前は`Entire Screen`で、`Screen <index>`はウィンドウで、名前はウィンドウのタイトルです。
* `thumbnail` [NativeImage](NativeImage.md) - サムネイル画像

**Note:**  `source.thumbnail`のサイズはいつも`options`の`thumnbailSize`と同じ保証はありません。画面またはウィンドウのサイズに依存します。
