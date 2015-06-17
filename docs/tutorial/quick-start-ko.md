# 시작하기

## 소개

Electron은 자바스크립트와 함께 제공되는 풍부한 네이티브 API를 이용하여 데스크톱 어플리케이션을 만들 수 있도록 해주는 프레임워크입니다.
이 프레임워크의 io.js(node.js)는 웹 서버 개발이 아닌 데스크톱 어플리케이션 개발에 초점을 맞췄습니다.

이것은 Electron이 GUI 라이브러리의 자바스크립트 바인딩이라는 뜻이 아닙니다.
대신에, Electron은 웹 페이지의 GUI를 사용합니다. 쉽게 말해 Electron은 자바스크립트를 사용하여 조작하는 작은 Chromium
 브라우저로 볼 수 있습니다.

### 메인 프로세스

Electron은 실행될 때 __메인 프로세스__ 로 불리는 `package.json`의 `main` 스크립트를 호출합니다.
이 스크립트는 메인 프로세스에서 작동합니다, GUI 컴포넌트를 컨트롤하거나 웹 페이지 창을 생성할 수 있습니다.

### 랜더러 프로세스

Electron이 웹페이지를 보여줄 때, Chromium의 multi-processes 구조도 같이 사용됩니다.
Electron 프로세스 내에서 작동하는 웹 페이지는 __랜더러 프로세스__ 라고 불립니다.

보통, 일반 브라우저의 웹 페이지들은 샌드박스가 적용된 환경에서 작동하며 네이티브 리소스에는 접근할 수 없도록 되어 있습니다.
하지만 Electron은 웹 페이지 내에서 io.js(node.js) API를 사용하여 low-level 수준으로 운영체제와 상호작용할 수 있습니다.

### 메인 프로세스와 랜더러 프로세스의 차이점

메인 프로세스는 `BrowserWindow` Class를 이용하여 창을 만들 수 있습니다. `BrowserWindow` 인스턴스는
따로 분리된 프로세스에서 랜더링 되며, `BrowserWindow` 인스턴스가 소멸할 때, 해당하는 랜더러 프로세스도 같이 소멸합니다.

메인 프로세스는 모든 웹 페이지와 그에 해당하는 랜더러 프로세스를 관리하며,
랜더러 프로세스는 각각의 프로세스에 고립되며 웹 페이지의 작동에만 영향을 끼칩니다.

웹 페이지 내에서 네이티브 GUI 리소스를 관리하는 것은 매우 위험하고,
리소스를 누수시킬 수 있기 때문에 웹 페이지 내에서는 네이티브 GUI와 관련된 API를 호출할 수 없도록 되어 있습니다.
만약 웹 페이지 내에서 GUI작업이 필요하다면, 메인 프로세스에서 그 작업을 할 수 있도록 통신을 해야합니다.

Electron에는 메인 프로세스와 랜더러 프로세스간에 통신을 할 수 있도록 [ipc](../api/ipc-renderer-ko.md) 모듈을 제공하고 있습니다.
또한 [remote](../api/remote-ko.md) 모듈을 사용하여 RPC 스타일로 통신할 수도 있습니다.

## 나의 첫번째 Electron 앱 만들기

보통, Electron 앱은 다음과 같은 폴더 구조를 가집니다:

```text
your-app/
├── package.json
├── main.js
└── index.html
```

`package.json`은 node 모듈의 package.json과 같습니다, 그리고 `main` 필드를 지정하여
메인 프로세스로 사용할 어플리케이션 시작점을 정의할 수 있습니다.
예를 들어 사용할 수 있는 `package.json`은 다음과 같습니다:

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

`main.js`에서 창을 만들거나 시스템 이벤트를 처리할 수 있습니다, 대표적인 예제로 다음과 같이 작성할 수 있습니다:

```javascript
var app = require('app');  // 어플리케이션 기반을 조작 하는 모듈.
var BrowserWindow = require('browser-window');  // 네이티브 브라우저 창을 만드는 모듈.

// Electron 개발자에게 crash-report를 보냄.
require('crash-reporter').start();

// 윈도우 객체를 전역에 유지합니다, 만약 이렇게 하지 않으면,
// 자바스크립트 GC가 일어날 때 창이 자동으로 닫혀버립니다.
var mainWindow = null;

// 모든 창이 닫히면 어플리케이션 종료.
app.on('window-all-closed', function() {
  if (process.platform != 'darwin') {
    app.quit();
  }
});

// 이 메서드는 Electron의 초기화가 모두 끝나고
// 브라우저 창을 열 준비가 되었을 때 호출됩니다.
app.on('ready', function() {
  // 새로운 브라우저 창을 생성합니다.
  mainWindow = new BrowserWindow({width: 800, height: 600});

  // 그리고 현재 디렉터리의 index.html을 로드합니다.
  mainWindow.loadUrl('file://' + __dirname + '/index.html');

  // 개발자 콘솔을 엽니다.
  mainWindow.openDevTools();

  // 창이 닫히면 호출됩니다.
  mainWindow.on('closed', function() {
    // 윈도우 객체의 참조를 삭제합니다, 보통 멀티 윈도우 지원을 위해
    // 윈도우 객체를 배열에 저장하는 경우가 있는데, 이 경우
    // 해당하는 모든 윈도우 객체의 참조를 삭제해 주어야 합니다.
    mainWindow = null;
  });
});
```

마지막으로, 사용자에게 보여줄 `index.html` 웹 페이지의 예제입니다:

```html
<!DOCTYPE html>
<html>
  <head>
    <title>헬로 월드!</title>
  </head>
  <body>
    <h1>헬로 월드!</h1>
    우리는 io.js <script>document.write(process.version)</script> 버전과
    Electron <script>document.write(process.versions['electron'])</script> 버전을 사용합니다.
  </body>
</html>
```

## 앱 실행하기

앱을 작성한 후, [어플리케이션 배포](./application-distribution-ko.md) 가이드를 따라 앱을 패키징 하고
패키징한 앱을 실행해 볼 수 있습니다. 또한 Electron 실행파일을 다운로드 받아 바로 실행해 볼 수도 있습니다.

Windows의 경우:

```bash
$ .\electron\electron.exe your-app\
```

Linux의 경우:

```bash
$ ./electron/electron your-app/
```

OS X의 경우:

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

앱 실행파일은 `Electron`의 release 패키지에 포함되어 있습니다.
[여기](https://github.com/atom/electron/releases)에서 다운로드 받을 수 있습니다.
