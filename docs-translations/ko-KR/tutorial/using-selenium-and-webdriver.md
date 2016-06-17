# Selenium 과 WebDriver 사용하기

[ChromeDriver - WebDriver for Chrome][chrome-driver]로부터 인용:

> WebDriver는 많은 브라우저에서 웹 앱을 자동적으로 테스트하는 툴입니다.
> 이 툴킷은 웹 페이지를 자동으로 탐색하고 유저 폼을 사용하거나 자바스크립트를 실행하는
> 등의 작업을 수행할 수 있습니다. ChromeDriver는 Chromium의 WebDriver wire 프로토콜
> 스텐드얼론 서버 구현입니다. Chromium 과 WebDriver 팀 멤버에 의해 개발되었습니다.

## Spectron 설정하기

[Spectron][spectron]은 공식적으로 지원하는 Electron을 위한 ChromeDriver 테스팅
프레임워크입니다. 이는 [WebdriverIO](http://webdriver.io/)를 기반으로 만들어졌고,
테스트에서 Electron API에 접근하기 위한 헬퍼를 가지고 있으며 ChromeDriver를 포함하고
있습니다.

```bash
$ npm install --save-dev spectron
```

```js
// 윈도우가 제목과 함께 보이는지 검증하는 간단한 테스트.
var Application = require('spectron').Application
var assert = require('assert')

var app = new Application({
  path: '/Applications/MyApp.app/Contents/MacOS/MyApp'
})

app.start().then(function () {
  // 윈도우가 보이는지 확인합니다.
  return app.browserWindow.isVisible()
}).then(function (isVisible) {
  // 윈도우가 보이는지 검증합니다.
  assert.equal(isVisible, true)
}).then(function () {
  // 윈도우의 제목을 가져옵니다.
  return app.client.getTitle()
}).then(function (title) {
  // 윈도우의 제목을 검증합니다.
  assert.equal(title, 'My App')
}).then(function () {
  // 애플리케이션을 중지합니다.
  return app.stop()
}).catch(function (error) {
  // 테스트의 실패가 있다면 로깅합니다.
  console.error('Test failed', error.message)
})
```

## WebDriverJs 설정하기

[WebDriverJs](https://code.google.com/p/selenium/wiki/WebDriverJs)는 WebDriver를
사용하여 테스트 할 수 있도록 도와주는 node 패키지입니다. 다음 예시를 참고하세요.

### 1. 크롬 드라이버 시작

먼저, `chromedriver` 바이너리를 다운로드 받고 실행합니다:

```bash
$ npm install electron-chromedriver
$ ./node_modules/.bin/chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

포트 `9515`는 나중에 사용하므로 기억해 놓습니다.

### 2. WebDriverJS 설치

```bash
$ npm install selenium-webdriver
```

### 3. 크롬 드라이버에 연결

`selenium-webdriver`를 Electron과 같이 사용하는 방법은 기본적으로 upstream과
같습니다. 한가지 다른점이 있다면 수동으로 크롬 드라이버 연결에 대해 설정하고 Electron
실행파일의 위치를 전달합니다:

```javascript
const webdriver = require('selenium-webdriver');

const driver = new webdriver.Builder()
  // 작동하고 있는 크롬 드라이버의 포트 "9515"를 사용합니다.
  .usingServer('http://localhost:9515')
  .withCapabilities({
    chromeOptions: {
      // 여기에 사용중인 Electron 바이너리의 경로를 지정하세요.
      binary: '/Path-to-Your-App.app/Contents/MacOS/Electron',
    }
  })
  .forBrowser('electron')
  .build();

driver.get('http://www.google.com');
driver.findElement(webdriver.By.name('q')).sendKeys('webdriver');
driver.findElement(webdriver.By.name('btnG')).click();
driver.wait(() => {
 return driver.getTitle().then((title) => {
   return title === 'webdriver - Google Search';
 });
}, 1000);

driver.quit();
```

## WebdriverIO 설정하기

[WebdriverIO](http://webdriver.io/)는 웹 드라이버와 함께 테스트를 위해 제공되는
node 패키지입니다.

### 1. 크롬 드라이버 시작

먼저, `chromedriver` 바이너리를 다운로드 받고 실행합니다:

```bash
$ npm install electron-chromedriver
$ ./node_modules/.bin/chromedriver --url-base=wd/hub --port=9515
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

포트 `9515`는 나중에 사용하므로 기억해 놓읍시다

### 2. WebDriverIO 설치

```bash
$ npm install webdriverio
```

### 3. 크롬 드라이버에 연결
```javascript
const webdriverio = require('webdriverio');
let options = {
  host: 'localhost', // localhost에서 작동중인 크롬 드라이버 서버를 사용합니다.
  port: 9515,        // 연결할 크롬 드라이버 서버의 포트를 설정합니다.
  desiredCapabilities: {
    browserName: 'chrome',
    chromeOptions: {
      binary: '/Path-to-Your-App/electron', // Electron 바이너리 경로
      args: [/* cli arguments */]           // 선택 사항, 'app=' + /path/to/your/app/
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

## 작업 환경

따로 Electron을 다시 빌드하지 않는 경우 간단히 애플리케이션을 Electron의 리소스
디렉터리에 [배치](application-distribution.md)하여 바로 테스트 할 수 있습니다.

또한, Electron 바이너리의 명령줄 인수에 애플리케이션 폴더를 지정하는 방법으로 실행할
수도 있습니다. 이 방법을 사용하면 애플리케이션 폴더를 Electron의 `resource`
디렉터리로 복사하는 불필요한 과정을 생략할 수 있습니다.

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/
[spectron]: http://electron.atom.io/spectron
