# protocol

`protocol`モジュールはカスタムプロトコルを登録したり、または既存のプロトコルをインターセプタ―することができます。

`file://`プロトコルの同様の効果をもつプロトコルを実装した例です。

```javascript
const {app} = require('electron')
const path = require('path')

app.on('ready', function () {
  var protocol = electron.protocol
  protocol.registerFileProtocol('atom', function (request, callback) {
    var url = request.url.substr(7)
    callback({path: path.join(__dirname, url)})
  }, function (error) {
    if (error) console.error('Failed to register protocol')
  })
})
```

**Note:** このモジュールは、`app`モジュールで`ready`イベントが出力された後のみ使うことができます。

## メソッド

`protocol`モジュールは、次のメソッドを持ちます。

### `protocol.registerStandardSchemes(schemes)`

* `schemes` Array - 標準的なスキーマーを登録するためのカスタムスキーマー

標準的な`scheme`は、RFC 3986で策定している[generic URI syntax](https://tools.ietf.org/html/rfc3986#section-3)に準拠しています。これには`file:` と `filesystem:`を含んでいます。

### `protocol.registerServiceWorkerSchemes(schemes)`

* `schemes` Array - サービスワーカーをハンドルするために登録されたカスタムスキーマー

### `protocol.registerFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

レスポンスとしてファイルを送信する`scheme`のプロトコルを登録します。`scheme`で`request`が生成された時、`handler`は`handler(request, callback)`で呼び出されます。`scheme` 登録が成功したり、`completion(error)`が失敗したときに、`completion` は`completion(null)`で呼び出されます。

* `request` Object
  * `url` String
  * `referrer` String
  * `method` String
  * `uploadData` Array (オプション)
* `callback` Function

`uploadData` は `data` オブジェクトの配列です:

* `data` Object
  * `bytes` Buffer - 送信するコンテンツ
  * `file` String - アップロードするファイルパス

`request`をハンドルするために、`callback`はファイルパスまたは`path`プロパティを持つオブジェクトで呼び出すべきです。例えば、`callback(filePath)` または`callback({path: filePath})`です。

何もなし、数字、`error`プロパティを持つオブジェクトで、`callback`が呼び出された時、 `request`は指定した`error`番号で失敗します。使用できる提供されているエラー番号は、[net error list][net-error]を参照してください。

既定では、`scheme`は、`file:`のような一般的なURIの構文に続くプロトコルと違う解析がされ、`http:`のように扱われます。なので、恐らく標準的なスキーマーのように扱われるスキーマーを持つために、`protocol.registerStandardSchemes` を呼び出したくなります。

### `protocol.registerBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

レスポンスとして`Buffer`を送信する`scheme`プロトコルを登録します。

 `callback`は、`Buffer`オブジェクトまたは、`data`と`mimeType`、 `charset`プロパティを持つオブジェクトのどちらかで呼ばれる必要があることを除いて、この使用方法は、`registerFileProtocol`と同じです。

例:

```javascript
protocol.registerBufferProtocol('atom', function (request, callback) {
  callback({mimeType: 'text/html', data: new Buffer('<h5>Response</h5>')})
}, function (error) {
  if (error) console.error('Failed to register protocol')
})
```

### `protocol.registerStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

レスポンスとして`String`を送信する`scheme`プロトコルを登録します。

`callback`は、`String`または`data`と `mimeType`、`chart`プロパティを持つオブジェクトを呼び出す必要があることを除いて、使用方法は`registerFileProtocol`と同じです。

### `protocol.registerHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

レスポンスとしてHTTPリクエストを送信する`scheme`プロトコルを登録します。

`callback`は、`url`と`method`、`referrer`、`uploadData`、`session`プロパティを持つオブジェクトを呼び出す必要があることを除いて、使用方法は`registerFileProtocol`と同じです。

* `redirectRequest` Object
  * `url` String
  * `method` String
  * `session` Object (オプション)
  * `uploadData` Object (オプション)

既定では、HTTPリクエストは現在のセッションを再利用します。別のセッションでリクエストをしたい場合、`session` に `null`を設定する必要があります。

POSTリクエストは`uploadData`オブジェクトを提供する必要があります。
* `uploadData` object
  * `contentType` String - コンテンツのMIMEタイプ
  * `data` String - 送信されるコンテンツ

### `protocol.unregisterProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (optional)

`scheme`のカスタムプロトコルを解除します。

### `protocol.isProtocolHandled(scheme, callback)`

* `scheme` String
* `callback` Function

`scheme`のハンドラーがあるかないかを示すブーリアン値で`callback`がコールされます。

### `protocol.interceptFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`プロトコルをインターセプタ―し、レスポンスとしてファイルを送信するプロトコルの新しいハンドラーとして`handler`を使います。

### `protocol.interceptStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`プロトコルをインターセプタ―し、レスポンスとして`String`を送信するプロトコルの新しいハンドラーとして`handler`を使います。

### `protocol.interceptBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`プロトコルをインターセプタ―し、レスポンスとして`Buffer`を送信するプロトコルの新しいハンドラーとして`handler`を使います。

### `protocol.interceptHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`プロトコルをインターセプタ―し、レスポンスとして新しいHTTPリクエストを送信するプロトコルの新しいハンドラーとして`handler`を使います。

### `protocol.uninterceptProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function

インターセプタ―したインストールされた`scheme`を削除し、オリジナルハンドラーをリストアします。


[net-error]: https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h
