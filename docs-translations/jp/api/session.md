# session

`session`モジュールは、新しい`Session`オブジェクトを作成するのに使われます。

[`BrowserWindow`](browser-window.md)のプロパティである [`webContents`](web-contents.md)プロパティの`session`を使うことで既存ページの `session`にアクセスできます。

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadURL("http://github.com");

var ses = win.webContents.session;
```

## メソッド

 `session`メソッドは次のメソッドを持ちます：

### session.fromPartition(partition)

* `partition` String

`partition`文字列から新しい`Session`インスタンスを返します。

`partition`が`persist:`から始まっている場合、同じ`partition`のアプリ内のすべてのページに永続セッションを提供するのにページが使います。`persist:`プレフィックスが無い場合、ページはインメモリセッションを使います。`partition`が空の場合、アプリの既定のセッションを返します。

## プロパティ

`session`モジュールは次のプロパティを持ちます:

### session.defaultSession

アプリの既定のセッションオブジェクトを返します。

## クラス: Session

`session`モジュールで、`Session`オブジェクトを作成できます:

```javascript
const session = require('electron').session;

var ses = session.fromPartition('persist:name');
```

### インスタンスイベント

`Session`のインスタンス上で次のイベントが提供されます:

#### イベント: 'will-download'

* `event` Event
* `item` [DownloadItem](download-item.md)
* `webContents` [WebContents](web-contents.md)

Electronが`webContents`で`item`をダウンロードしようとすると出力されます。

`event.preventDefault()` をコールするとダウンロードをキャンセルできます。

```javascript
session.defaultSession.on('will-download', function(event, item, webContents) {
  event.preventDefault();
  require('request')(item.getURL(), function(data) {
    require('fs').writeFileSync('/somewhere', data);
  });
});
```

### インスタンスのメソッド

`Session`のインスタンス上で次のメソッドが提供されています:

### `ses.cookies`

`cookies`は、cookiesに問い合わせしたり、修正をできるようにします。例：

```javascript
// Query all cookies.
session.defaultSession.cookies.get({}, function(error, cookies) {
  console.log(cookies);
});

// Query all cookies associated with a specific url.
session.defaultSession.cookies.get({ url : "http://www.github.com" }, function(error, cookies) {
  console.log(cookies);
});

// Set a cookie with the given cookie data;
// may overwrite equivalent cookies if they exist.
var cookie = { url : "http://www.github.com", name : "dummy_name", value : "dummy" };
session.defaultSession.cookies.set(cookie, function(error) {
  if (error)
    console.error(error);
});
```

### `ses.cookies.get(filter, callback)`

* `filter` Object
  * `url` String (オプション) - `url`に関連付けられているcookiesを取得します。空の場合すべてのurlのcookiesを取得します
  * `name` String (オプション) - `name`でcookiesをフィルタリングします
  * `domain` String (オプション) - `domains`のドメインまたはサブドメインに一致するcookiesを取得します
  * `path` String (オプション) - `path`に一致するパスのcookiesを取得します
  * `secure` Boolean (オプション) - Secure プロパティでcookiesをフィルターします
  * `session` Boolean (オプション) - Filters out `session`または永続cookiesを除外します
* `callback` Function

`details`に一致するすべてのcookiesを取得するためにリクエストを送信し、完了時に`callback(error, cookies)`で`callback`がコールされます。

`cookies` は`cookie`オブジェクトの配列です。

* `cookie` Object
  *  `name` String - cookieの名前
  *  `value` String - cookieの値
  *  `domain` String - cookieのドメイン
  *  `hostOnly` String - cookieがホストのみのcookieかどうか
  *  `path` String - cookieのパス
  *  `secure` Boolean - cookieがセキュアとマークされているかどうか
  *  `httpOnly` Boolean - HTTPのみとしてcookieがマークされているかどうか
  *  `session` Boolean - cookieがセッションcookieまたは有効期限付きの永続cookieかどうか
  *  `expirationDate` Double (オプション) -

cookieの有効期限をUNIX時間で何秒かを示します。セッションcookiesは提供されません。

### `ses.cookies.set(details, callback)`

* `details` Object
  * `url` String - `url`に関連付けられているcookiesを取得します。
  * `name` String - cookieの名前。省略した場合、既定では空です。
  * `value` String - cookieの名前。省略した場合、既定では空です。
  * `domain` String - cookieのドメイン。省略した場合、既定では空です。
  * `path` String - cookieのパス。 省略した場合、既定では空です。
  * `secure` Boolean - cookieをセキュアとしてマークする必要があるかどうか。既定ではfalseです。
  * `session` Boolean - cookieをHTTPのみとしてマークする必要があるかどうか。既定ではfalseです。
  * `expirationDate` Double -	cookieの有効期限をUNIX時間で何秒か。省略した場合、cookieはセッションcookieになります。
* `callback` Function

`details`でcookieを設定し、完了すると`callback(error)`で`callback`がコールされます。

### `ses.cookies.remove(url, name, callback)`

* `url` String - cookieに関連付けられているURL
* `name` String - 削除するcookieの名前
* `callback` Function

`url` と `name`と一致するcookiesを削除し、完了すると`callback`が、`callback()`でコールされます。

### `ses.getCacheSize(callback)`

* `callback` Function
  * `size` Integer - 使用しているキャッシュサイズバイト数

現在のセッションのキャッシュサイズを返します。

### `ses.clearCache(callback)`

* `callback` Function - 操作が完了したら、コールされます。

セッションのHTTPキャッシュをクリアします。

### `ses.clearStorageData([options, ]callback)`

* `options` Object (オプション)
  * `origin` String - `window.location.origin`の説明で、`scheme://host:port`に従う
  * `storages` Array - クリアするストレージの種類で、次を含められます:
    `appcache`、 `cookies`、 `filesystem`、 `indexdb`、 `local storage`、
    `shadercache`、 `websql`、 `serviceworkers`
  * `quotas` Array - クリアするクォーターの種類で、次を含められます:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - 操作をするとコールされます。

ウェブストレージのデータをクリアします。

### `ses.flushStorageData()`

書き込まれていないDOMStorageデータをディスクに書き込みます。

### `ses.setProxy(config, callback)`

* `config` Object
  * `pacScript` String - PACファイルに関連付けらえたURL
  * `proxyRules` String - 使用するプロキシを指定するルール
* `callback` Function - 操作をするとコールされます。

プロキシ設定を設定します。

`pacScript` と `proxyRules`が一緒に渡されたら、`proxyRules`オプションは無視され、`pacScript`設定が適用されます。

 `proxyRules`はつふぃのルールに従います。

```
proxyRules = schemeProxies[";"<schemeProxies>]
schemeProxies = [<urlScheme>"="]<proxyURIList>
urlScheme = "http" | "https" | "ftp" | "socks"
proxyURIList = <proxyURL>[","<proxyURIList>]
proxyURL = [<proxyScheme>"://"]<proxyHost>[":"<proxyPort>]
```

具体例:

* `http=foopy:80;ftp=foopy2` - `http://`URLは`foopy:80`HTTPプロキシを使用し、`ftp://`URLは`foopy2:80` HTTPプロキシを使用します。
* `foopy:80` - 全てのURLで`foopy:80`を使用します。
* `foopy:80,bar,direct://` - 全てのURLで`foopy:80`HTTPプロキシを使用し、`foopy:80`が提供されていなければ`bar`を使用し、さらに使えない場合はプロキシを使いません。
* `socks4://foopy` - 全てのURLでSOCKS  `foopy:1080`プロキシを使います。
* `http=foopy,socks5://bar.com` - http URLで`foopy`HTTPプロキシを使い、`foopy`が提供されていなければ、SOCKS5 proxy `bar.com`を使います。
* `http=foopy,direct://` - 　http URLで`foopy`HTTPプロキシを使い、`foopy`が提供されていなければ、プロキシを使いません。
* `http=foopy;socks=foopy2` -  http URLで`foopy`HTTPプロキシを使い、それ以外のすべてのURLで`socks4://foopy2`を使います。

### `ses.resolveProxy(url, callback)`

* `url` URL
* `callback` Function

`url`をプロキシ情報で解決します。リクエストが実行された時、`callback(proxy)`で `callback`がコールされます。

### `ses.setDownloadPath(path)`

* `path` String - ダウンロード場所

ダウンロードの保存ディレクトリを設定します。既定では、ダウンロードディレクトリは、個別のアプリフォルダー下の`Downloads`です。

### `ses.enableNetworkEmulation(options)`

* `options` Object
  * `offline` Boolean - ネットワーク停止を再現するかどうか
  * `latency` Double - RTT  ms秒
  * `downloadThroughput` Double - Bpsでのダウンロード割合
  * `uploadThroughput` Double - Bpsでのアップロード割合

再現ネットワークは、`session`用の設定を付与します。

```javascript
// To emulate a GPRS connection with 50kbps throughput and 500 ms latency.
window.webContents.session.enableNetworkEmulation({
    latency: 500,
    downloadThroughput: 6400,
    uploadThroughput: 6400
});

// To emulate a network outage.
window.webContents.session.enableNetworkEmulation({offline: true});
```

### `ses.disableNetworkEmulation()`

`session`ですでに有効になっているネットワークエミュレーションを無効化します。オリジナルのネットワーク設定にリセットします。

### `ses.setCertificateVerifyProc(proc)`

* `proc` Function

`session`の証明書検証ロジックを設定し、サーバー証明書確認がリクエストされた時、`proc(hostname, certificate, callback)`で`proc`がコールされます。`callback(true)`がコールされると証明書を受け入れ、`callback(false)`がコールされると拒否します。

Calling `setCertificateVerifyProc(null)`をコールして、既定の証明書検証ロジックに戻します。

```javascript
myWindow.webContents.session.setCertificateVerifyProc(function(hostname, cert, callback) {
  if (hostname == 'github.com')
    callback(true);
  else
    callback(false);
});
```

### `ses.webRequest`

`webRequest`APIセットをインターセプトし、そのライフタイムの様々な段階でリクエストの内容を変更できます。

APIのイベントが発生したとき、それぞれのAPIはオプションで`filter`と `listener`を受け入れ、`listener(details)` で`listener`がコールされ、`details`はリクエストを説明するオブジェクトです。`listener`に`null`が渡されるとイベントの購読をやめます。

`filter`は`urls`プロパティを持つオブジェクトで、URLパターンにマッチしないリクエストを除外するのに使われるURLパターンの配列です。`filter`を省略した場合、全てのリクエストにマッチします。

いくつかのイベントで`callback`で`listener`に渡され、`listener`が動作するとき、`response`オブジェクトでコールされる必要があります。

```javascript
// Modify the user agent for all requests to the following urls.
var filter = {
  urls: ["https://*.github.com/*", "*://electron.github.io"]
};

session.defaultSession.webRequest.onBeforeSendHeaders(filter, function(details, callback) {
  details.requestHeaders['User-Agent'] = "MyAgent";
  callback({cancel: false, requestHeaders: details.requestHeaders});
});
```

### `ses.webRequest.onBeforeRequest([filter, ]listener)`

* `filter` Object
* `listener` Function

リクエストが発生しようとしている時、`listener(details, callback)`で`listener` がコールされます。

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `uploadData` Array (オプション)
  * `callback` Function

`uploadData`は `data`オブジェクトの配列です。

* `data` Object
  * `bytes` Buffer - 送信されるコンテンツ
  * `file` String - アップロードされるファイルパス

`callback`は`response`オブジェクトでコールされる必要があります:

* `response` Object
  * `cancel` Boolean (オプション)
  * `redirectURL` String (オプション) - オリジナルリクエストが送信もしくは完了するのを中断し、代わりに付与したURLにリダイレクトします。

### `ses.webRequest.onBeforeSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

リクエストヘッダーが提供されれば、HTTPリクエストが送信される前に、`listener(details, callback)`で`listener`がコールされます。TCP接続がサーバーに対して行われた後に発生することがありますが、HTTPデータは送信前です。

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object
* `callback` Function

The `callback` has to be called with an `response` object:

* `response` Object
  * `cancel` Boolean (オプション)
  * `requestHeaders` Object (オプション) - 付与されると、リクエストはそれらのヘッダーで作成されます。

### `ses.webRequest.onSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

サーバーにリクエストを送信しようする直前に`listener(details)`で、`listener` がコールされます。前回の`onBeforeSendHeaders`レスポンスの変更箇所は、このリスナーが起動した時点で表示されます。

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object

### `ses.webRequest.onHeadersReceived([filter,] listener)`

* `filter` Object
* `listener` Function

リクエストのHTTPレスポンスヘッダーを受信したとき、`listener`は`listener(details, callback)`でコールされます。

* `details` Object
  * `id` String
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `statusLine` String
  * `statusCode` Integer
  * `responseHeaders` Object
* `callback` Function

`callback`は`response`オブジェクトでコールされる必要があります:

* `response` Object
  * `cancel` Boolean
  * `responseHeaders` Object (オプション) - 付与されていると、これらのヘッダーでサーバーはレスポンスしたと仮定します。

### `ses.webRequest.onResponseStarted([filter, ]listener)`

* `filter` Object
* `listener` Function

レスポンスボディの最初のバイトを受信したとき、`listener` は`listener(details)` でコールされます。HTTPリクエストでは、ステータス行とレスポンスヘッダーを意味します。

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `responseHeaders` Object
  * `fromCache` Boolean  -ディスクキャッシュから取得したレスポンスかどうかを示します
  * `statusCode` Integer
  * `statusLine` String

### `ses.webRequest.onBeforeRedirect([filter, ]listener)`

* `filter` Object
* `listener` Function

サーバーがリダイレクトを開始しはじめたとき、`listener(details)`で`listener` がコールされます。

* `details` Object
  * `id` String
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `redirectURL` String
  * `statusCode` Integer
  * `ip` String (オプション) - 実際にリクエストが送信されるサーバーIPアドレス
  * `fromCache` Boolean
  * `responseHeaders` Object

### `ses.webRequest.onCompleted([filter, ]listener)`

* `filter` Object
* `listener` Function

リクエスト完了時、`listener`が`listener(details)`でコールされます。

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `responseHeaders` Object
  * `fromCache` Boolean
  * `statusCode` Integer
  * `statusLine` String

### `ses.webRequest.onErrorOccurred([filter, ]listener)`

* `filter` Object
* `listener` Function

エラー発生時、 `listener(details)` で`listener`がコールされます。

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `fromCache` Boolean
  * `error` String - エラーの説明
