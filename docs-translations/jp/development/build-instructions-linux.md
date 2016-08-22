# ビルド方法 (Linux)

Linux で Electron をビルドする際は以下のガイドラインに従って下さい

## 事前準備

* 25GB のディスク空き容量と8GB の RAM が少なくとも必要です
* Python 2.7.xが必要です。CentOS のようないくつかのディストリビューションはまだ Python 2.6.x を使ってるので、`python -V` で Python のバージョンを確認する必要があるでしょう。
* Node.js v0.12.x が必要です。Node のインストールには色んな方法があります。[Node.js](http://nodejs.org) からソースコードをダウンロードしてビルドすることもできます。そうすることで root でないユーザーのホームディレクトリに Node をインストールすることもできます。あるいは [NodeSource](https://nodesource.com/blog/nodejs-v012-iojs-and-the-nodesource-linux-repositories) のようなレポジトリを試してみてください。
* Clang 3.4 以上が必要です。
* GTK+ と libnotify の開発用ヘッダーが必要です。

Ubuntu では以下のライブラリをインストールしてください。

```bash
$ sudo apt-get install build-essential clang libdbus-1-dev libgtk2.0-dev \
                       libnotify-dev libgnome-keyring-dev libgconf2-dev \
                       libasound2-dev libcap-dev libcups2-dev libxtst-dev \
                       libxss1 libnss3-dev gcc-multilib g++-multilib curl
```

Fedora では以下のライブラリをインストールしてください。

```bash
$ sudo yum install clang dbus-devel gtk2-devel libnotify-devel libgnome-keyring-devel \
                   xorg-x11-server-utils libcap-devel cups-devel libXtst-devel \
                   alsa-lib-devel libXrandr-devel GConf2-devel nss-devel
```

他のディストリビューションでは pacman のようなパッケージマネージャーを通して似たようなインストールパッケージを提供しているかもしれません。あるいはソースコードからコンパイルもできます。

## ソースコードの取得

```bash
$ git clone https://github.com/electron/electron.git
```

## Bootstrapp

bootstrap スクリプトは、ビルドに必要な全ての依存関係の解決と、プロジェクトファイルを作成してくれます。
スクリプトの実行には Python 2.7.x が必要です。これらのファイルをダウンロードするには時間が結構かかります。Electron のビルドには [ninja](https://ninja-build.org/) を使用しているので、 `Makefile` が生成されないことに注意してください。

```bash
$ cd electron
$ ./script/bootstrap.py -v
```

### クロスコンパイル

`arm` アーキテクチャ向けにビルドしたい場合、下記の依存ライブラリをインストールしてください。

```bash
$ sudo apt-get install libc6-dev-armhf-cross linux-libc-dev-armhf-cross \
                       g++-arm-linux-gnueabihf
```

`arm` や `ia32` 向けにクロスコンパイルするには、`bootstrap.py` に `--target_arch` パラメータを渡してください。

```bash
$ ./script/bootstrap.py -v --target_arch=arm
```

## ビルド

`Release` 向けと `Debug` 向けにビルドするには下記を実行してください。

```bash
$ ./script/build.py
```

このスクリプトはとても大きな Electron の実行ファイルを `out/R` に生成します。ファイルサイズは1.3GB を超えます、なぜかというと、`Release` 向けのバイナリがデバッグシンボルを含むためです。ファイルサイズを減らすには下記のように `create-dist.py` を実行してください。

```bash
$ ./script/create-dist.py
```

このスクリプトは非常に小さい小さいファイルサイズのディストリビューションを `dist` ディレクトリに生成します。`create-dist.py` 実行後、まだ `out/R` に 1.3 GB を超えたバイナリがあるので、削除したくなることでしょう。

`Debug` 向けのみビルドしたい時は下記を実行してください。

```bash
$ ./script/build.py -c D
```

ビルドが終われば、`out/D` ディレクトリ配下に `Electron.app`が生成されます。

## ビルドファイルのクリーン

ビルドファイルを削除するには下記を実行してください。

```bash
$ ./script/clean.py
```

## トラブルシューティング

### Error While Loading Shared Libraries: libtinfo.so.5

あらかじめビルドされた `clang` は `libtinfo.so.5` をリンクしようとします。
ホストのシステムによっては 適切な `libncurses` への symlink を作成したほうがよいです。

```bash
$ sudo ln -s /usr/lib/libncurses.so.5 /usr/lib/libtinfo.so.5
```

## Tests

コーディング規約どおりになっているかテストするには下記を実行してください。

```bash
$ npm run lint
```

機能のテストは下記を実行してください。

```bash
$ ./script/test.py
```

## 高度なトピック

デフォルトのビルド設定は、メジャーなデスクトップLinux 向けになっています。
ある特定のディストリビューションやデバイス向けにビルドする際は、下記の情報が役に立つでしょう。

### `libchromiumcontent` をローカルでビルドする

あらかじめビルドされた `libchromiumcontent` のバイナリの使用を回避するには、下記のように`--build_libchromiumcontent` オプションを `bootstrap.py` に渡してください。

```bash
$ ./script/bootstrap.py -v --build_libchromiumcontent
```
`shared_library` の設定はデフォルトではビルドされないことに注意してください。以下のモードを使うことで、Electron の `Release` バージョンのみビルドすることができます。

```bash
$ ./script/build.py -c R
```

### ダウンロードした `clang` ではなくシステムの `clang` を使う

デフォルトではElectron は Chromium プロジェクトによって提供されるあらかじめビルドされた `clang` を使ってビルドします。もしなんらかの理由でシステムにインストールされた `clang` でビルドしたい場合、 `--clang_dir=<path>` オプションをつけて、`bootstrap.py` を実行してください。オプションを渡すことで、ビルドスクリプトは `clang` が `<path>/bin/` にあるとして実行されます。

例えば、`/user/local/bin/clang` 配下に `clang` をインストールした場合、以下のようになります。

```bash
$ ./script/bootstrap.py -v --build_libchromiumcontent --clang_dir /usr/local
$ ./script/build.py -c R
```

### `clang` 以外のコンパイラを使う

Electron のビルドに `g++` のようなコンパイラを使うためには、まず `--disable_clang` オプションで、`clang` を無効化し、そして `CC` と `CXX` 環境変数を望むように設定してください。

例えば GCC toolchain でビルドするには以下のようになります。

```bash
$ env CC=gcc CXX=g++ ./script/bootstrap.py -v --build_libchromiumcontent --disable_clang
$ ./script/build.py -c R
```

### 環境変数

また `CC` と `CXX` とは別に、下記の環境変数を設定することでもビルド設定をイジることができます。

* `CPPFLAGS`
* `CPPFLAGS_host`
* `CFLAGS`
* `CFLAGS_host`
* `CXXFLAGS`
* `CXXFLAGS_host`
* `AR`
* `AR_host`
* `CC`
* `CC_host`
* `CXX`
* `CXX_host`
* `LDFLAGS`

これら環境変数は `bootstrap.py` 実行時に設定されてないといけません。 `build.py` 実行時に設定しても動きません。
