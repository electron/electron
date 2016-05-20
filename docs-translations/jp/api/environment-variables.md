# 環境変数

> アプリケーションの設定や動作を、コードの変更なしで制御します。

コマンドラインやアプリのコードよりも早く初期化されるために、Electronのいくつかの挙動は環境変数がコントロールしています。

POSIX shellでの例:

```bash
$ export ELECTRON_ENABLE_LOGGING=true
$ electron
```

Windows コンソール上:

```powershell
> set ELECTRON_ENABLE_LOGGING=true
> electron
```

## `ELECTRON_RUN_AS_NODE`

通常のNode.jsプロセスでプロセスを開始します。

## `ELECTRON_ENABLE_LOGGING`

Chromeのインターナルログをコンソールに出力します。


## `ELECTRON_LOG_ASAR_READS`

ASARファイルからElectronが読み込んだとき、システム`tmpdir`へ読み込みオフセットとファイルのパスを記録します。ファイルの順序を最適化するために、得られたファイルはASARモジュールに提供されます。

## `ELECTRON_ENABLE_STACK_DUMPING`

Electronがクラッシュしたとき、コンソールにスタックとレースを出力します。
`crashReporter`が開始していないと、この環境変数は動作しません。

## `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

Electronがクラッシュしたとき、Windowsのクラッシュダイアログを表示します。
`crashReporter`が開始していないと、この環境変数は動作しません。

## `ELECTRON_NO_ATTACH_CONSOLE` _Windows_

現在のコンソールセッションにはアタッチできません。

## `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

Linuxでグローバルメニューバーを使用できません。

