# ビルド方法 (macOS)

macOS で Electron をビルドする際は以下のガイドラインに従って下さい

## 前提条件

* macOS >= 10.8
* [Xcode](https://developer.apple.com/technologies/tools/) >= 5.1
* [node.js](http://nodejs.org) (external)

Homebrew からダウンロードした Python を使用している場合、下記 python モジュールもインストールしてください

* pyobjc

## ソースコードの取得

```bash
$ git clone https://github.com/electron/electron.git
```

## bootstrap

bootstrap スクリプトは、ビルドに必要な全ての依存関係の解決と、プロジェクトファイルを作成してくれます。Electron のビルドには [ninja](https://ninja-build.org/) を使用しているので、 Xcode project が生成されないことに注意してください。

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

## ビルド

`Release` 向けと `Debug` 向けを両方共ビルドするには下記を実行してください。

```bash
$ ./script/build.py
```

`Debug` 向けのみビルドしたい時は下記を実行してください。

```bash
$ ./script/build.py -c D
```

ビルドが終われば、`out/D` ディレクトリ配下に `Electron.app`が生成されます。

## 32bit サポート

Electron は 64bit の macOS 向けのみビルドできます。32bit macOS への対応の予定はありません。

## テスト

コーディング規約を満たしているかどうかは下記でテストできます。

```bash
$ ./script/cpplint.py
```

機能のテストは下記でテストできます。

```bash
$ ./script/test.py
```
