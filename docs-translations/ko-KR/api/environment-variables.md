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
