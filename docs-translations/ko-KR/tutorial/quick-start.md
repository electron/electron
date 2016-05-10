# 시작하기

## 소개

Electron은 자바스크립트와 함께 제공된 풍부한 네이티브 API를 사용하여 멋진 데스크탑
어플리케이션을 만들 수 있도록 해주는 프레임워크입니다. 이 프레임워크의 Node.js는 웹
서버 개발이 아닌 데스크탑 어플리케이션 개발에 초점을 맞췄습니다.

이 말은 Electron이 GUI 라이브러리의 자바스크립트 바인딩이라는 뜻이 아닙니다. 대신,
Electron은 웹 페이지의 GUI를 사용합니다. 쉽게 말해 Electron은 자바스크립트를 사용하여
조작하는 작은 Chromium 브라우저로 볼 수 있습니다.

### 메인 프로세스

Electron은 실행될 때 __메인 프로세스__ 로 불리는 `package.json`의 `main` 스크립트를
호출합니다. 이 스크립트는 메인 프로세스에서 작동합니다. GUI 컴포넌트를 조작하거나 웹
페이지 창을 생성할 수 있습니다.

### 렌더러 프로세스

Electron이 웹페이지를 보여줄 때 Chromium의 multi-processes 구조도 같이 사용됩니다.
Electron 프로세스 내에서 작동하는 웹 페이지를 __렌더러 프로세스__ 라고 불립니다.

보통 일반 브라우저의 웹 페이지들은 샌드박스가 적용된 환경에서 작동하며 네이티브
리소스에는 접근할 수 없도록 되어 있습니다. 하지만 Electron은 웹 페이지 내에서 Node.js
API를 사용하여 low-level 수준으로 운영체제와 상호작용할 수 있습니다.

### 메인 프로세스와 렌더러 프로세스의 차이점

메인 프로세스는 `BrowserWindow` Class를 사용하여 새로운 창을 만들 수 있습니다.
`BrowserWindow` 인스턴스는 따로 분리된 프로세스에서 렌더링 되며 이 프로세스를 렌더러
프로세스라고 합니다. `BrowserWindow` 인스턴스가 소멸할 때 그 창의 렌더러 프로세스도
같이 소멸합니다.

메인 프로세스는 모든 웹 페이지와 렌더러 프로세스를 관리하며 렌더러 프로세스는 각각의
프로세스에 고립되며 웹 페이지의 작동에만 영향을 끼칩니다.

웹 페이지 내에선 기본적으로 네이티브 GUI와 관련된 API를 호출할 수 없도록 설계 되어
있습니다. 왜냐하면 웹 페이지 내에서 네이티브 GUI 리소스를 관리하는 것은 보안에 취약하고
리소스를 누수시킬 수 있기 때문입니다. 꼭 웹 페이지 내에서 API를 사용해야 한다면 메인
프로세스에서 그 작업을 처리할 수 있도록 메인 프로세스와 통신을 해야 합니다.

Electron에는 메인 프로세스와 렌더러 프로세스 사이에 통신을 할 수 있도록
[ipc](../api/ipc-renderer.md) 모듈을 제공하고 있습니다.
또는 [remote](../api/remote.md) 모듈을 사용하여 RPC 스타일로 통신할 수도 있습니다.
또한 FAQ에서 [다양한 객체를 공유하는 방법](share-data)도 소개하고 있습니다.

## 첫번째 Electron 앱 만들기

보통 Electron 앱은 다음과 같은 폴더 구조를 가집니다:

```text
your-app/
├── package.json
├── main.js
└── index.html
```

`package.json`은 node 모듈의 package.json과 같습니다. 그리고 `main` 필드에 스크립트
파일을 지정하면 메인 프로세스의 엔트리 포인트로 사용합니다. 예를 들어 사용할 수 있는
`package.json`은 다음과 같습니다:

```json
{
  "name"    : "your-app",
  "version" : "0.1.0",
  "main"    : "main.js"
}
```

__알림__: 만약 `main` 필드가 `package.json`에 설정되어 있지 않으면 Electron은
자동으로 같은 디렉터리의 `index.js`를 로드합니다.

반드시 `main.js`에서 창을 만들고 시스템 이벤트를 처리해야 합니다. 대표적인 예시로
다음과 같이 작성할 수 있습니다:

```javascript
const electron = require('electron');
// 어플리케이션 생명주기를 조작 하는 모듈.
const {app} = electron;
// 네이티브 브라우저 창을 만드는 모듈.
const {BrowserWindow} = electron;

// 윈도우 객체를 전역에 유지합니다. 만약 이렇게 하지 않으면
// 자바스크립트 GC가 일어날 때 창이 멋대로 닫혀버립니다.
let win;

function createWindow () {
  // 새로운 브라우저 창을 생성합니다.
  win = new BrowserWindow({width: 800, height: 600});

  // 그리고 현재 디렉터리의 index.html을 로드합니다.
  win.loadURL(`file://${__dirname}/index.html`);

  // 개발자 도구를 엽니다.
  win.webContents.openDevTools();

  // 창이 닫히면 호출됩니다.
  win.on('closed', () => {
    // 윈도우 객체의 참조를 삭제합니다. 보통 멀티 윈도우 지원을 위해
    // 윈도우 객체를 배열에 저장하는 경우가 있는데 이 경우
    // 해당하는 모든 윈도우 객체의 참조를 삭제해 주어야 합니다.
    win = null;
  });
}

// 이 메서드는 Electron의 초기화가 끝나면 실행되며 브라우저
// 윈도우를 생성할 수 있습니다. 몇몇 API는 이 이벤트 이후에만
// 사용할 수 있습니다.
app.on('ready', createWindow);

// 모든 창이 닫히면 어플리케이션 종료.
app.on('window-all-closed', () => {
  // OS X의 대부분의 어플리케이션은 유저가 Cmd + Q 커맨드로 확실하게
  // 종료하기 전까지 메뉴바에 남아 계속 실행됩니다.
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  // OS X에선 보통 독 아이콘이 클릭되고 나서도
  // 열린 윈도우가 없으면, 새로운 윈도우를 다시 만듭니다.
  if (win === null) {
    createWindow();
  }
});

// 이 파일엔 제작할 어플리케이션에 특화된 메인 프로세스 코드를
// 포함할 수 있습니다. 또한 파일을 분리하여 require하는 방법으로
// 코드를 작성할 수도 있습니다.
```

마지막으로, 사용자에게 보여줄 `index.html` 웹 페이지의 예시입니다:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>헬로 월드!</title>
  </head>
  <body>
    <h1>헬로 월드!</h1>
    이 어플리케이션은 node <script>document.write(process.version)</script>,
    Chrome <script>document.write(process.versions.chrome)</script>,
    Electron <script>document.write(process.versions.electron)</script>을 사용합니다.
  </body>
</html>
```

## 앱 실행하기

앱을 작성한 후 [어플리케이션 배포](application-distribution.md) 가이드를 따라 앱을
패키징 하고 패키징한 앱을 실행할 수 있습니다. 또한 Electron 실행파일을 다운로드 받아
바로 실행해 볼 수도 있습니다.

### electron-prebuilt

[`electron-prebuilt`](https://github.com/electron-userland/electron-prebuilt)는
Electron의 미리 컴파일된 바이너리를 포함하는 `npm` 모듈입니다.

만약 `npm`을 통해 전역에 이 모듈을 설치했다면, 어플리케이션 소스 디렉터리에서 다음
명령을 실행하면 바로 실행할 수 있습니다:

```bash
electron .
```

또는 앱 디렉터리 밖에서 앱을 실행할 수도 있습니다:

```bash
electron app
```

npm 모듈을 로컬에 설치했다면 다음 명령으로 실행할 수 있습니다:

```bash
./node_modules/.bin/electron .
```

### 다운로드 받은 Electron 바이너리 사용

따로 Electron 바이너리를 다운로드 받았다면 다음 예시와 같이 실행하면 됩니다.

#### Windows

```bash
$ .\electron\electron.exe your-app\
```

#### Linux

```bash
$ ./electron/electron your-app/
```

#### OS X

```bash
$ ./Electron.app/Contents/MacOS/Electron your-app/
```

어플리케이션 실행파일은 `Electron`의 release 패키지에 포함되어 있습니다.
[여기](https://github.com/electron/electron/releases)에서 다운로드 받을 수 있습니다.

### 배포용 실행 파일 만들기

어플리케이션 작성을 모두 끝냈다면 [어플리케이션 배포](application-distribution.md)
가이드를 통해 제작한 앱을 패키징하고 배포할 수 있습니다.

### 미리 작성된 앱 실행하기

[`atom/electron-quick-start`](https://github.com/electron/electron-quick-start)
저장소를 클론하면 이 문서에서 작성한 예시 앱을 바로 실행해 볼 수 있습니다.

**참고**: 이 예시를 실행시키려면 [Git](https://git-scm.com)과
[Node.js](https://nodejs.org/en/download/)가 필요합니다. (CLI에서 실행 가능한
  [npm](https://npmjs.org)이 있어야 합니다)

**역주**: `npm`은 보통 Node.js를 설치하면 자동으로 같이 설치됩니다.

```bash
# 저장소를 클론합니다
$ git clone https://github.com/electron/electron-quick-start
# 저장소 안으로 들어갑니다
$ cd electron-quick-start
# 어플리케이션의 종속성 모듈을 설치한 후 실행합니다
$ npm install && npm start
```

[share-data]: ../faq/electron-faq.md#어떻게-웹-페이지-간에-데이터를-공유할-수-있나요
