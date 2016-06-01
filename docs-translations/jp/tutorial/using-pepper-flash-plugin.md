# Pepper Flash プラグインを使用する

Electronは、Pepper Flashプラグインをサポートしています。ElectronでPepper Flashプラグインを使用するために、マニュアルでPepper Flashプラグインのパスを指定し、アプリケーションで有効化しなければなりません。


## Flash プラグインのコピー準備

OS XとLinuxでは、Pepper Flashプラグインの詳細は、Chromeブラウザーで、`chrome://plugins` にアクセスして確認できます。そこで表示されるパスとバージョンは、ElectronのPepper Flashサポートに役立ちます。それを別のパスにコピーすることができます。

## Electron スイッチの追加

直接、Electronコマンドラインに`--ppapi-flash-path` と `ppapi-flash-version` を追加するか、 アプリのreadyイベントの前に、`app.commandLine.appendSwitch`メソッドを使用します。`browser-window`の`plugins`スイッチを追加します。

例:

```javascript
// Specify flash path.
// On Windows, it might be /path/to/pepflashplayer.dll
// On OS X, /path/to/PepperFlashPlayer.plugin
// On Linux, /path/to/libpepflashplayer.so
app.commandLine.appendSwitch('ppapi-flash-path', '/path/to/libpepflashplayer.so');

// Specify flash version, for example, v17.0.0.169
app.commandLine.appendSwitch('ppapi-flash-version', '17.0.0.169');

app.on('ready', function() {
  mainWindow = new BrowserWindow({
    'width': 800,
    'height': 600,
    'web-preferences': {
      'plugins': true
    }
  });
  mainWindow.loadURL('file://' + __dirname + '/index.html');
  // Something else
});
```

##  `<webview>` タグでFlashプラグインを有効化

`<webview>` タグに`plugins`属性を追加する

```html
<webview src="http://www.adobe.com/software/flash/about/" plugins></webview>
```

## トラブルシューティング

Pepper Flashプラグインが読み込まれているかは、devtoolsのconsoleで`navigator.plugins`を見ることで確認することができます。(プラグインのパスが正しいかがわからなくても)

Pepper FlashプラグインのアーキテクチャはElectronのアーキテクチャと一致している必要があります。Windowsで、64bit Electronに32bit Flashプラグインを使用するのはよくある間違いです。
