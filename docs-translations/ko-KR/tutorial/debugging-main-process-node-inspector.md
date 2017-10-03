# node-inspector 에서 메인 프로세스 디버깅하기

[`node-inspector``][node-inspector] 는 Electron 의 메인 프로세스를 디버깅하기
위해 크롬에서 사용할 수 있는 익숙한 개발도구 GUI 를 제공합니다. 그러나,
`node-inspector` 가 네이티브 Node 모듈에 의존적이기 때문에 디버깅하려는
Electron 버전에 맞춰 다시 빌드해야 합니다. `node-inspector` 다시 빌드하여
의존성을 재구성하거나 [`electron-inspector`] 가 대신 하게 할 수 있으며, 두
방식이 이 문서에 나와있습니다.

**참고**: 글쓴 시점 현재 `node-inspector` 최종버전 (0.12.8) 은 Electron 1.3.0
이상에서 해당 의존성 중 하나를 패치하지 않고 빌드 할 수 없습니다.
`electron-inspector` 을 사용한다면 알아서 처리될 것 입니다.


## 디버깅에 `electron-inspector` 사용하기

### 1. [node-gyp 필수 도구][node-gyp-required-tools] 설치

### 2. [`electron-rebuild`][electron-rebuild]가 없다면, 설치

```shell
npm install electron-rebuild --save-dev
```

### 3. [`electron-inspector`][electron-inspector] 설치

```shell
npm install electron-inspector --save-dev
```

### 4. Electron 시작

`--debug` 스위치와 함께 Electron 실행:

```shell
electron --debug=5858 your/app
```

또는, 스크립트의 첫번째 줄에서 실행 중단:

```shell
electron --debug-brk=5858 your/app
```

### 5. electron-inspector 시작

macOS / Linux:

```shell
node_modules/.bin/electron-inspector
```

Windows:

```shell
node_modules\\.bin\\electron-inspector
```

`electron-inspector` 는 첫 실행과 Electron 버전 변경시에 `node-inspector`
의존성을 다시 빌드 할 것 입니다. 다시 빌드하는 과정에 Node 헤더와 라이브러리를
다운받기 위해 인터넷 연결이 필요하며, 이 작업은 몇 분 정도 시간이 소요됩니다.

### 6. 디버거 UI 로드

Chrome 브라우저에서 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858
주소에 접속합니다. `--debug-brk` 로 시작한 경우 UI 갱신을 위해 일시정지 버튼을
클릭해야 할 수도 있습니다.


## 디버깅에 `node-inspector` 사용하기

### 1. [node-gyp 필수 도구][node-gyp-required-tools] 설치

### 2. [`node-inspector`][node-inspector] 설치

```bash
$ npm install node-inspector
```

### 3. [`node-pre-gyp`][node-pre-gyp] 설치

```bash
$ npm install node-pre-gyp
```

### 4. Electron 용 `node-inspector` `v8` 모듈 재 컴파일

**참고:** 사용하는 Electron의 버전에 맞춰 target 인수를 변경하세요.

```bash
$ node_modules/.bin/node-pre-gyp --target=1.2.5 --runtime=electron --fallback-to-build --directory node_modules/v8-debug/ --dist-url=https://atom.io/download/electron reinstall
$ node_modules/.bin/node-pre-gyp --target=1.2.5 --runtime=electron --fallback-to-build --directory node_modules/v8-profiler/ --dist-url=https://atom.io/download/electron reinstall
```

또한 [네이티브 모듈 설치 방법][how-to-install-native-modules] 문서도 참고해보세요.

### 5. Electron 디버그 모드 활성화

다음과 같이 debung 플래그로 Electron 을 실행할 수 있습니다:

```bash
$ electron --debug=5858 your/app
```

또는 스크립트 첫번째 줄에서 일시 정지할 수 있습니다:

```bash
$ electron --debug-brk=5858 your/app
```

### 6. Electron을 사용하는 [`node-inspector`][node-inspector] 서버 시작하기

```bash
$ ELECTRON_RUN_AS_NODE=true path/to/electron.exe node_modules/node-inspector/bin/inspector.js
```

### 7. 디버거 UI 로드

Chrome 브라우저에서 http://127.0.0.1:8080/debug?ws=127.0.0.1:8080&port=5858
주소에 접속합니다. `--debug-brk` 로 시작한 경우 엔트리 라인을 보기 위해
일시정지 버튼을 클릭해야 할 수도 있습니다.

[electron-inspector]: https://github.com/enlight/electron-inspector
[electron-rebuild]: https://github.com/electron/electron-rebuild
[node-inspector]: https://github.com/node-inspector/node-inspector
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp
[node-gyp-required-tools]: https://github.com/nodejs/node-gyp#installation
[how-to-install-native-modules]: using-native-node-modules.md#how-to-install-native-modules
