# 메인 프로세스 디버깅하기

브라우저 창의 개발자 콘솔은 랜더러 프로세스의 스크립트만 디버깅이 가능합니다. (다시말해 웹 페이지)
메인 프로세스의 디버깅 방법을 제공하기 위해, Electron은 `--debug` 과 `--debug-brk` 스위치들을 제공합니다.

## 커맨드 라인 스위치(command line switches)

### `--debug=[port]`

이 스위치를 사용하면 Electron은 지정한 `port`에 V8 디버거 프로토콜을 리스닝합니다. `port`는 `5858`이 기본적으로 사용됩니다.

### `--debug-brk=[port]`

`--debug`와 비슷하지만 스크립트의 첫번째 라인에서 일시정지합니다.

## node-inspector로 디버깅 하기

__주의:__ Electron은 node v0.11.13 버전을 사용합니다, node-inspector는 현재 아주 잘 작동하지 않습니다.
그리고 메인 프로세스의 `process`를 node-inspector 콘솔 내에서 검사할 경우 크래시가 발생할 수 있습니다.

### 1. [node-inspector][node-inspector] 서버 시작

```bash
$ node-inspector
```

### 2. Electron용 디버그 모드 활성화

다음과 같이 debung 플래그로 Electron을 실행할 수도 있습니다:

```bash
$ electron --debug=5858 your/app
```

또는 스크립트 첫번째 라인에서 일시정지:

```bash
$ electron --debug-brk=5858 your/app
```

### 3. 디버그 UI 로드

Chrome 브라우저에서 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 주소에 접속합니다. (기본포트 또는 지정한 포트로 접속)

[node-inspector]: https://github.com/node-inspector/node-inspector
