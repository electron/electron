# 継続的インテグレーションシステムによるテスト(Travis CI, Jenkins)

Electron は Chromium を元にしているので、実行にはディスプレイドライバーが必要です。Chromium がディスプレイドライバーを見つけられないと、Electron は起動に失敗してテストの実行ができません。Travis や Circle、 Jenkins などの継続的インテグレーションシステムで Electron アプリをテストするにはちょっとした設定が必要です。端的に言うと仮想ディスプレイドライバーを使用します。

## 仮想ディスプレイサーバーの設定

まず [Xvfb](https://en.wikipedia.org/wiki/Xvfb) をインストールします(リンク先は英語)。Xvfb は X Window System のプロトコルを実装した仮想フレームバッファです。描画系の操作をスクリーン表示無しにメモリ内で行ってくれます。

その後 xvfb の仮想スクリーンを作成して、環境変数 `$DISPLAY` に作成した仮想スクリーンを指定します。Electron の Chromium は自動で`$DISPLAY`を見るので、アプリ側の設定は必要ありません。この手順は Paul Betts 氏が公開しているツール [xvfb-maybe](https://github.com/paulcbetts/xvfb-maybe) によって自動化されています。テスト実行コマンドの前に `xvfb-maybe` を追加するだけで、現在のシステムが必要とする xvfb の設定を自動で行ってくれます。xvfb の設定が必要ない Windows や macOS では何もしません。

```
## Windows や macOS では以下のコマンドは electron-mocha をただ起動するだけです。
## Linux でかつ GUI 環境でない場合、以下のコマンドは
## xvfb-run electron-mocha ./test/*.js と同等になります。
xvfb-maybe electron-mocha ./test/*.js
```

### Travis CI

Travis では `.travis.yml` を以下のようにするといいでしょう。

```yml
addons:
  apt:
    packages:
      - xvfb

install:
  - export DISPLAY=':99.0'
  - Xvfb :99 -screen 0 1024x768x24 > /dev/null 2>&1 &
```

### Jenkins

Jenkins では [Xvfb plugin](https://wiki.jenkins-ci.org/display/JENKINS/Xvfb+Plugin) という便利なプラグインが利用可能です(リンク先は英語)。

### Circle CI

Circle CI では、なんと既に xvfb 及び `$DISPLAY` が設定されているので、[何の設定も必要ありません](https://circleci.com/docs/environment#browsers)(リンク先は英語)。

### AppVeyor

AppVeyor は Selenium や Chromium、Electron などをサポートしている Windows 上で動くので、特に設定は必要ありません。
