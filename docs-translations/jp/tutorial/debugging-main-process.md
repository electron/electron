# メインプロセスのデバッグ

ブラウザウィンドウ DevToolsのみ、レンダリングプロセススクリプトをデバッグすることができます。メインプロセスからスクリプトをデバッグする方法を提供するために、Electronは、`--debug` と `--debug-brk` スイッチを提供します。

## コマンドライン スイッチ

Electronのメインプロセスをデバッグするために次のコマンドラインスイッチを使用します

### `--debug=[port]`

このスイッチを使ったとき、Electronは、 `port` 上でV8デバッガープロトコルメッセージをリッスンします。デフォルトの `port` は `5858`です。

### `--debug-brk=[port]`

`--debug` のようですが、最初の行でスクリプトをポーズします。

## デバッグ用のnode-inspectorを使用する

__Note:__ Electron は今のところ、明確にはnode-inspectorは動作せず、node-inspectorのコンソール下で、`process` オブジェクトを調査した場合、メインプロセスはクラッシュするでしょう。

### 1. インストールされた[node-gyp required tools][node-gyp-required-tools] を確認する

### 2. [node-inspector][node-inspector]をインストールする

```bash
$ npm install node-inspector
```

### 3. `node-pre-gyp`のパッチバージョンをインストールする

```bash
$ npm install git+https://git@github.com/enlight/node-pre-gyp.git#detect-electron-runtime-in-find
```

### 4. Electron用の `node-inspector` `v8` モジュールをリコンパイルする(対象のElectronのバージョン番号を変更する)

```bash
$ node_modules/.bin/node-pre-gyp --target=0.36.2 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/atom-shell reinstall
$ node_modules/.bin/node-pre-gyp --target=0.36.2 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/atom-shell reinstall
```

[How to install native modules](how-to-install-native-modules)を見る。

### 5. Electron用のデバッグモードを有効にする

デバッグフラッグでElectronを開始する:

```bash
$ electron --debug=5858 your/app
```

または、最初のライン上でスクリプトをポーズする:

```bash
$ electron --debug-brk=5858 your/app
```

### 6. Electronを使用して、[node-inspector][node-inspector] サーバーを開始する

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. デバッグUIを読み込みます

Chromeブラウザで、 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 を開きます。エントリーラインを見るために、debug-brkを始めるには、ポーズをクリックします。

[node-inspector]: https://github.com/node-inspector/node-inspector
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#how-to-install-native-modules
