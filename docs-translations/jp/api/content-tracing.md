# contentTracing

`content-tracing`モジュールは、Chromiumコンテンツモジュールによって生成されるトーレスデータを収集するのに使われます。このモジュールはウェブインターフェイスを含んでいないので、Chromeブラウザーで `chrome://tracing/`を開いて、結果を表示するために生成されたファイルを読み込む必要があります。

```javascript
const contentTracing = require('electron').contentTracing

const options = {
  categoryFilter: '*',
  traceOptions: 'record-until-full,enable-sampling'
}

contentTracing.startRecording(options, function () {
  console.log('Tracing started')

  setTimeout(function () {
    contentTracing.stopRecording('', function (path) {
      console.log('Tracing data recorded to ' + path)
    })
  }, 5000)
})
```

## メソッド

`content-tracing`モジュールは次のメソッドを持っています。

### `contentTracing.getCategories(callback)`

* `callback` Function

カテゴリグループ一式を取得します。新しいコードパスに到達しているとしてカテゴリグループを変更できます。

一度、全ての子プロセスが`getCategories`リクエストを認識すると、カテゴリグループの配列で`callback`が呼び出されます。

### `contentTracing.startRecording(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

全てのプロセスで記録を開始します。

EnableRecordingリクエストを受信するとすぐに、子プロセス上でただちに非同期にローカルに記録を始めます。全ての子プロセスが`startRecording`リクエストを認識すると、`callback`が呼び出されます。

`categoryFilter`はどのカテゴリグループをトレースすべきかをフィルタリングします。フィルターは、マッチしたカテゴリーを含むカテゴリグループを除外する`-`プレフィックスをオプションうぃ持っています。同じリストでの、対象カテゴリパターンと、除外カテゴリーパターンの両方を持つことはサポートしていません。

例:

* `test_MyTest*`,
* `test_MyTest*,test_OtherStuff`,
* `"-excluded_category1,-excluded_category2`

`traceOptions` は、どの種類のトレースを有効にするかを制御し、コンマ区切りのリストです。

取りうるオプション:

* `record-until-full`
* `record-continuously`
* `trace-to-console`
* `enable-sampling`
* `enable-systrace`

最初の3つのオプションは、トレースの記録モードで、そのため相互排他的です。`traceOptions`文字列に1つ以上のトレース記録モードが現れると、最後のモードが優先されます。トレース記録モードが指定されていない場合、記録モードは、`record-until-full`です。

適用される`traceOptions`からオプションをパースする前に、トレースオプションは最初に既定のオプションにリセットされます（`record_mode`は、`record-until-full`を設定し、 `enable_sampling`と `enable_systrace` は `false`に設定します）。

### `contentTracing.stopRecording(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

全てのプロセスで記録を止めます。

子プロセスは基本的にトレースデータをキャッシュし、まれにフラッシュし、メインプロセスにトレースデータを送り返します。IPC越しにトレースデータを送信するのは高コストな操作なので、トレースのランタイムオーバーヘッドを最小限にするのに役立ちます。トレースが終了すると、保留されているトレースデータのフラッシュをするためにすべての子プロセスに非道に問い合わせすべきです。


一度、すべての子プロセスが`stopRecording` リクエストを認識すると、トレースデータを含んだファイルで`callback`が呼び出されます。

トレースデータは`resultFilePath`が空でなければ、そこに書き込まれ、空の場合は一時ファイルに書き込まれます。実際のファイルパスは`null`でなければ `callback` に通します。

### `contentTracing.startMonitoring(options, callback)`

* `options` Object
  * `categoryFilter` String
  * `traceOptions` String
* `callback` Function

全てのプロセス上で監視を開始します。

`startMonitoring`リクエスト受信するとすぐに、子プロセス上でローカルに非同期にただちに監視を始めます。

全ての子プロセスが`startMonitoring`リクエストを認識すると、`callback`がコールされます。

### `contentTracing.stopMonitoring(callback)`

* `callback` Function

全てのプロセス上で監視を止めます。

全ての子プロセスが`stopMonitoring`リクエスト認識すると、`callback`がコールされます。

### `contentTracing.captureMonitoringSnapshot(resultFilePath, callback)`

* `resultFilePath` String
* `callback` Function

現在の監視トレースデータを取得します。子プロセスは基本的にトレースデータをキャッシュし、まれにフラッシュし、メインプロセスにトレースデータを送り返します。IPC越しにトレースデータを送信するのは高コストな操作なので、トレースによる不必要なランタイムオーバーヘッドを避けるます。トレースが終了するために、保留されているトレースデータのフラッシュをするためにすべての子プロセスに非道に問い合わせすべきです。

全ての子プロセスが`captureMonitoringSnapshot`リクエストを認識すると、トレースデータを含んだファイルで`callback`が呼び出されます。

### `contentTracing.getTraceBufferUsage(callback)`

* `callback` Function

プロセスのトレースバッファのプロセス間で最大使用量をフルの状態の何%かで取得します。TraceBufferUsage値が設定されていると、 `callback`がコールされます。

### `contentTracing.setWatchEvent(categoryName, eventName, callback)`

* `categoryName` String
* `eventName` String
* `callback` Function

プロセス上でイベント発生すると、その度に`callback`がコールされます。

### `contentTracing.cancelWatchEvent()`

イベントウオッチをキャンセルします。トレースが有効になっていると、監視イベントのコールバックとの競合状態になる可能性があります。
