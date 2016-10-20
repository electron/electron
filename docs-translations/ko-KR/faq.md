# Electron FAQ

## 언제 Electron이 최신 버전의 Chrome으로 업그레이드 되나요?

Electron의 Chrome 버전은 보통 새로운 Chrome 안정 버전이 릴리즈 된 이후 1주 내지 2주
내로 업데이트됩니다. 하지만 이러한 업데이트 주기는 보장되지 않으며 업그레이드에 필요한
작업의 양에 따라 달라집니다.

Electron은 크롬이 사용하는 안정된 채널만을 이용합니다, 만약 중요한 수정이 베타 또는
개발 채널에 패치된 경우, 이전 버전의 채널로 롤백합니다.

자세한 내용은 [보안 설명](tutorial/security.md)을 참고하세요.

## Electron은 언제 최신 버전의 Node.js로 업그레이드 하나요?

새로운 버전의 Node.js가 릴리즈 되면, 보통 Electron을 업그레이드 하기 전에 한 달 정도
대기합니다. 이렇게 하면 새로운 Node.js 버전을 업데이트 함으로써 발생하는 버그들을
피할 수 있기 때문입니다. 이러한 상황은 자주 발생합니다.

Node.js의 새로운 기능은 보통 V8 업그레이드에서 가져옵니다. Electron은 Chrome
브라우저에 탑재된 V8을 사용하고 있습니다. 눈부신 새로운 Node.js 버전의 자바스크립트
기능은 보통 이미 Electron에 있습니다.

## 어떻게 웹 페이지 간에 데이터를 공유할 수 있나요?

두 웹페이지 간에 (렌더러 프로세스) 데이터를 공유하려면 간단히 이미 모든 브라우저에서
사용할 수 있는 HTML5 API들을 사용하면 됩니다. 가장 좋은 후보는
[Storage API][storage], [`localStorage`][local-storage],
[`sessionStorage`][session-storage], 그리고 [IndexedDB][indexed-db]가 있습니다.

또는 Electron에서만 사용할 수 있는 IPC 시스템을 사용하여 메인 프로세스의 global
변수에 데이터를 저장한 후 다음과 같이 렌더러 프로세스에서 `electron` 모듈의 `remote`
속성을 통하여 접근할 수 있습니다:

```javascript
// 메인 프로세스에서
global.sharedObject = {
  someProperty: 'default value'
}
```

```javascript
// 첫 번째 페이지에서
require('electron').remote.getGlobal('sharedObject').someProperty = 'new value'
```

```javascript
// 두 번째 페이지에서
console.log(require('electron').remote.getGlobal('sharedObject').someProperty)
```

## 제작한 애플리케이션의 윈도우/트레이가 몇 분 후에나 나타납니다.

이러한 문제가 발생하는 이유는 보통 윈도우/트레이를 담은 변수에 가비지 컬렉션이 작동해서
그럴 가능성이 높습니다.

이러한 문제를 맞닥뜨린 경우 다음 문서를 읽어보는 것이 좋습니다:

* [메모리 관리][memory-management]
* [변수 스코프][variable-scope]

만약 빠르게 고치고 싶다면, 다음과 같이 변수를 전역 변수로 만드는 방법이 있습니다:

```javascript
app.on('ready', () => {
  const tray = new Tray('/path/to/icon.png')
})
```

를 이렇게:

```javascript
let tray = null
app.on('ready', () => {
  tray = new Tray('/path/to/icon.png')
})
```

## Electron에서 jQuery/RequireJS/Meteor/AngularJS를 사용할 수 없습니다.

Node.js가 Electron에 합쳐졌기 때문에, DOM에 `module`, `exports`, `require` 같은
몇 가지 심볼들이 추가됬습니다. 따라서 같은 이름의 심볼을 사용하는 몇몇 라이브러리들과
충돌이 발생할 수 있습니다.

이러한 문제를 해결하려면, Electron에서 node 포함을 비활성화시켜야 합니다:

```javascript
// 메인 프로세스에서.
let win = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
})
```

하지만 Node.js의 기능과 Electron API를 유지하고 싶다면 페이지에 다른 라이브러리를
추가하기 전에 심볼들의 이름을 변경해야 합니다:

```html
<head>
<script>
window.nodeRequire = require;
delete window.require;
delete window.exports;
delete window.module;
</script>
<script type="text/javascript" src="jquery.js"></script>
</head>
```

## `require('electron').xxx`가 undefined를 반환합니다.

Electron의 빌트인 모듈을 사용할 때, 다음과 같은 오류가 발생할 수 있습니다:

```
> require('electron').webFrame.setZoomFactor(1.0);
Uncaught TypeError: Cannot read property 'setZoomLevel' of undefined
```

이러한 문제가 발생하는 이유는 [npm의 `electron` 모듈][electron-module]이 로컬 또는
전역 중 한 곳에 설치되어, Electron의 빌트인 모듈을 덮어씌우는 바람에 빌트인 모듈을
사용할 수 없기 때문입니다.

올바른 빌트인 모듈을 사용하고 있는지 확인하고 싶다면, `electron` 모듈의 경로를
출력하는 방법이 있습니다:

```javascript
console.log(require.resolve('electron'))
```

그리고 다음과 같은 경로를 가지는지 점검하면 됩니다:

```
"/path/to/Electron.app/Contents/Resources/atom.asar/renderer/api/lib/exports/electron.js"
```

하지만 `node_modules/electron/index.js`와 같은 경로로 되어있을 경우, `electron`
모듈을 지우거나 이름을 바꿔야만 합니다.

```bash
npm uninstall electron
npm uninstall -g electron
```

그런데 여전히 빌트인 모듈이 계속해서 문제를 발생시키는 경우, 아마 모듈을 잘못 사용하고
있을 가능성이 큽니다. 예를 들면 `electron.app`은 메인 프로세스에서만 사용할 수 있는
모듈이며, 반면 `electron.webFrame` 모듈은 렌더러 프로세스에서만 사용할 수 있는
모듈입니다.

[memory-management]: https://developer.mozilla.org/ko/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
[electron-module]: https://www.npmjs.com/package/electron
[storage]: https://developer.mozilla.org/ko/docs/Web/API/Storage
[local-storage]: https://developer.mozilla.org/ko/docs/Web/API/Window/localStorage
[session-storage]: https://developer.mozilla.org/ko/docs/Web/API/Window/sessionStorage
[indexed-db]: https://developer.mozilla.org/ko/docs/Web/API/IndexedDB_API
