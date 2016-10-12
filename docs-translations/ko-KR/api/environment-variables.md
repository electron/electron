# 환경 변수

> 애플리케이션의 구성과 동작을 코드 변경 없이 제어합니다.

특정 Electron 동작은 명령줄 플래그와 애플리케이션의 코드보다 먼저 초기화되어야 하므로
환경 변수에 의해 작동합니다.

POSIX 쉘의 예시입니다:

```bash
$ export ELECTRON_ENABLE_LOGGING=true
$ electron
```

Windows 콘솔의 예시입니다:

```powershell
> set ELECTRON_ENABLE_LOGGING=true
> electron
```

## 제품 변수

다음 환경 변수는 Electron 애플리케이션 패키지 실행에 우선적으로 사용됩니다.

### `GOOGLE_API_KEY`

Electron 은 하드코딩 된 구글의 위치정보 웹서비스 요청을 위한 API 키를 포함하고
있습니다. 이 API 키가 모든 버전의 Electron 에 포함되어 있기 때문에 종종
사용량을 초과합니다. 이 문제를 해결하기 위해 자신의 구글 API 키를 사용할 수
있습니다. 메인 프로세스 파일에 다음 코드를 위치정보 요청이 있는 브라우저를 열기
전에 넣어주세요.

```javascript
process.env.GOOGLE_API_KEY = 'YOUR_KEY_HERE'
```

구글 API 키를 획득하는 방법은 다음 페이지를 참고하세요.
https://www.chromium.org/developers/how-tos/api-keys

기본적으로, 새로 생성된 구글 API 키는 위치정보 요청이 허용되지 않습니다.
위치정보 요청을 사용하려면 다음 페이지를 방문하세요:
https://console.developers.google.com/apis/api/geolocation/overview

## 개발 변수

다음 환경 변수는 개발과 디버깅시 우선적으로 사용됩니다.

### `ELECTRON_RUN_AS_NODE`

프로세스를 일반 Node.js 프로세스처럼 시작합니다. (electron 모듈 제외)

### `ELECTRON_ENABLE_LOGGING`

Chrome의 내부 로그를 콘솔에 출력합니다.

### `ELECTRON_LOG_ASAR_READS`

Electron이 ASAR 파일을 읽을 때, 읽기 오프셋의 로그를 남기고 시스템 `tmpdir`에 파일로
저장합니다. 결과 파일은 ASAR 모듈의 파일 순서를 최적화 하는데 사용할 수 있습니다.

### `ELECTRON_ENABLE_STACK_DUMPING`

Electron이 크래시되면, 콘솔에 stack trace를 출력합니다.

이 환경 변수는 `crashReporter`가 시작되지 않았을 경우 작동하지 않습니다.

### `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

Electron이 크래시되면 스택 출력 정보를 콘솔에 출력합니다.

이 환경 변수는 `crashReporter`가 시작되지 않았을 경우 작동하지 않습니다.

### `ELECTRON_NO_ATTACH_CONSOLE` _Windows_

현재 콘솔 세션에 소속시키지 않습니다.

### `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

Linux의 전역 메뉴바를 사용하지 않습니다.
