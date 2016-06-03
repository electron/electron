# 메인 프로세스 디버깅하기

브라우저 창의 개발자 도구는 웹 페이지 같은 렌더러 프로세스의 스크립트만 디버깅이
가능합니다. 대신 Electron은 메인 프로세스의 디버깅을 위해 `--debug` 과 `--debug-brk`
스위치들을 제공합니다.

## 커맨드 라인 스위치(command line switches)

다음 스위치들을 사용하여 Electron의 메인 프로세스를 디버깅 할 수 있습니다:

### `--debug=[port]`

이 스위치를 사용하면 Electron은 지정한 `port`에 V8 디버거 프로토콜을 리스닝합니다.
기본 `port`는 `5858` 입니다.

### `--debug-brk=[port]`

`--debug`와 비슷하지만 스크립트의 첫번째 라인에서 일시정지합니다.

## node-inspector로 디버깅 하기

**참고:** Electron은 현재 node-inspector 유틸리티와 호환성 문제가 있습니다. 따라서
node-inspector 콘솔 내에서 메인 프로세스의 `process` 객체를 탐색할 경우 크래시가
발생할 수 있습니다.

### 1. [node-gyp 필수 도구][node-gyp-required-tools]를 설치했는지 확인

### 2. [node-inspector][node-inspector] 설치

```bash
$ npm install node-inspector
```

### 3. 패치된 버전의 `node-pre-gyp` 설치

```bash
$ npm install git+https://git@github.com/enlight/node-pre-gyp.git#detect-electron-runtime-in-find
```

### 4. Electron용 `node-inspector` `v8` 모듈을 재 컴파일 (target을 사용하는 Electron의 버전에 맞춰 변경)

```bash
$ node_modules/.bin/node-pre-gyp --target=0.36.11 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/atom-shell reinstall
$ node_modules/.bin/node-pre-gyp --target=0.36.11 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/atom-shell reinstall
```

또한 [네이티브 모듈을 사용하는 방법][how-to-install-native-modules] 문서도 참고해보세요.

### 5. Electron 디버그 모드 활성화

다음과 같이 debung 플래그로 Electron을 실행할 수 있습니다:

```bash
$ electron --debug=5858 your/app
```

또는 스크립트 첫번째 라인에서 일시 정지할 수 있습니다:

```bash
$ electron --debug-brk=5858 your/app
```

### 6. Electron을 사용하는 [node-inspector][node-inspector] 시작

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. 디버거 UI 로드

Chrome 브라우저에서 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858 주소에
접속합니다. (기본 포트 또는 지정한 포트로 접속) 엔트리의 라인이 debug-brk로 시작하는
경우 일시정지 버튼을 클릭해야 할 수도 있습니다.

[node-inspector]: https://github.com/node-inspector/node-inspector
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#네이티브-모듈을-설치하는-방법
