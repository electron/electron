# ネイティブのNodeモジュールを使用する

Electronは、ネイティブのNodeモジュールをサポートしていますが、Node公式とは異なるV8バージョンを使用しているので、ネイティブモジュールでビルドする時、Electronのヘッダーで指定する必要があります。

## ネイティブNodeモジュールとの互換性

Nodeが新しいV8バージョンを使用し始めた時、Nativeモジュールは動作しなくなるかもしれません。ElectronでNativeモジュールが動作するかどうかを確認するために、Electronで使用する内部のNodeバージョンがサポートしているかを確認すべきです。ElectronでNodeが使用しているバージョンを確認するには、[releases](https://github.com/electron/electron/releases)ページを見るか、`process.version` (例えば [Quick Start](https://github.com/electron/electron/blob/master/docs/tutorial/quick-start.md)を見てください)を使用してください。

Nodeの複数バージョンを簡単にサポートできるので、あなたのモジュールに [NAN](https://github.com/nodejs/nan/) を使うことを検討してください。古いバージョンからElectronで動作するNodeの新しいバージョンへ移植するのに役立ちます。

## ネイティブモジュールのインストール方法

ネイティブモジュールをインストールするための3つの方法:

### 簡単な方法

ヘッダーをダウンロードし、ネイティブモジュールをビルドする手順をマニュアルで管理する、[`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild) パッケージ経由で、ネイティブモジュールをリビルドするのが最も簡単な方法です。

```sh
npm install --save-dev electron-rebuild

# Every time you run "npm install", run this
./node_modules/.bin/electron-rebuild

# On Windows if you have trouble, try:
.\node_modules\.bin\electron-rebuild.cmd
```

### npmを使う方法

モジュールをインストールするために`npm` を使用できます。環境変数の設定を除いて、Nodeモジュールと完全に同じ手順です。

```bash
export npm_config_disturl=https://atom.io/download/atom-shell
export npm_config_target=0.33.1
export npm_config_arch=x64
export npm_config_runtime=electron
HOME=~/.electron-gyp npm install module-name
```

### node-gypを使う方法

ElectronのヘッダーでNodeモジュールをビルドするために、どこからヘッダーをダウンロードするかとどのバージョンを使用するかを選択して`node-gyp`を実行する必要があります。

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/atom-shell
```

開発ヘッダーを探し、 `HOME=~/.electron-gyp` を変更します。`--target=0.29.1`がElectronのバージョンです。 `--dist-url=...` で、どこからヘッダーをダウンロードするかを指定します。`--arch=x64`を使用して、64bit システム用にモジュールをビルドします。
