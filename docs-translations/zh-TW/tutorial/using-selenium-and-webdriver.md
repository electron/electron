# 使用 Selenium 和 WebDriver

根據 [ChromeDriver - WebDriver for Chrome][chrome-driver]：

> WebDriver is an open source tool for automated testing of web apps across many
> browsers. It provides capabilities for navigating to web pages, user input,
> JavaScript execution, and more. ChromeDriver is a standalone server which
> implements WebDriver's wire protocol for Chromium. It is being developed by
> members of the Chromium and WebDriver teams.

為了與 Electron 一起使用 `chromedriver`，你需要告訴 `chromedriver` 去哪找 Electron 並讓他知道 Electron 是 Chrome 瀏覽器。

## 透過 WebDriverJs 設定

[WebDriverJs](https://code.google.com/p/selenium/wiki/WebDriverJs) 提供一個 Node 套件來透過 web driver 做測試，我們將使用它作為例子。

### 1. 啟動 ChromeDriver

首先你需要下載 `chromedriver` 執行檔，然後執行它：

```bash
$ ./chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

記住埠號(port number) `9515`，等等會使用到它

### 2. 安裝 WebDriverJS 

```bash
$ npm install selenium-webdriver
```

### 3. 連接到 ChromeDriver

與 Electron 一起使用 `selenium-webdriver` 的方法基本上與 upstream 相同，除了你需要手動指定如何連接 chrome driver 和去哪找 Electron 的執行檔：

```javascript
const webdriver = require('selenium-webdriver');

var driver = new webdriver.Builder()
  // The "9515" is the port opened by chrome driver.
  .usingServer('http://localhost:9515')
  .withCapabilities({
    chromeOptions: {
      // Here is the path to your Electron binary.
      binary: '/Path-to-Your-App.app/Contents/MacOS/Atom',
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

## 透過 WebdriverIO 設定

[WebdriverIO](http://webdriver.io/) 提供一個 Node 套件來透過 web driver 做測試。

### 1. 啟動 ChromeDriver

首先你需要下載 `chromedriver` 執行檔，然後執行它：

```bash
$ chromedriver --url-base=wd/hub --port=9515
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

記住埠號(port number)  `9515`，等等會用到它

### 2. 安裝 WebdriverIO

```bash
$ npm install webdriverio
```

### 3. 連接到 chrome driver

```javascript
const webdriverio = require('webdriverio');
var options = {
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

var client = webdriverio.remote(options);

client
    .init()
    .url('http://google.com')
    .setValue('#q', 'webdriverio')
    .click('#btnG')
    .getTitle().then(function(title) {
        console.log('Title was: ' + title);
    })
    .end();
```

## 運作流程

要在不重新建置 Electron 的情況下測試你的應用程式，只需要 [放置](https://github.com/electron/electron/blob/master/docs/tutorial/application-distribution.md) 你的應用程式到 Electron 的資源目錄中即可。

或者，傳遞一個指向你應用程式資料夾的參數來透過你的 Electron 執行檔運行，這會減少複製你應用程式到 Electron 資源資料夾的需求。

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/
