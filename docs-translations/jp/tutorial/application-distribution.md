# アプリケーションの配布

Electronでアプリケーションを配布するために、アプリケーションを`app` という名前のディレクトリにし、Electronのリソースディレクトリ(OS X では
`Electron.app/Contents/Resources/` 、Linux と Windows では `resources/`)配下に置くべきです,

OS X:

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

`Electron.app` (または Linux上では、`electron`、Windows上では、 `electron.exe`)を実行すると、Electronはアプリケーションを開始します。The `electron` ディレクトリを最終的なユーザーに提供するために配布します。

## ファイルにアプリケーションをパッケージングする

すべてのソースコードをコピーすることでアプリケーションを提供する方法とは別に、アプリケーションのソースコードをユーザーに見えるのを避けるために、[asar](https://github.com/atom/asar) にアーカイブしてアプリケーションをパッケージ化することができます。

`asar`アーカイブを使用するために、`app`フォルダーと置き換え、アーカイブファイルを`app.asar`という名前に変更する必要があり、Electronのリソースディレクトリの下に置くと、Electronはアーカイブを読み込もうとし、それを開始します。

OS X:

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

## ダウンローするバイナリのブランド名の変更

Electronにバンドルした後、ユーザーに配布する前に、 Electron名を変更したいでしょう。

### Windows

`electron.exe`を任意の名前に変更でき、[rcedit](https://github.com/atom/rcedit) または
[ResEdit](http://www.resedit.net)のようなツールでアイコンやその他の情報を編集できます。

### OS X

 `Electron.app` を任意の名前に変更でき、次のファイルの `CFBundleDisplayName`と `CFBundleIdentifier`、 `CFBundleName`のフィールドの名前を変更する必要があります。

* `Electron.app/Contents/Info.plist`
* `Electron.app/Contents/Frameworks/Electron Helper.app/Contents/Info.plist`

Activity Monitorで、`Electron Helper`と表示されるのを避けるために、helper appの名前を変更でき、 helper appの実行ファイルの名前を確認します。

次のようなappの変更する名前の構造

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

マニュアルでは、Electronのコードをダウンロードし、リビルドさせます。一方で、面倒なタスクはこれで自動化できます:
[grunt-build-atom-shell](https://github.com/paulcbetts/grunt-build-atom-shell).

This task will automatically handle editing the 自動処理を管理するのに、`.gyp`ファイルを編集し、ソースからビルドし、新しい実行名に一致したときにはNativeのNodeモジュールをリビルドするようにできます。

## パッケージングツール

マニュアルでパッケージ化するのとは別に、サードパーティのパッケジングツールを選ぶこともできます。

* [electron-packager](https://github.com/maxogden/electron-packager)
* [electron-builder](https://github.com/loopline-systems/electron-builder)
