# 환경 변수

Electron의 몇몇 동작은 명령 줄과 어플리케이션의 코드보다 먼저 초기화되어야 하므로 환경 변수에 의해 작동합니다.

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

## `ELECTRON_RUN_AS_NODE`

프로세스를 일반 Node.js 프로세스처럼 시작합니다. (electron 모듈 제외)

## `ELECTRON_ENABLE_LOGGING`

Chrome의 내부 로그를 콘솔에 출력합니다.

## `ELECTRON_ENABLE_STACK_DUMPING`

Electron이 크래시되면, 콘솔에 stack trace를 출력합니다.

이 환경 변수는 `crashReporter`가 시작되지 않았을 경우 작동하지 않습니다.

## `ELECTRON_DEFAULT_ERROR_MODE` _Windows_

Electron이 크래시되면, 크래시 정보 창을 표시합니다.

이 환경 변수는 `crashReporter`가 시작되지 않았을 경우 작동하지 않습니다.

## `ELECTRON_NO_ATTACH_CONSOLE` _Windows_

현재 콘솔 세션에 소속시키지 않습니다.

## `ELECTRON_FORCE_WINDOW_MENU_BAR` _Linux_

Linux의 글로벌 메뉴 막대를 사용하지 않습니다.

## `ELECTRON_HIDE_INTERNAL_MODULES`

`require('ipc')`같은 예전 방식의 빌트인 모듈을 비활성화합니다.
