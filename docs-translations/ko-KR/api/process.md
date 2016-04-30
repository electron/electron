# process

Electron의 `process` 객체는 기존의 node와는 달리 약간의 차이점이 있습니다:

* `process.type` String - 프로세스의 타입, `browser` (메인 프로세스) 또는
  `renderer`가 됩니다.
* `process.versions.electron` String - Electron의 버전.
* `process.versions.chrome` String - Chromium의 버전.
* `process.resourcesPath` String - JavaScript 소스 코드의 경로.
* `process.mas` Boolean - Mac 앱 스토어용 빌드일 때 `true`로 지정됩니다. 다른
  빌드일 땐 `undefined`로 지정됩니다.
* `process.windowsStore` Boolean - 만약 앱이 Windows Store 앱 (appx)으로 작동하고
  있다면, 이 값이 `true`로 지정되며 다른 빌드인 경우엔 `undefined`로 지정됩니다.

## Events

### Event: 'loaded'

Electron 내부 초기화 스크립트의 로드가 완료되고, 웹 페이지나 메인 스크립트를 로드하기
시작할 때 발생하는 이벤트입니다.

이 이벤트는 preload 스크립트를 통해 node 통합이 꺼져있는 전역 스코프에 node의 전역
심볼들을 다시 추가할 때 사용할 수 있습니다:

```javascript
// preload.js
var _setImmediate = setImmediate;
var _clearImmediate = clearImmediate;
process.once('loaded', function() {
  global.setImmediate = _setImmediate;
  global.clearImmediate = _clearImmediate;
});
```

## Properties

### `process.noAsar`

이 속성을 `true`로 지정하면 Node 빌트인 모듈의 `asar` 아카이브 지원을 비활성화 시킬
수 있습니다.

## Methods

`process` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `process.hang()`

현재 프로세스의 주 스레드를 중단시킵니다.

### `process.setFdLimit(maxDescriptors)` _OS X_ _Linux_

* `maxDescriptors` Integer

현재 프로세스 파일 디스크립터의 제한 값을 소프트 제한 `maxDescriptors`의 값이나 OS 하드
제한 중 낮은 값으로 설정합니다.
