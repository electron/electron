# Selenium 과 WebDriver 사용하기

[ChromeDriver - WebDriver for Chrome][chrome-driver]로 부터 인용:

> WebDriver는 많은 브라우저에서 웹 앱을 자동적으로 테스트하는 툴입니다.
> 이 툴은 웹 페이지를 자동으로 탐색하고, 유저 폼을 사용하거나, 자바스크립트를 실행하는 등의 작업을 수행할 수 있습니다.
> ChromeDriver는 Chromium의 WebDriver wire 프로토콜 스텐드얼론 서버 구현입니다.
> Chromium 과 WebDriver 팀 멤버에 의해 개발되었습니다.

Electron의 [releases](https://github.com/atom/electron/releases) 페이지에서 `chromedriver` 릴리즈 압축파일을 찾을 수 있습니다.
`chromedriver`의 Electron 배포판과 upstream과의 차이는 없습니다.
`chromedriver`와 Electron을 함께 사용하려면, 몇가지 설정이 필요합니다.

또한 releases에는 `chromedriver`를 포함하여 주 버전만 업데이트 됩니다. (예시: `vX.X.0` releases)
왜냐하면 `chromedriver`는 Electron 자체에서 자주 업데이트하지 않기 때문입니다.

## WebDriverJs 설정하기

[WebDriverJs](https://code.google.com/p/selenium/wiki/WebDriverJs)는 WebDriver를 사용하여 테스팅 할 수 있도록 도와주는 node 패키지입니다.
다음 예제를 참고하세요:

### 1. 크롬 드라이버 시작

먼저, `chromedriver` 바이너리를 다운로드 받고 실행합니다:

```bash
$ ./chromedriver
Starting ChromeDriver (v2.10.291558) on port 9515
Only local connections are allowed.
```

포트 `9515`를 기억하세요, 나중에 사용합니다

### 2. WebDriverJS 설치

```bash
$ npm install selenium-webdriver
```

### 3. 크롬 드라이버에 연결

`selenium-webdriver`를 Electron과 같이 사용할 땐 기본적으로 upstream과 같습니다.
한가지 다른점이 있다면 수동으로 크롬 드라이버 연결에 대해 설정하고 , Electron 실행파일의 위치를 전달합니다:

```javascript
var webdriver = require('selenium-webdriver');

var driver = new webdriver.Builder()
  // 작동하고 있는 크롬 드라이버의 포트 "9515"를 사용합니다.
  .usingServer('http://localhost:9515')
  .withCapabilities({chromeOptions: {
    // 여기에 사용중인 Electron 바이너리의 경로를 기재하세요.
    binary: '/Path-to-Your-App.app/Contents/MacOS/Atom'}})
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

## 작업환경

따로 Electron을 다시 빌드하지 않는다면, 간단히 어플리케이션을 Electron의 리소스 디렉터리에
[배치](https://github.com/atom/electron/blob/master/docs/tutorial/application-distribution-ko.md)하여 바로 테스트 할 수 있습니다.

[chrome-driver]: https://sites.google.com/a/chromium.org/chromedriver/
