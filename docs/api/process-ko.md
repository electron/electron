# 프로세스 객체

Electron의 `process` 객체는 기존의 node와는 달리 약간의 차이점이 있습니다:

* `process.type` String - 프로세스의 타입, `browser` (메인 프로세스) 또는 `renderer`가 됩니다.
* `process.versions['electron']` String - Electron의 버전.
* `process.versions['chrome']` String - Chromium의 버전.
* `process.resourcesPath` String - JavaScript 소스코드의 경로.

## process.hang

현재 프로세스의 주 스레드를 중단시킵니다.
