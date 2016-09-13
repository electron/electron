# Widevine CDM Pluginを使用する

Electronで、Chromeブラウザーに同梱される Widevine CDMプラグインを使用できます。

## プラグインを取得する

Electronは、ライセンス的な理由でWidevine CDMプラグインは同梱されません。Widevine CDMプラグインを取得するために、最初に、使用するElectronビルドのChromeバージョンとアーキテクチャを合わせた公式のChromeブラウザーをインストールする必要があります。

__Note:__ Chromeブラウザのメジャーバージョンは、Electronが使用するChromeバージョンと同じでなければなりません。そうでなければ、プラグインは、`navigator.plugins`経由でロードされて表示されるにも関わらず動作しません。

### Windows & macOS

Chromeブラウザーで、`chrome://components/`を開き、 `WidevineCdm` を探し、それが最新であることを確認し、`APP_DATA/Google/Chrome/WidevineCDM/VERSION/_platform_specific/PLATFORM_ARCH/`ディレクトリからすべてのプラグインバイナリを探します。

`APP_DATA` は、アプリデータを格納するシステムロケーションです。Windowsでは`%LOCALAPPDATA%`、macOSでは`~/Library/Application Support`です。`VERSION` は、Widevine CDM プラグインのバージョン文字列で、 `1.4.8.866`のような文字列が格納されます。`PLATFORM` は、 `mac` か `win`です。`ARCH` は `x86` か `x64`です。

Windowsでは、`widevinecdm.dll` と `widevinecdmadapter.dll`が必要で、macOSでは、`libwidevinecdm.dylib` と `widevinecdmadapter.plugin`です。任意の場所にコピーできますが、一緒に配置する必要があります。

### Linux

Linux上では、プラグインバイナリは、Chromeブラウザーにいっほに格納され、 `/opt/google/chrome` 配下にあり、ファイル名は `libwidevinecdm.so` と `libwidevinecdmadapter.so`です。

## プラグインを使用する

プラグインファイルを取得した後、Electronoの`--widevine-cdm-path`コマンドラインスイッチで`widevinecdmadapter`のパスを指定し、`--widevine-cdm-version`スイッチでプラグインのバージョンを指定します。

__Note:__ `widevinecdmadapter` バイナリはElectronにパスが通っていても`widevinecdm` バイナリはそこに置く必要があります。

コマンドラインスイッチは、`app`モジュールの`ready`イベントが発火する前に通り、プラグインが使用するページは、プラグインを有効にしなければなりません。

サンプルコード:

```javascript
// You have to pass the filename of `widevinecdmadapter` here, it is
// * `widevinecdmadapter.plugin` on macOS,
// * `libwidevinecdmadapter.so` on Linux,
// * `widevinecdmadapter.dll` on Windows.
app.commandLine.appendSwitch('widevine-cdm-path', '/path/to/widevinecdmadapter.plugin');
// The version of plugin can be got from `chrome://plugins` page in Chrome.
app.commandLine.appendSwitch('widevine-cdm-version', '1.4.8.866');

let win = null;
app.on('ready', () => {
  win = new BrowserWindow({
    webPreferences: {
      // The `plugins` have to be enabled.
      plugins: true
    }
  });
});
```

## プラグインの検証

プラグインが動作するかどうかを検証するために、次の方法を使用できます。

* devtoolsを開き、Widevine CDMプラグインに`navigator.plugins`が含まれているかどうかを確認します。
* https://shaka-player-demo.appspot.com/ を開き、`Widevine`で使用するマニフェストをロードします。
* http://www.dash-player.com/demo/drm-test-area/を開き、`bitdash uses Widevine in your browser`をチェックし、ビデオを再生します。
