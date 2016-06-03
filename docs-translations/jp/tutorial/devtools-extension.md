# DevTools エクステンション

簡単にデバッグをするために、Electronは[Chrome DevTools Extension][devtools-extension]に基本的な対応しています。

DevToolsエクステンションのために、ソースコードをダウンローし、`BrowserWindow.addDevToolsExtension` APIを使って読み込みます。読み込んだエクステンションは維持されているので、ウィンドウを作成するとき、毎回APIをコールする必要はありません。

** NOTE: React DevTools は動作しません。issue  https://github.com/electron/electron/issues/915 をご覧ください **

例えば、[React DevTools Extension](https://github.com/facebook/react-devtools)を使うために、最初にソースコードをダウンロードする必要があります。

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

エクステンションをビルドするために、[`react-devtools/shells/chrome/Readme.md`](https://github.com/facebook/react-devtools/blob/master/shells/chrome/Readme.md)を参照してください。

複数のウィンドウで、DevToolsを開くために、Electronでエクステンションをロードし、DevToolsコンソールで次のコードを実行します。

```javascript
const BrowserWindow = require('electron').remote.BrowserWindow;
BrowserWindow.addDevToolsExtension('/some-directory/react-devtools/shells/chrome');
```

エクステンションをアンロードするために、`BrowserWindow.removeDevToolsExtension` APIを名前を指定してコールすると、次回DevToolsを開いた時にはロードされません。

```javascript
BrowserWindow.removeDevToolsExtension('React Developer Tools');
```

## DevTools エクステンションのフォーマット

理想的には、Chromeブラウザー用に書かれたすべてのDevToolsエクステンションをElectronがロードできることですが、それらはプレーンディレクトリにある必要があります。`crx` エクステンションパッケージは、ディレクトリに解凍できる方法がなければ、Electronはロードする方法はありません。

## バックグラウンドページ

今のところ、ElectronはChromeエクステンションのバックグラウンドページ機能に対応していません。そのため、Electronでは、この機能に依存するDevToolsエクステンションは動作しません。

## `chrome.*` APIs

いくつかのChromeエクステンションは、`chrome.*` APIsを使用しており、ElectronでそれらのAPIを実装するための努力をしていますが、すべては実装されていません。

すべては実装されていない`chrome.*` APIについて考えると、もしDevToolsエクステンションが `chrome.devtools.*` 以外のAPIを使用していると、エクステンションは動作しない可能性が非常に高いです。うまく動作しないエクステンションがあれば、APIのサポートの追加のために、issue trackerへご報告ください。

[devtools-extension]: https://developer.chrome.com/extensions/devtools
