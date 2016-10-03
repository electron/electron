# `<webview>` タグ

> 外部ウェブコンテンツを別のプロセスとフレームで表示します。

Electronアプリ内でウェブページのような外部コンテンツを埋め込む際は、`webview`タグを使用してください。外部コンテンツは`webview`コンテナ内に閉じ込められます。
アプリケーションの埋め込みページは、外部コンテンツがどのように表示されるかを制御することができます。

`iframe`と違い、`webview`はあなたのアプリと別のプロセスで動作します。あなたのウェブページとは違う実行許可レベルを持っており、あなたのアプリと埋め込みコンテンツの間のデータのやり取りは非同期で行われます。これにより、あなたのアプリケーションは埋め込みコンテンツからの安全が確保されます。

セキュリティ的な理由で、`webview`は`nodeIntegration`が有効な`BrowserWindow`でしか使用できません。

## 例

ウェブページをあなたのアプリに埋め込むには、`webview`タグをあなたの埋め込みページ(外部コンテンツを表示するアプリのページ)に追加してください。
もっとも単純な方法では、`webview`タグはウェブページの`src`と、見た目を制御する`style`を持っています。

```html
<webview id="foo" src="https://www.github.com/" style="display:inline-flex; width:640px; height:480px"></webview>
```

もし、外部コンテンツを制御したい場合、`webview`イベントを受け取り、`webview`メソッドでそれらのイベントに返答するJavaScriptコードを書くことになります。
下記にあるのはウェブページの読み込みが開始した時と、止まった時にイベントを受け取るサンプルで、読み込み中に"loading..."というメッセージを表示します。

```html
<script>
  onload = () => {
    const webview = document.getElementById('foo');
    const indicator = document.querySelector('.indicator');

    const loadstart = () => {
      indicator.innerText = 'loading...';
    };

    const loadstop = () => {
      indicator.innerText = '';
    };

    webview.addEventListener('did-start-loading', loadstart);
    webview.addEventListener('did-stop-loading', loadstop);
  };
</script>
```

## CSS Styling Notes

もし、flexbox layouts(v.0.36.11以降)を使用する場合は、`webview`タグは子`object`要素が`webview`自体の高さと幅いっぱいとなるよう、内部で`display: flex;`を使用しているのに注意してください。
インラインレイアウトとしたい時に`display: inline-flex;`を指定する以外には、この`display: flex;`は上書きしないでください。

`webview`は`hidden`属性か、`display: none;`と使用して非表示にする際に、いくつか問題があります。
`browserplugin`オブジェクトの中での描画や、ウェブページを再度表示するために再読み込みした際などに通常とは異なる描画となる場合があります。
`webview`を隠すオススメの方法としては、`width`と`height`をゼロに設定し、`flex`を通じて、0pxまで小さくできるようにします。

```html
<style>
  webview {
    display:inline-flex;
    width:640px;
    height:480px;
  }
  webview.hide {
    flex: 0 1;
    width: 0px;
    height: 0px;
  }
</style>
```

## Tag 属性

`webview`タグは下記のような属性を持っています：

### `src`

```html
<webview src="https://www.github.com/"></webview>
```

表示されているURLを返します。本属性への書き込みは、トップレベルナビゲーションを開始させます。

`src`の値を現在の値に再度設定することで、現在のページを再読み込みさせることができます。

また、`src`属性はdata URLを指定することができます(例: `data:text/plain,Hello, world!`)。

### `autosize`

```html
<webview src="https://www.github.com/" autosize="on" minwidth="576" minheight="432"></webview>
```

"on"の際は、`minwidth`, `minheight`, `maxwidth`, `maxheight`で設定された範囲内で、自動的に大きさが変化します。
これらの制限値は、`autosize`が有効でない場合は影響しません。
`autosize`が有効の際は、`webview`コンテナサイズは指定した最小値より小さくなりませんし、最大値より大きくなりません。

### `nodeintegration`

```html
<webview src="http://www.google.com/" nodeintegration></webview>
```

"on"の際は、`webview`内のゲストページ(外部コンテンツ)でnode integrationが有効となり、`require`や`process`と行ったnode APIでシステムリソースにアクセスできるようになります。1

**注意:** 親ウィンドウでnode integrationが無効の場合は、`webview`でも常に無効になります。


### `plugins`

```html
<webview src="https://www.github.com/" plugins></webview>
```

"on"の際は、ゲストページはブラウザプラグインを使用できます。

### `preload`

```html
<webview src="https://www.github.com/" preload="./test.js"></webview>
```

ゲストページ上のスクリプトより先に実行されるスクリプトを指定してください。内部では、ゲストページ内で`require`で読み込まれるので、スクリプトのURLのプロトコルはは`file:`または`asar:`でなければなりません。

ゲストページがnode integrationが無効の場合でも、このスクリプトは全てのNode APIにアクセスできます。ただし、グローバルオブジェクトはこのスクリプトの実行終了後にすべて削除されます。

### `httpreferrer`

```html
<webview src="https://www.github.com/" httpreferrer="http://cheng.guru"></webview>
```

ゲストページのためにリファラを設定します。

### `useragent`

```html
<webview src="https://www.github.com/" useragent="Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko"></webview>
```

ページ遷移の前に、User agentを指定します。すでにページが読み込まれている場合は、`setUserAgent`メソッドを利用して変更してください。

### `disablewebsecurity`

```html
<webview src="https://www.github.com/" disablewebsecurity></webview>
```

"on"の際は、ゲストページのウェブセキュリティは無効となります。

### `partition`

```html
<webview src="https://github.com" partition="persist:github"></webview>
<webview src="http://electron.atom.io" partition="electron"></webview>
```

ページで使用されるセッションを設定します。もし、`partition`が`persist:`から始まる場合、アプリ上の同じ`partition`を指定した全てのページで有効な永続セッションを使用します。
`persist:`接頭辞なしで指定した場合、メモリ内セッションを使用します。同じセッションを指定すると複数のページで同じセッションを使用できます。
`partition`を設定しない場合は、アプリケーションのデフォルトセッションが使用されます。

レンダラプロセスのセッションは変更できないため、この値は最初のページ遷移の前に変更されないといけません。
その後に変更をしようとしても、DOM例外を起こすことになります。

### `allowpopups`

```html
<webview src="https://www.github.com/" allowpopups></webview>
```

"on"の場合、ゲストページは新しいウィンドウを開くことができます。

### `blinkfeatures`

```html
<webview src="https://www.github.com/" blinkfeatures="PreciseMemoryInfo, CSSVariables"></webview>
```

有効にしたいblink featureを`,`で区切って指定します。
サポートされている全ての機能は、[setFeatureEnabledFromString][blink-feature-string]にリストがあります。

## メソッド

`webview`タグは、下記のようなメソッドを持っています：

**注意:** webview要素はメソッドを使用する前に読み込まれていないといけません。

**例**

```javascript
webview.addEventListener('dom-ready', () => {
  webview.openDevTools()
})
```

### `<webview>.loadURL(url[, options])`

* `url` URL
* `options` Object (optional)
  * `httpReferrer` String - リファラURL 
  * `userAgent` String - リクエストに使用されるUser agent
  * `extraHeaders` String - 追加のヘッダを"\n"で区切って指定します。

`url`をwebviewで読み込みます。`url`はプロトコル接頭辞(`http://`や`file://`など)を含んでいなければいけません。

### `<webview>.getURL()`

ゲストページのURLを取得します。

### `<webview>.getTitle()`

ゲストページのタイトルを返します。

### `<webview>.isLoading()`

ゲストページが読み込み中かのbool値を返します。

### `<webview>.isWaitingForResponse()`

ゲストページがページの最初の返答を待っているかのbool値を返します。

### `<webview>.stop()`

実行待ち中のナビゲーションを中止します。

### `<webview>.reload()`

ゲストページを再読み込みします。

### `<webview>.reloadIgnoringCache()`

キャッシュを無効にして再読み込みします。

### `<webview>.canGoBack()`

ページを戻ることができるかのbool値を返します。

### `<webview>.canGoForward()`

ページを進むことができるかのbool値を返します。

### `<webview>.canGoToOffset(offset)`

* `offset` Integer

`offset`だけ、移動できるかのbool値を返します。

### `<webview>.clearHistory()`

移動履歴をクリアします。

### `<webview>.goBack()`

ページを戻ります。

### `<webview>.goForward()`

ページを進みます。

### `<webview>.goToIndex(index)`

* `index` Integer

インデックスを指定して移動します。

### `<webview>.goToOffset(offset)`

* `offset` Integer

現在の場所からの移動量を指定して移動します。

### `<webview>.isCrashed()`

レンダラプロセスがクラッシュしているかを返します。

### `<webview>.setUserAgent(userAgent)`

* `userAgent` String

ゲストページ用のUser agentを変更します。

### `<webview>.getUserAgent()`

User agentを取得します。

### `<webview>.insertCSS(css)`

* `css` String

ゲストエージにCSSを追加します。

### `<webview>.executeJavaScript(code, userGesture, callback)`

* `code` String
* `userGesture` Boolean - Default `false`.
* `callback` Function (optional) - Called after script has been executed.
  * `result`

ページ内で`code`を評価します。`userGesture`を設定した場合、ページ内でgesture contextを作成します。例えば`requestFullScreen`のようなユーザーの対応が必要なHTML APIでは、自動化時にこれが有利になる時があります。

### `<webview>.openDevTools()`

DevToolsを開きます。

### `<webview>.closeDevTools()`

DevToolsを閉じます。

### `<webview>.isDevToolsOpened()`

DevToolsが開いているかのbool値を返します。

### `<webview>.isDevToolsFocused()`

DevToolsがフォーカスを得ているかのbool値を返します。

### `<webview>.inspectElement(x, y)`

* `x` Integer
* `y` Integer

ゲストページの(`x`, `y`)の位置にある要素を調べます。

### `<webview>.inspectServiceWorker()`

ゲストページのサービスワーカーのDevToolsを開きます。

### `<webview>.setAudioMuted(muted)`

* `muted` Boolean

ページをミュートするかを設定します。

### `<webview>.isAudioMuted()`

ページがミュートされているかを返します。

### `<webview>.undo()`

`undo` (元に戻す)を行います。
Executes editing command `undo` in page.

### `<webview>.redo()`

`redo` (やり直し)を行います。

### `<webview>.cut()`

`cut` (切り取り)を行います。

### `<webview>.copy()`

`copy` (コピー)を行います。

### `<webview>.paste()`

`paste` (貼り付け)を行います。

### `<webview>.pasteAndMatchStyle()`

`pasteAndMatchStyle`(貼り付けてスタイルを合わせる)を行います。

### `<webview>.delete()`

`delete` (削除)を行います。

### `<webview>.selectAll()`

`selectAll` (全て選択)を行います。

### `<webview>.unselect()`

`unselect` (選択を解除)を行います。

### `<webview>.replace(text)`

* `text` String

`replace` (置き換え)を行います。

### `<webview>.replaceMisspelling(text)`

* `text` String

`replaceMisspelling` (スペル違いを置き換え)を行います。

### `<webview>.insertText(text)`

* `text` String

`text`を選択された要素に挿入します。

### `<webview>.findInPage(text[, options])`

* `text` String - 検索する文字列(空文字列にはできません)
* `options` Object (省略可)
  * `forward` Boolean - 前方・後方のどちらを検索するかどうかです。省略時は`true`で前方に検索します。
  * `findNext` Boolean - 初回検索か、次を検索するかを選びます。省略時は`false`で、初回検索です。
  * `matchCase` Boolean - 大文字・小文字を区別するかを指定します。省略時は`false`で、区別しません。
  * `wordStart` Boolean - ワード始まりからの検索かを指定します。省略時は`false`で、語途中でもマッチします。
  * `medialCapitalAsWordStart` Boolean - `wordStart`指定時、CamelCaseの途中もワード始まりと見なすかを指定します。省略時は`false`です。

`text`をページ内全てから検索し、リクエストに使用するリクエストID(`Integer`)を返します。リクエストの結果は、[`found-in-page`](web-view-tag.md#event-found-in-page)イベントを介して受け取ることができます。

### `<webview>.stopFindInPage(action)`

* `action` String - [`<webview>.findInPage`](web-view-tag.md#webviewtagfindinpage)リクエストを終わらせる時にとるアクションを指定します。
  * `clearSelection` - 選択をクリアします。
  * `keepSelection` - 選択を通常の選択へと変換します。
  * `activateSelection` - 選択ノードにフォーカスを当て、クリックします。

`action`にしたがって`webview`への`findInPage`リクエストを中止します。

### `<webview>.print([options])`

`webview`で表示されているウェブページを印刷します。`webContents.print([options])`と同じです。

### `<webview>.printToPDF(options, callback)`

`webview`のウェブサイトをPDFとして印刷します。`webContents.printToPDF(options, callback)`と同じです。

### `<webview>.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `arg` (optional)

`channel`を通じてレンダラプロセスに非同期メッセージを送ります。合わせて、
任意のメッセージを送ることができます。レンダラプロセスは`ipcRenderer`モジュールを
使用して、`channel`イベントでメッセージを把握することができます。

サンプルが[webContents.send](web-contents.md#webcontentssendchannel-args)にあります。

### `<webview>.sendInputEvent(event)`

* `event` Object

入力イベント(`event`)をページに送ります。

`event`に関する詳しい説明は、[webContents.sendInputEvent](web-contents.md##webcontentssendinputeventevent)を
参照してください。

### `<webview>.getWebContents()`

`webview`に関連付けられた[WebContents](web-contents.md)を取得します。

## DOM events

`webview`タグで有効なイベントは次の通りです：

### Event: 'load-commit'

返り値:

* `url` String
* `isMainFrame` Boolean

読み込みが行われる時に発生するイベントです。
これには、現在のドキュメントやサブフレームの読み込みが含まれます。ただし、非同期なリソース読み込みは含まれません。

### Event: 'did-finish-load'

ナビゲーションが完了した際に発生するイベントです。
言い換えると、タブ上の"くるくる"が止まった時に発生し、`onload`イベントが配信されます。

### Event: 'did-fail-load'

返り値:

* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String
* `isMainFrame` Boolean

`did-finish-load`と同じようですが、読み込みに失敗した時やキャンセルされた時に発生します。
例えば、`window.stop()`が呼ばれた時などです。

### Event: 'did-frame-finish-load'

返り値:

* `isMainFrame` Boolean

フレームがナビゲーションを終えた時に発生します。

### Event: 'did-start-loading'

タブ上の"くるくる"が回転を始めた時点でこのイベントが発生します。

### Event: 'did-stop-loading'

タブ上の"くるくる"が回転をやめた時点でこのイベントが発生します。

### Event: 'did-get-response-details'

返り値:

* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

読み込むリソースの詳細がわかった時点で発生します。
`status`は、リソースをダウンロードするソケット接続であるかを示します。

### Event: 'did-get-redirect-request'

返り値:

* `oldURL` String
* `newURL` String
* `isMainFrame` Boolean

リソースの取得中に、リダイレクトを受け取った際に発生します。

### Event: 'dom-ready'

該当フレーム内のドキュメントが読み込まれた時に発生します。

### Event: 'page-title-updated'

返り値:

* `title` String
* `explicitSet` Boolean

ページのタイトルが設定された時に発生します。
タイトルがurlから作られたものであれば、`explicitSet`は`false`になります。

### Event: 'page-favicon-updated'

返り値:

* `favicons` Array - URLの配列

ページのfavicon URLを受け取った時に発生します。

### Event: 'enter-html-full-screen'

HTML APIでフルスクリーンになった際に発生します。

### Event: 'leave-html-full-screen'

HTML APIでフルスクリーンでなくなった際に発生します。

### Event: 'console-message'

返り値:

* `level` Integer
* `message` String
* `line` Integer
* `sourceId` String

ゲストウィンドウがコンソールメッセージを記録する際に発生します。

下記のサンプルは、埋め込まれたサイトのログを、log levelに関係なく親側に転送します。


```javascript
webview.addEventListener('console-message', (e) => {
  console.log('Guest page logged a message:', e.message)
})
```

### Event: 'found-in-page'

返り値:

* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - 次のレスポンスが待っているかを示します。
  * `activeMatchOrdinal` Integer (optional) - このマッチの場所を示します。
  * `matches` Integer (optional) - マッチした数です。
  * `selectionArea` Object (optional) - 最初のマッチした場所です。

[`webview.findInPage`](web-view-tag.md#webviewtagfindinpage)リクエストで結果が得られた場合に発生します。

```javascript
webview.addEventListener('found-in-page', (e) => {
  if (e.result.finalUpdate)
    webview.stopFindInPage('keepSelection')
})

const requestId = webview.findInPage('test')
```

### Event: 'new-window'

返り値:

* `url` String
* `frameName` String
* `disposition` String -`default`, `foreground-tab`, `background-tab`,
  `new-window`, `other`のどれかです。
* `options` Object - 新しい`BrowserWindow`を作る際に使用されるoptionです。

ゲストページが新しいブラウザウィンドウを開こうとする際に発生します。

下記のサンプルは、新しいURLをシステムのデフォルトブラウザで開きます。

```javascript
const {shell} = require('electron')

webview.addEventListener('new-window', (e) => {
  const protocol = require('url').parse(e.url).protocol
  if (protocol === 'http:' || protocol === 'https:') {
    shell.openExternal(e.url)
  }
})
```

### Event: 'will-navigate'

返り値:

* `url` String

ユーザーまたはページがナビゲーションを始めようとする際に発生します。これは、
`window.location`が変更になる時や、ユーザーがリンクをクリックした際に発生します。

`<webview>.loadURL`や`<webview>.back`でプログラムによりナビゲーションが始まった場合は
このイベントは発生しません。

アンカーリンクのクリックや`window.location.hash`の変更といった、ページ内遷移でも、このイベントは発生しません。
この場合は、`did-navigate-in-page`イベントを使ってください。

`event.preventDefault()`は使用しても__何も起こりません__。

### Event: 'did-navigate'

返り値:

* `url` String

ナビゲーション終了時に呼ばれます。

アンカーリンクのクリックや`window.location.hash`の変更といった、ページ内遷移では、このイベントは発生しません。
この場合は、`did-navigate-in-page`イベントを使ってください。

### Event: 'did-navigate-in-page'

返り値:

* `url` String

ページ内遷移の際に発生します。

ページ内遷移の際は、ページURLは変更になりますが、ページ外部へのナビゲーションは発生しません。
例として、アンカーリンクのクリック時や、DOMの`hashchange`イベントが発生した時にこれが起こります。

### Event: 'close'

ゲストページそのものが、閉じられようとしている際に発生します。

下記のサンプルは、`webview`が閉じられる際に、`about:blank`にナビゲートします。

```javascript
webview.addEventListener('close', () => {
  webview.src = 'about:blank'
})
```

### Event: 'ipc-message'

返り値:

* `channel` String
* `args` Array

ゲストページが埋め込み元に非同期メッセージを送ってきた際に発生します。

`sendToHost`メソッドと、`ipc-message`イベントを利用すると、ゲストページと埋め込み元のページでデータのやり取りを簡単に行うことができます。

```javascript
// 埋め込み元ページ(<webview>があるページ)で
webview.addEventListener('ipc-message', (event) => {
  console.log(event.channel)
  // Prints "pong"
})
webview.send('ping')
```

```javascript
// ゲストページ(<webview>内)で
const {ipcRenderer} = require('electron')
ipcRenderer.on('ping', () => {
  ipcRenderer.sendToHost('pong')
})
```

### Event: 'crashed'

プロセスがクラッシュした際に発生します。

### Event: 'gpu-crashed'

GPUプロセスがクラッシュした際に発生します。

### Event: 'plugin-crashed'

返り値:

* `name` String
* `version` String

プラグインプロセスがクラッシュした際に発生します。

### Event: 'destroyed'

WebContentsが破壊された際に呼ばれます。

### Event: 'media-started-playing'

メディアの再生が開始された際に呼ばれます。

### Event: 'media-paused'

メディアが一時停止になるか、再生を終えた際に呼ばれます。

### Event: 'did-change-theme-color'

返り値:

* `themeColor` String

ページのテーマカラーが変更になった際に呼ばれます。
これは、下記のようなメタタグがある際に通常発生します：

```html
<meta name='theme-color' content='#ff0000'>
```

### Event: 'devtools-opened'

DevToolsが開かれた際に発生します。

### Event: 'devtools-closed'

DevToolsが閉じられた際に発生します。

### Event: 'devtools-focused'

DevToolsにフォーカスが当たった際 / 開かれた際に発生します。

[blink-feature-string]: https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/blink/platform/RuntimeEnabledFeatures.cpp&sq=package:chromium&type=cs&l=527
