# autoUpdater

このモジュールは、`Squirrel`オートアップデートフレームワークのインターフェイスを提供します。

## プラットフォーム留意点

`autoUpdater`は、異なるプラットフォーム用に統一したAPIを提供していますが、それぞれのプラットフォーム上で、まだ多少の差があります。

### macOS

macOSでは、 `autoUpdater` モジュールは、[Squirrel.Mac][squirrel-mac]上に構築されていて、動作させるのに特別な設定が不要であることを意味します。サーバーサイドの要件は、[Server Support][server-support]を読んでください。

### Windows

Windowsでは、auto-updaterを使う前に、ユーザーのPCにアプリをインストールする必要があるので、Windows インストーラーを生成するために[grunt-electron-installer][installer]モジュールを使用することをお勧めします。

Squirrelで生成されたインストーラーは、`com.squirrel.PACKAGE_ID.YOUR_EXE_WITHOUT_DOT_EXE`のフォーマット（例えば、`com.squirrel.slack.Slack` と `com.squirrel.code.Code`）で[Application User Model ID][app-user-model-id]とショートカットアイコンを作成します。`app.setAppUserModelId`APIで同じIDを使う必要があります。同じIDでないと、Windowsはタスクバーに適切にピン止めすることができません。

サーバーサイドのセットアップは、macOSと異なります。詳細は、[Squirrel.Windows][squirrel-windows] を参照してください。

### Linux

Linuxでは、auot-updater用のサポートがビルトインされていないので、アプリをアップデートするためにディストリビューションのパッケージマネジャーを使用することをお勧めします。

## イベント

`autoUpdater` オブジェクトは次のイベントを出力します。

### イベント: 'error'

戻り値:

* `error` Error

アップデート中にエラーがあった場合に出力されます。

### イベント: 'checking-for-update'

アップデートを開始したかチェックしたときに出力されます。

### イベント: 'update-available'

アップデートが提供されているときに出力されます。アップデートは自動的にダウンロードされます。

### イベント: 'update-not-available'

アップデートが提供されていないときに出力されます。

### イベント: 'update-downloaded'

戻り値:

* `event` Event
* `releaseNotes` String
* `releaseName` String
* `releaseDate` Date
* `updateURL` String

アップデートをダウンロードしたときに出力されます。

Windowsでは、`releaseName` のみ提供されます。

## メソッド

`autoUpdater` オブジェクトは次のメソッドを持っています。

### `autoUpdater.setFeedURL(url)`

* `url` String

 `url`を設定し、自動アップデートを初期化します。 `url`は一度設定すると変更できません。

### `autoUpdater.checkForUpdates()`

アップデートがあるかどうかサーバーに問い合わせます。APIを使う前に、`setFeedURL`をコールしなければなりません。

### `autoUpdater.quitAndInstall()`

ダウンロード後、アプリを再起動して、アップデートをインストールします。`update-downloaded`が出力された後のみ、コールすべきです。

[squirrel-mac]: https://github.com/Squirrel/Squirrel.Mac
[server-support]: https://github.com/Squirrel/Squirrel.Mac#server-support
[squirrel-windows]: https://github.com/Squirrel/Squirrel.Windows
[installer]: https://github.com/atom/grunt-electron-installer
[app-user-model-id]: https://msdn.microsoft.com/en-us/library/windows/desktop/dd378459(v=vs.85).aspx
