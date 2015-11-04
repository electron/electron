# 네이티브 node 모듈 사용하기

Electron에선 node.js 네이티브 모듈이 지원됩니다. 하지만 Electron은 공식 node.js의 V8 엔진과는 다른 V8 버전을 사용합니다.
이러한 이유로 네이티브 모듈을 사용하기 위해선 Electron의 V8 버전에 맞춰 네이티브 모듈을 다시 빌드하고 헤더를 변경해야 합니다.

## 네이티브 node 모듈 호환성

네이티브 모듈은 node.js가 새로운 V8 버전을 사용함으로 인해 작동하지 않을 수 있습니다.
사용하는 네이티브 모듈이 Electron에 맞춰 작동할 수 있도록 하려면 Electron에서 사용하는 node.js의 버전을 확인할 필요가 있습니다.
Electron에서 사용하는 node 버전은 [releases](https://github.com/atom/electron/releases)에서 확인할 수 있으며
`process.version`을 출력하여 버전을 확인할 수도 있습니다. ([시작하기](https://github.com/atom/electron/blob/master/docs/tutorial/quick-start.md)의 예제를 참고하세요)

혹시 직접 만든 네이티브 모듈이 있다면 [NAN](https://github.com/nodejs/nan/) 모듈을 사용하는 것을 고려해보는 것이 좋습니다.
이 모듈은 다중 버전의 node.js를 지원하기 쉽게 해줍니다. 이를 통해 오래된 모듈을 새 버전의 node.js에 맞게 포팅할 수 있습니다.
Electron도 이 모듈을 통해 포팅된 네이티브 모듈을 사용할 수 있습니다.

## 네이티브 모듈을 설치하는 방법

네이티브 모듈을 설치하는 방법은 세 가지 종류가 있습니다.

### 쉬운 방법

[`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild) 패키지를 사용하면 빠르고 간단하게 네이티브 모듈을 다시 빌드할 수 있습니다.

다음 예제는 `electron-rebuild`를 통해 자동으로 모듈의 헤더를 다운로드하고 네이티브 모듈을 빌드합니다:

```sh
npm install --save-dev electron-rebuild

# 필요한 네이티브 모듈을 `npm install`로 설치한 후 다음 명령을 실행하세요:
./node_modules/.bin/electron-rebuild

# Windows에서 문제가 발생하면 다음 명령을 대신 실행하세요:
.\node_modules\.bin\electron-rebuild.cmd
```

### `npm`을 이용한 방법

또한 `npm`을 통해 설치할 수도 있습니다.
환경변수가 필요한 것을 제외하고 일반 Node 모듈을 설치하는 방법과 완전히 똑같습니다:

```bash
export npm_config_disturl=https://atom.io/download/atom-shell
export npm_config_target=0.33.1
export npm_config_arch=x64
export npm_config_runtime=electron
HOME=~/.electron-gyp npm install module-name
```

### `node-gyp`를 이용한 방법

Node 모듈을 `node-gyp`를 사용하여 Electron을 타겟으로 빌드할 때는 `node-gyp`에 헤더 다운로드 주소와 버전을 알려주어야 합니다:

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/atom-shell
```

`HOME=~/.electron-gyp`은 변경할 헤더의 위치를 찾습니다. `--target=0.29.1`은 Electron의 버전입니다.
`--dist-url=...`은 헤더를 다운로드 하는 주소입니다. `--arch=x64`는 64비트 시스템을 타겟으로 빌드 한다는 것을 `node-gyp`에게 알려줍니다.
