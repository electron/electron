# process

> process 객체에 대한 확장 기능.

`process` 객체는 Electron에서 약간 추가적인 기능이 있으며 API는 다음과 같습니다:

## Events

### Event: 'loaded'

Electron 내부 초기화 스크립트의 로드가 완료되고, 웹 페이지나 메인 스크립트를 로드하기
시작할 때 발생하는 이벤트입니다.

이 이벤트는 preload 스크립트를 통해 node 통합이 꺼져있는 전역 스코프에 node의 전역
심볼들을 다시 추가할 때 사용할 수 있습니다:

```javascript
// preload.js
const _setImmediate = setImmediate
const _clearImmediate = clearImmediate
process.once('loaded', () => {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})
```

## Properties

### `process.noAsar`

이 속성을 `true`로 지정하면 Node 빌트인 모듈의 `asar` 아카이브 지원을 비활성화 시킬
수 있습니다.

### `process.type`

현재 프로세스의 종류입니다. `"browser"` (메인 프로세스) 또는 `"renderer"`가 될 수
있습니다.

### `process.versions.electron`

Electron의 버전 문자열입니다.

### `process.versions.chrome`

Chrome의 버전 문자열입니다.

### `process.resourcesPath`

리소스 디렉토리의 경로입니다.

### `process.mas`

Mac App Store 빌드에선 이 속성이 `true`가 됩니다. 다른 빌드에선 `undefined`가 됩니다.

### `process.windowsStore`

애플리케이션이 Windows Store 앱 (appx) 형태로 작동하고 있을 경우 이 속성이 `true`가
됩니다. 그렇지 않은 경우 `undefined`가 됩니다.

### `process.defaultApp`

애플리케이션이 기본 어플리케이션 형식으로 전달되는 인수와 함께 실행됐을 때, 메인
프로세스에서 이 속성이 `true`가 되며 다른 경우엔 `undefined`가 됩니다.

## Methods

`process` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `process.crash()`

현재 프로세스의 메인 스레드에 크래시를 일으킵니다.

### `process.hang()`

현재 프로세스의 메인 스레드를 중단시킵니다.

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

현재 프로세스 파일 디스크립터의 제한 값을 소프트 제한 `maxDescriptors`의 값이나 OS
하드 제한 중 낮은 값으로 설정합니다.

### `process.getProcessMemoryInfo()`

Returns `Object`:
* `workingSetSize` Integer - 현재 실제 물리적 RAM에 예약된 메모리의 양.
* `peakWorkingSetSize` Integer - 물리적 RAM에 예약된 메모리의 최대 사용량.
* `privateBytes` Integer - 다른 프로세스에 공유되지 않은 JS 힙 또는 HTML 콘텐츠와
  같은 메모리의 양.
* `sharedBytes` Integer - 다른 프로세스와 공유된, 일반적으로 Electron 코드 자체에서
  사용하는 메모리의 양.

현재 프로세스의 메모리 사용량에 대한 정보를 객체 형태로 반환합니다. 참고로 모든 사용량은
킬로바이트로 표기됩니다.

### `process.getSystemMemoryInfo()`

Returns `Object`:
* `total` Integer - 시스템에서 사용할 수 있는 킬로바이트 단위의 전체 물리적 메모리의
  양.
* `free` Integer - 애플리케이션이나 디스크 캐시로 사용되지 않은 메모리의 양.
* `swapTotal` Integer - 시스템에서 사용할 수 있는 킬로바이트 단위의 스왑 메모리
  전체 양.  _Windows_ _Linux_
* `swapFree` Integer - 시스템에서 사용할 수 있는 킬로 바이트 단위의 사용되지 않은
  스왑 메모리의 양. _Windows_ _Linux_

전체 시스템의 메모리 사용량에 대한 정보를 객체 형태로 반환합니다. 참고로 모든 사용량은
킬로바이트로 표기됩니다.
