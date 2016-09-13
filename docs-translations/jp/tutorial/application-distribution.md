# アプリケーションの配布

Electronでアプリケーションを配布するために、アプリケーションを`app` という名前のディレクトリにし、Electronのリソースディレクトリ(macOS では
`Electron.app/Contents/Resources/` 、Linux と Windows では `resources/`)配下に置くべきです,

macOS:

```text
electron/Electron.app/Contents/Resources/app/
├── package.json
├── main.js
└── index.html
```

Windows と Linux:

```text
electron/resources/app
├── package.json
├── main.js
└── index.html
```

`Electron.app` (または Linux上では、`electron`、Windows上では、 `electron.exe`)を実行すると、Electronはアプリケーションを開始します。`electron` ディレクトリを最終的なユーザーに提供するために配布します。

## ファイルにアプリケーションをパッケージングする

すべてのソースコードをコピーすることでアプリケーションを提供する方法とは別に、アプリケーションのソースコードをユーザーに見えるのを避けるために、[asar](https://github.com/electron/asar) にアーカイブしてアプリケーションをパッケージ化することができます。

`app` フォルダの代わりに `asar` アーカイブを使用するためには、アーカイブファイルを `app.asar` という名前に変更し、Electron のリソースディレクトリに以下のように配置する必要があります。すると、Electron はアーカイブを読み込もうとし、それを開始します。

macOS:

```text
electron/Electron.app/Contents/Resources/
└── app.asar
```

Windows と Linux:

```text
electron/resources/
└── app.asar
```

[Application packaging](application-packaging.md)で、詳細を確認できます。

## ダウンロードするバイナリのブランド名の変更

Electronにバンドルした後、ユーザーに配布する前に、 Electron名を変更したいでしょう。

### Windows

`electron.exe`を任意の名前に変更でき、[rcedit](https://github.com/atom/rcedit)
のようなツールでアイコンやその他の情報を編集できます。

### macOS

`Electron.app` を任意の名前に変更でき、次のファイルの `CFBundleDisplayName`と `CFBundleIdentifier`、 `CFBundleName`のフィールドの名前を変更する必要があります。

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

ヘルパーアプリケーションの名前を変更して、アクティビティモニタに `Electron Helper` と表示されないようにすることもできますが、その際はヘルパーアプリケーションの実行可能ファイルの名前を変更したことを確認してください。

名前変更後のアプリケーションの構成は以下の通りです：

```
MyApp.app/Contents
├── Info.plist
├── MacOS/
│   └── MyApp
└── Frameworks/
    ├── MyApp Helper EH.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper EH
    ├── MyApp Helper NP.app
    |   ├── Info.plist
    |   └── MacOS/
    |       └── MyApp Helper NP
    └── MyApp Helper.app
        ├── Info.plist
        └── MacOS/
            └── MyApp Helper
```

### Linux

`electron` を任意の名前に変更できます。

## ソースからElectronをリビルドしてブランド名を変更する

プロダクト名を変更し、ソースからビルドすることで、Electronのブランド名を変更できます。これをするために、`atom.gyp` ファイルを変更し、クリーンリビルドする必要があります。

### grunt-build-atom-shell

Electron のコードを手動チェックアウトして再構築するのには複雑な手順が必要ですが、これを自動的に扱うための Grunt タスクが作られています：
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

このタスクは自動的に `.gyp` ファイルの編集、ソースからのビルド、そして新しい実行可能ファイル名に一致するようにネイティブの Node モジュールを再構築します。

## パッケージングツール

手動でアプリケーションをパッケージ化する以外の方法として、サードパーティのパッケジングツールを選ぶこともできます。

* [electron-packager](https://github.com/maxogden/electron-packager)
* [electron-builder](https://github.com/loopline-systems/electron-builder)
