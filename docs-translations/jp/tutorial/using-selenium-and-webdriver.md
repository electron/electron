# Selenium と WebDriverを使用する

[ChromeDriver - WebDriver for Chrome][chrome-driver]より:

> WebDriverは、複数のブラウザ間でWebアプリの自動テストをするためのオープンソースツールです。Webページの移動、ユーザー入力、JavaScriptの実行などを可能にします。ChromeDriverはChromium用のWebDriverのワイヤープロトコルで実装されたスタンドアロンサーバーです。ChromiumとWebDriverチームメンバーが開発しています。

Electronで `chromedriver` を使用するために、Electronがどこにあるかを示し、ElectronはChromeブラウザーであると思わせます。

## Spectronの設定

[Spectron][spectron]はElectronのための公式なChromeDriverテストフレームワークです。
[WebdriverIO](http://webdriver.io/)に基づいてビルドされており、テストの中でElectron APIを使用するヘルパーやChromeDriveを持っています。

```bash
$ npm install --save-dev spectron
```

```js
// 可視のウィンドウがタイトル付きで開かれているかを調べる簡単なテスト
var Application = require('spectron').Application
var assert = require('assert')

var app = new Application({
  path: '/Applications/MyApp.app/Contents/MacOS/MyApp'
})

app.start().then(function () {
  // ウィンドウは表示されている？
  return app.browserWindow.isVisible()
}).then(function (isVisible) {
  // ウィンドウが表示されているか確認
  assert.equal(isVisible, true)
}).then(function () {
  // ウィンドウのタイトルを取得
  return app.client.getTitle()
}).then(function (title) {
  // ウィンドウのタイトルを確認
  assert.equal(title, 'My App')
}).then(function () {
  // アプリケーションを止める
  return app.stop()
}).catch(function (error) {
  // 失敗した場合ログを残す
  console.error('Test failed', error.message)
})
```

## WebDriverJsを設定

[WebDriverJs](https://code.google.com/p/selenium/wiki/WebDriverJs) は、web driver でテストするためのNodeパッケージを提供します。例のように使用します。

### 1. ChromeDriverを開始

最初に、 `chromedriver` バイナリをダウンロードし、実行します:

```bash
$ ./chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

ポート番号 `9515`を覚えておいてください、あとで使用します。

### 2. WebDriverJSのインストール

```bash
$ npm install selenium-webdriver
```

### 3. ChromeDriverに接続する

Electronでの `selenium-webdriver` 使用方法は、chrome driverへの接続方法とElectronバイナリがどこにあるかをマニュアルで指定する以外は、upstreamと基本的に同じです。

```javascript
const webdriver = require('selenium-webdriver');

const driver = new webdriver.Builder()
  // The "9515" is the port opened by chrome driver.
  .usingServer('http://localhost:9515')
  .withCapabilities({
    chromeOptions: {
      // Here is the path to your Electron binary.
      binary: '/Path-to-Your-App.app/Contents/MacOS/Electron',
    }
  })
  .forBrowser('electron')
  .build();

driver.get('http://www.google.com');
driver.findElement(webdriver.By.name('q')).sendKeys('webdriver');
driver.findElement(webdriver.By.name('btnG')).click();
driver.wait(function() {
 return driver.getTitle().then(function(title) {
   return title === 'webdriver - Google Search';
 });
}, 1000);

driver.quit();
```

## WebdriverIOのセットアップをする

[WebdriverIO](http://webdriver.io/) は、web driverでテストするNodeパッケージを提供します。

### 1. ChromeDriverを開始する

最初に、 `chromedriver` バイナリをダウンロードし、実行します。:

```bash
$ chromedriver --url-base=wd/hub --port=9515
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

ポート番号 `9515`を覚えておいてください、あとで使用します。

### 2. WebdriverIOをインストールする

```bash
$ npm install webdriverio
```

### 3. chrome driverに接続する

```javascript
const webdriverio = require('webdriverio');
const options = {
    host: "localhost", // Use localhost as chrome driver server
    port: 9515,        // "9515" is the port opened by chrome driver.
    desiredCapabilities: {
        browserName: 'chrome',
        chromeOptions: {
          binary: '/Path-to-Your-App/electron', // Path to your Electron binary.
          args: [/* cli arguments */]           // Optional, perhaps 'app=' + /path/to/your/app/
        }
    }
};

let client = webdriverio.remote(options);

client
    .init()
    .url('http://google.com')
    .setValue('#q', 'webdriverio')
    .click('#btnG')
    .getTitle().then((title) => {
        console.log('Title was: ' + title);
    })
    .end();
```

## ワークフロー

Electronはリビルドせずにアプリケーションをテストするために、単純にElectronのリソースディレクトリでアプリのソースを[配置します](https://github.com/electron/electron/blob/master/docs/tutorial/application-distribution.md)。

もしくは、アプリのフォルダーを引数にしてElectronバイナリを実行します。これによって、Electronのリソースディレクトリにアプリをコピー＆ペーストする必要がなくなります。

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/
[spectron]: http://electron.atom.io/spectron
