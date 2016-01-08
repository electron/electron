# Electron FAQ

## 언제 Electron이 최신 버전의 Chrome으로 업그레이드 되나요?

Electron의 Chrome 버전은 보통 새로운 Chrome 안정 버전이 릴리즈 된 이후 1주 내지 2주
내로 업데이트됩니다.

또한 우리는 크롬의 안정된 채널만을 이용합니다, 만약 중요한 수정이 베타 또는 개발 채널인
경우, 우리는 해당 버전 대신 이전 버전을 다시 사용합니다.

## Electron은 언제 최신 버전의 Node.js로 업그레이드 하나요?

새로운 버전의 Node.js가 릴리즈 되면, 우리는 보통 Electron을 업그레이드 하기 전에 한
달 정도 대기합니다. 이렇게 하면 새로운 Node.js 버전을 업데이트 함으로써 발생하는
버그들을 피할 수 있습니다. 이러한 상황은 자주 발생합니다.

Node.js의 새로운 기능은 보통 V8 업그레이드에서 가져옵니다. Electron은 Chrome
브라우저에 탑재된 V8을 사용하고 있습니다. 눈부신 새로운 Node.js 버전의 자바스크립트
기능은 보통 이미 Electron에 있습니다.

## 제작한 어플리케이션의 윈도우/트레이가 몇 분 후에나 나타납니다.

이러한 문제가 발생하는 이유는 보통 윈도우/트레이를 담은 변수에 가비지 컬렉션이 작동해서
그럴 가능성이 높습니다.

이러한 문제를 맞닥뜨린 경우 다음 문서를 읽어보는 것이 좋습니다:

* [메모리 관리][memory-management]
* [변수 스코프][variable-scope]

만약 빠르게 고치고 싶다면, 다음과 같이 변수를 전역 변수로 만드는 방법이 있습니다:

```javascript
app.on('ready', function() {
  var tray = new Tray('/path/to/icon.png');
})
```

를 이렇게:

```javascript
var tray = null;
app.on('ready', function() {
  tray = new Tray('/path/to/icon.png');
})
```

## Electron에서 jQuery/RequireJS/Meteor/AngularJS를 사용할 수 없습니다.

Node.js가 Electron에 합쳐졌기 때문에, DOM에 `module`, `exports`, `require` 같은
몇 가지 심볼들이 추가됬습니다. 따라서 같은 이름의 심볼을 사용하는 몇몇 라이브러리들과
충돌이 발생할 수 있습니다.

이러한 문제를 해결하려면, Electron에서 node 포함을 비활성화시켜야 합니다:

```javascript
// 메인 프로세스에서.
var mainWindow = new BrowserWindow({
  webPreferences: {
    nodeIntegration: false
  }
});
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

[memory-management]: https://developer.mozilla.org/ko/docs/Web/JavaScript/Memory_Management
[variable-scope]: https://msdn.microsoft.com/library/bzt2dkta(v=vs.94).aspx
