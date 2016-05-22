#サポートしているChromeコマンドラインスイッチ

> Electronでサポートされているコマンドラインスイッチ

アプリケーションのメインスクリプトで[app.commandLine.appendSwitch][append-switch]を使うことで、[app][app]モジュールの[ready][ready]イベントが発行される前にコマンドラインスイッチを追加できます。

```javascript
const {app} = require('electron');
app.commandLine.appendSwitch('remote-debugging-port', '8315');
app.commandLine.appendSwitch('host-rules', 'MAP * 127.0.0.1');

app.on('ready', () => {
  // Your code here
});
```


## --ignore-connections-limit=`domains`

`domains`で指定されたリストは接続数制限を無視します。リストは`,`で区切られます。

## --disable-http-cache

HTTPリクエストのディスクキャッシュの無効化。

## --disable-http2

HTTP/2 と SPDY/3.1 プロトコルを無効にします。

## --remote-debugging-port=`port`

`port`で指定したHTTP越しのリモートデバッグの有効化。

## --js-flags=`flags`

JSエンジンに渡されるフラグの指定。メインプロセスで`flags`を有効化したいのなら、Electron開始時に渡される必要があります。

```bash
$ electron --js-flags="--harmony_proxies --harmony_collections" your-app
```

## --proxy-server=`address:port`

システム設定を上書きし、指定したプロキシサーバーを使用します。HTTPS、WebSocketリクエストを含むHTTPプロトコルのリクエストのみに影響します。全てのプロキシサーバーがHTTPSとWebSocketリクエストに対応しているわけではないことに注意してください。

## --proxy-bypass-list=`hosts`

プロキシを使用しないサーバーをセミコロンで区切って指定します。
このフラグは、`--proxy-server`と同時に使われるときのみに影響します。

例:

```javascript
app.commandLine.appendSwitch('proxy-bypass-list', '<local>;*.google.com;*foo.com;1.2.3.4:5678')
```

ローカルアドレス（`localhost`や`127.0.0.1`など）、`google.com`サブドメイン、`foo.com` サフィックスを含むホスト、`1.2.3.4:5678`を除いてすべてのホストでプロキシサーバーが使われます。

## --proxy-pac-url=`url`

`url`で指定したPACスクリプトが使われます。

## --no-proxy-server

プロキシサーバーを使わず、常に直接接続します。ほかのプロキシサーバーフラグを上書きします。

## --host-rules=`rules`

ホスト名がどのようにマップされているかを制御するコンマで分割された`rules`一覧

例:

* `MAP * 127.0.0.1` 全てのホスト名を127.0.0.1にマッピングするよう強制します。
* `MAP *.google.com proxy` すべてのgoogle.comサブドメインを "proxy"で解決するよう強制します。
* `MAP test.com [::1]:77` "test.com"をIPv6ループバックで解決するよう強制します。結果ポートのをソケットアドレス77番にするよう強制します。
* `MAP * baz, EXCLUDE www.google.com` 全てを"baz"に再マッピングし、  "www.google.com"は除外します。

これらのマッピングは、ネットリクエスト（直接接続で、TCP接続とホスト解決とHTTPプロキシ接続での`CONNECT`、`SOCKS`プロキシ接続でのエンドポイントホスト）でエンドポイントホストに適用されます。

## --host-resolver-rules=`rules`

`--host-rules`のようですが、`rules` はホスト解決のみに適用されます。

## --ignore-certificate-errors

証明書関連エラーを無視します。

## --ppapi-flash-path=`path`

pepper flash pluginの`path`を設定します。

## --ppapi-flash-version=`version`

pepper flash pluginの`version`を設定します。

## --log-net-log=`path`

ネットログイベントを保存し、`path`に書き込みを有効化します。

## --ssl-version-fallback-min=`version`

TLSフォールバックを許可する最小のSSL/TLSバージョン ("tls1"や"tls1.1" 、 "tls1.2")を設定します。

## --cipher-suite-blacklist=`cipher_suites`

無効にするために、SSL暗号スイートのカンマ区切りのリストを指定します。

## --disable-renderer-backgrounding

不可視のページのレンダラープロセスの優先度を下げることからChromiumを防ぎます。

このフラグは、グローバルですべてのレンダラープロセスに有効で、一つのウィンドウだけで無効化したい場合、[無音を再生する][play-silent-audio]というハックで対応します。

## --enable-logging

コンソールにChromiumのログを出力します。

ユーザーのアプリが読み込まれる前に解析されるため、`app.commandLine.appendSwitch`では使用できませんが、`ELECTRON_ENABLE_LOGGING`を環境変数に設定すると同じ効果を得ることができます。

## --v=`log_level`

標準のloggingレベルを設定します。0が既定です。通常、V-loggingレベルには正の値が使用されます。

`--enable-logging` が渡された時だけ、このスイッチは動作します。

## --vmodule=`pattern`

`--v`で付与された値を上書きするために、モジュール毎の最大V-loggingレベルを付与します。例えば、 `my_module=2,foo*=3` は、`my_module.*` と `foo*.*`のソースファイル全てのロギングレベルを変更します。

前方または後方スラッシュを含む任意のパターンは、全体のパス名だけでなく、モジュールに対してもテストとされます。例えば、`*/foo/bar/*=2`は`foo/bar`ディレクトリ下のソースファイルですべてのコードのロギングレベルを変更します。

このスイッチは、`--enable-logging`が渡された時のみ動作します。

[app]: app.md
[append-switch]: app.md#appcommandlineappendswitchswitch-value
[ready]: app.md#event-ready
[play-silent-audio]: https://github.com/atom/atom/pull/9485/files
