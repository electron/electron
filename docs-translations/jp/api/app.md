# app

 `app` モジュールは、アプリケーションのライフサイクルコントロールを担います。

次の例は、最後のウィンドウが閉じたときにアプリケーションを終了させる方法を示しています。

```javascript
const app = require('electron').app
app.on('window-all-closed', function () {
  app.quit()
})
```

## イベント

`app` オブジェクトは次のイベントを出力します。

### イベント: 'will-finish-launching'

アプリケーションの基礎起動が終わったときに出力されます。Windows と Linuxでは、 `will-finish-launching` イベントと`ready`イベントは同じです。macOSでは、`NSApplication`の `applicationWillFinishLaunching` 通知をに相当します。通常、`open-file`と`open-url` 用のリスナーの設定、クラッシュレポートの開始、自動アップデートをします。

ほとんどの場合、 `ready` イベントハンドラーですべてをするべきです。

### イベント: 'ready'

Electronの初期化が終わった時に出力します。

### イベント: 'window-all-closed'

全てのウィンドウを閉じたときに出力します。

このイベントは、アプリケーションが終了する予定ではないときのみ出力します。ユーザーが `Cmd + Q`を押したり、開発者が`app.quit()`をコールすると、Electronは最初にすべてのウィンドウをクローズしようとし、`will-quit`イベントを出力します。この場合、`window-all-closed`イベントは出力されません。

### イベント: 'before-quit'

戻り値:

* `event` Event

アプリケーションがウィンドウをクローズし始める前に出力します。`event.preventDefault()`をコールすると、アプリケーションを終了させる既定の挙動を止めることができます。

### イベント: 'will-quit'

戻り値:

* `event` Event

全てのウィンドウが閉じて、アプリケーションを終了するときに出力します。`event.preventDefault()`をコールすると、アプリケーションを終了させる既定の挙動を止めることができます。

詳細は、`will-quit`イベント と `window-all-closed` イベントの違いは、`window-all-closed` イベントの説明を見てください。

### イベント: 'quit'

戻り値:

* `event` Event
* `exitCode` Integer

アプリケーションが終了したときに出力されます。

### イベント: 'open-file' _macOS_

戻り値:

* `event` Event
* `path` String

アプリケーションでファイルを開こうとしたときに出力します。アプリケーションがすでに起動し、OSがファイルを開くアプリケーションを再使用したいとき、`open-file`イベントは出力します。ファイルがdockにドロップアウトされ、アプリケーションがまだ起動していないときにも`open-file` は出力します。このケースを処理するために、アプリケーションの起動のかなり早い段階で、`open-file` イベントをリッスンして確認します（まだ `ready` イベントが出力する前に）。

このイベントをハンドルしたいときには `event.preventDefault()` をコールすべきです。

Windowsでは、ファイルパスを取得するために、 `process.argv` をパースする必要があります。

### イベント: 'open-url' _macOS_

戻り値:

* `event` Event
* `url` String

アプリケーションでURLを開こうとしたときに出力されます。URLスキーマーは、アプリケーションが開くように登録しなければなりません。

このイベントをハンドルしたい場合は、`event.preventDefault()`をコールすべきです。

### イベント: 'activate' _macOS_

戻り値:

* `event` Event
* `hasVisibleWindows` Boolean

アプリケーションがアクティブになったときに出力されます。通常は、アプリケーションのドックアイコンをクリックしたときに発生します。

### イベント: 'browser-window-blur'

戻り値:

* `event` Event
* `window` BrowserWindow

[browserWindow](browser-window.md) からフォーカスが外れたときに出力されます。

### イベント: 'browser-window-focus'

戻り値:

* `event` Event
* `window` BrowserWindow

[browserWindow](browser-window.md) にフォーカスが当たったとき出力されます。

### イベント: 'browser-window-created'

戻り値:

* `event` Event
* `window` BrowserWindow

新しい [browserWindow](browser-window.md) が作成されたときに出力されます。

### イベント: 'certificate-error'

戻り値:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `error` String - The error code
* `certificate` Object
  * `data` Buffer - PEM encoded data
  * `issuerName` String
* `callback` Function

 `url` の  `certificate` 検証に失敗したときに発生します。証明書を信頼するために`event.preventDefault()` と `callback(true)`をコールして既定の動作を止める必要があります。

```javascript
session.on('certificate-error', function (event, webContents, url, error, certificate, callback) {
  if (url === 'https://github.com') {
    // Verification logic.
    event.preventDefault()
    callback(true)
  } else {
    callback(false)
  }
})
```

### イベント: 'select-client-certificate'

戻り値:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `url` URL
* `certificateList` [Objects]
  * `data` Buffer - PEM encoded data
  * `issuerName` String - Issuer's Common Name
* `callback` Function

クライアント証明書が要求されたときに出力されます。

`url` は、クライアント証明書を要求するナビゲーションエントリーに対応し、`callback` リストからエントリをフィルターしてコールするのに必要です。`event.preventDefault()` を使用して、アプリケーションの証明書ストアから最初の証明書を使用するのを止めることができます。

```javascript
app.on('select-client-certificate', function (event, webContents, url, list, callback) {
  event.preventDefault()
  callback(list[0])
})
```

### イベント: 'login'

Returns:

* `event` Event
* `webContents` [WebContents](web-contents.md)
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

`webContents` がベーシック認証をしようとしたときに出力されます。

既定の動作ではすべての認証をキャンセルしたり、`event.preventDefault()` と `callback(username, password)` とを証明書でコールし既定の動作をとめてオーバーライドします。

```javascript
app.on('login', function (event, webContents, request, authInfo, callback) {
  event.preventDefault()
  callback('username', 'secret')
})
```

### イベント: 'gpu-process-crashed'

gpu プロセスがクラッシュしたときに出力されます。

## メソッド

`app` オブジェクトは次のメソッドを持ちます。

**Note:** いくつかのメソッドは、特定のオペレーティングシステム向けに提供され、そのようにラベルで表示します。

### `app.quit()`

全てのウィンドウを閉じようとします。`before-quit`イベントは、最初に出力されます。すべてのウィンドウを閉じることに成功したら、`will-quit`イベントが出力され、既定では、アプリケーションが終了します。

このメソッドは、全ての`beforeunload`と`unload`イベントハンドラは正確に発生することを保証されます。`beforeunload` イベントハンドラで、`false`を返すことでウィンドウの終了をキャンセルすることができます。

### `app.exit(exitCode)`

* `exitCode` Integer

`exitCode`で今すぐ終了します。

全てのウィンドウは、ユーザーに確認することなく、すぐに閉じ、`before-quit`と`will-quit` イベントは出力されません。

### `app.getAppPath()`

減殺のアプリケーションディレクトリを戻します。

### `app.getPath(name)`

* `name` String

`name`に関連した特定のディレクトリやファイルへのパスを返します。失敗したら、`Error`をスローします。

`name`で次のパスをリクエストできます。

* `home` ユーザーのホームディレクトリ
* `appData` 既定で示すユーザーごとのアプリケーションディレクトリ
  * `%APPDATA%` Windows上
  * `$XDG_CONFIG_HOME` or `~/.config` Linux上
  * `~/Library/Application Support` macOS上
* `userData` アプリの設定ファイルを格納するディレクトリで、既定では`appData` ディレクトリ配下のアプリ名ディレクトリです
* `temp` 一時ディレクトリ
* `exe` 現在の実行ファイル
* `module` `libchromiumcontent` ライブラリ
* `desktop` 現在のユーザーのデスクトップディレクトリ
* `documents` ユーザーの "My Documents"用ディレクトリ
* `downloads` ユーザーのダウンロード用ディレクトリ
* `music` ユーザーのミュージック用ディレクトリ
* `pictures` ユーザーのピクチャー用ディレクトリ
* `videos` ユーザーのビデオ用ディレクトリ

### `app.setPath(name, path)`

* `name` String
* `path` String

`name`に関連した特定のディレクトリやファイルへの`path` を上書きします。存在しないパスを指定した場合、このメソッドがディレクトリを作成します。失敗したら、`Error`をスローします。

`app.getPath`で、`name` で定義されたパスを上書きできます。

既定では、webページのクッキーとキャッシュは`userData`ディレクトリ配下に格納できます。ロケーションを変更したい場合、 `app` モジュールの `ready` イベントが出力される前に`userData`パスを上書きする必要があります。

### `app.getVersion()`

ロードしたアプリケーションのバージョンを戻します。アプリケーションの `package.json`ファイルにversionが無ければ、現在のバンドルまたは実行ファイルのバージョンになります。

### `app.getName()`

現在のアプリケーション名を戻し、`package.json` ファイルのnameです。

通常、 `package.json`の`name` フィールドは、短い小文字名で、npm module spec と一致します。通常、`productName`で、アプリケーションの大文字正式名を指定し、Electronでは`name`をそれで上書きます。

### `app.getLocale()`

現在のアプリケーションのロケールを戻します。

### `app.addRecentDocument(path)` _macOS_ _Windows_

* `path` String

最近のドキュメント一覧に`path`を追加します。

この一覧はOSが管理しています。Windowsではタスクバーからこの一覧を見れ、macOSではdockメニューから見れます。

### `app.clearRecentDocuments()` _macOS_ _Windows_

最近のドキュメント一覧をクリアします。

### `app.setUserTasks(tasks)` _Windows_

* `tasks` Array - `Task` オブジェクトの配列

Windowsのジャンプリストの[Tasks][tasks]カテゴリに`tasks`を追加します。

`tasks` は`Task`オブジェクトの配列で、次のフォーマットになります。

`Task` Object:

* `program` String - 実行するプログラムのパスで、通常はプログラムが開く`process.execPath`を指定します
* `arguments` String - `program` を実行するときのコマンドライン引数です
* `title` String - ジャンプリストに表示される文字列です
* `description` String - タスクの説明
* `iconPath` String - ジャンプリストに表示するアイコンの絶対パスで、アイコンを含む任意のリソースファイルです。通常、プログラムのアイコンを表示する`process.execPath`を指定します。
* `iconIndex` Integer - アイコンファイルのアイコンインデックスです。アイコンファイルに2つ以上のアイコンが含まれている場合、この値でアイコンを指定します。1つしかアイコンファイルに含まれていない場合は、この値は0です。

### `app.allowNTLMCredentialsForAllDomains(allow)`

* `allow` Boolean

HTTP NTLMまたはNegotiate認証用の照明を常に送信するかどうかを動的に設定できます。通常、Electronはローカルインターネットサイト（例えば、あなたと同じドメイン名のとき）に該当するURL用のNTLM/Kerberos証明書のみ送信します。しかし、この検知はコーポレートネットワークの設定が悪いときには、頻繁に失敗するので、この挙動を共通に行うことを選べば、全てのURLで有効にできます。

### `app.makeSingleInstance(callback)`

* `callback` Function

このメソッドは、アプリケーションをシングルインスタンスアプリケーションにします。アプリの実行を複数のインスタンスで実行することを許可せず、アプリケーション実行をシングルインスタンスのみにすることを保証し、ほかのインスタンスにはこのインスタンスの存在を知らせ終了さえます。

2つ目のインスタンスが起動したとき、`callback` は、`callback(argv, workingDirectory)` でコールします。`argv` は、2つ目のインスタンスのコマンドライン引数の配列で、`workingDirectory` は現在のワーキングディレクトリです。通常、アプリケーションはメインのウィンドウにフォーカスをあて最小化させないことで対応します。

The `callback` は、 `app`の`ready` イベントの出力後に実行することを保証します。

プロセスがアプリケーションのプライマリインスタンスでアプリがロードし続けるなら、このメソッドは `false`を戻します。プロセスがほかのインスタンスにパラメーターを送信し、`true`を戻すと、直ちに終了します。

macOSは、ユーザーがFinderで2つ目のアプリインスタンスを開いたり、`open-file` 、 `open-url`イベントが出力しようとすると、システムが自動的にシングルインスタンスを強制します。しかし、コマンドラインでアプリを開始するとシステムのシングルインスタンスメカニズムは無視されるので、シングルインスタンスを強制するためには、このメソッドを使う必要があります。

2つ目のインスタンスを起動するとき、メインのインスタンスのウィンドウをアクティブにする例

```javascript
var myWindow = null

var shouldQuit = app.makeSingleInstance(function (commandLine, workingDirectory) {
  // Someone tried to run a second instance, we should focus our window
  if (myWindow) {
    if (myWindow.isMinimized()) myWindow.restore()
    myWindow.focus()
  }
  return true
})

if (shouldQuit) {
  app.quit()
}

app.on('ready', function () {
  // Create myWindow, load the rest of the app, etc...
})
```

### `app.setAppUserModelId(id)` _Windows_

* `id` String

[Application User Model ID][app-user-model-id] を `id`に変更します。

### `app.isAeroGlassEnabled()` _Windows_

[DWM composition](https://msdn.microsoft.com/en-us/library/windows/desktop/aa969540.aspx)(Aero Glass) が有効なら、このメソッドは`true`を戻し、有効でなければ`false`を戻します。透明なウィンドウを作成する必要があるか、またはできないとき（DWM compositionが無効なとき用明なウィンドウは正しく動作しません）、このAPIを使うことができます。

使用例:

```javascript
let browserOptions = {width: 1000, height: 800}

// Make the window transparent only if the platform supports it.
if (process.platform !== 'win32' || app.isAeroGlassEnabled()) {
  browserOptions.transparent = true
  browserOptions.frame = false
}

// Create the window.
win = new BrowserWindow(browserOptions)

// Navigate.
if (browserOptions.transparent) {
  win.loadURL(`file://${__dirname}/index.html`)
} else {
  // No transparency, so we load a fallback that uses basic styles.
  win.loadURL(`file://${__dirname}/fallback.html`)
}
```

### `app.commandLine.appendSwitch(switch[, value])`

Chromiumのコマンドラインにスイッチ（ `value`をオプションにし）を追加します。

**Note:** これは、`process.argv`に影響せず、開発者が、Chromiumのローレベルな挙動をコントロールするのに使用します。

### `app.commandLine.appendArgument(value)`

Chromiumのコマンドダインに引数を追加します。引数は正しく引用符で囲まれます。

**Note:** `process.argv`に影響しません。

### `app.dock.bounce([type])` _macOS_

* `type` String (optional) - `critical` または `informational`を指定できます。既定では、 `informational`です。

`critical`を渡すと、アプリケーションがアクティブ、もしくはリクエストがキャンセルされるまでは、dockアイコンは、バウンスします。

`informational` を渡すと、1秒dockアイコンはバウンスします。しかし、アプリケーションがアクティブ、もしくはリクエストがキャンセルされるまでは、リクエストは残ります。

リクエストを示すIDを戻します。

### `app.dock.cancelBounce(id)` _macOS_

* `id` Integer

`id`のバウンスをキャンセルします。

### `app.dock.setBadge(text)` _macOS_

* `text` String

dockのバッジエリアで表示する文字列を設定します。

### `app.dock.getBadge()` _macOS_

dockのバッジ文字列を戻します。

### `app.dock.hide()` _macOS_

dock アイコンを隠します。

### `app.dock.show()` _macOS_

dock アイコンを表示します。

### `app.dock.setMenu(menu)` _macOS_

* `menu` Menu

アプリケーションの[dock menu][dock-menu]を設定します。

### `app.dock.setIcon(image)` _macOS_

* `image` [NativeImage](native-image.md)

dock アイコンに紐づいた`image`を設定します。

[dock-menu]:https://developer.apple.com/library/mac/documentation/Carbon/Conceptual/customizing_docktile/concepts/dockconcepts.html#//apple_ref/doc/uid/TP30000986-CH2-TPXREF103
[tasks]:http://msdn.microsoft.com/en-us/library/windows/desktop/dd378460(v=vs.85).aspx#tasks
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
