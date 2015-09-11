# 네이티브 node 모듈 사용하기

Electron에선 node.js 네이티브 모듈이 지원됩니다. 하지만 Electron은 공식 node.js의 V8 엔진과는 다른 V8 버전을 사용합니다.
이러한 이유로 네이티브 모듈을 사용하기 위해선 Electron의 V8 버전에 맞춰 네이티브 모듈을 다시 빌드하고 헤더를 변경해야 합니다.

## 네이티브 node 모듈 호환성

Node v0.11.x 버전부터는 V8 API의 중대한 변경이 있었습니다. 하지만 대부분의 네이티브 모듈은 Node v0.10.x 버전을 타겟으로 작성 되었기 때문에
새로운 Node 또는 io.js 버전에서 작동하지 않을 수 있습니다. Electron은 내부적으로 __io.js v3.1.0__ 버전을 사용하기 때문에 호환성 문제가 발생할 수 있습니다.

이 문제를 해결하기 위해선 모듈이 v0.11.x 또는 최신 버전을 지원할 수 있도록 변경해야 합니다.
현재 [많은 모듈들](https://www.npmjs.org/browse/depended/nan)이 안정적으로 두 버전 모두 지원하고 있지만 오래된 모듈의 경우 여전히 Node v0.10.x 버전만을 지원하고 있습니다.
예를 들어 [nan](https://github.com/rvagg/nan) 모듈을 사용해야 한다면 Node v0.11.x 또는 최신 버전의 Node와 io.js로 포팅 할 필요가 있습니다.

## 네이티브 모듈 설치하는 방법

네이티브 모듈을 설치하는 방법은 세 가지 종류가 있습니다.

### 쉬운 방법

[`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild) 패키지를 사용하면 빠르고 간단하게 네이티브 모듈을 다시 빌드할 수 있습니다.

다음 예제는 `electron-rebuild`를 통해 자동으로 모듈의 헤더를 다운로드하고 네이티브 모듈을 빌드합니다:

```sh
npm install --save-dev electron-rebuild

# 필요한 네이티브 모듈을 `npm install`로 설치한 후 다음 명령을 실행하세요:
node ./node_modules/.bin/electron-rebuild
```

### node-gyp을 이용한 방법

Node 모듈을 `node-gyp`를 사용하여 Electron을 타겟으로 빌드할 때는 `node-gyp`에 헤더 다운로드 주소와 버전을 알려주어야 합니다:

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.29.1 --arch=x64 --dist-url=https://atom.io/download/atom-shell
```

`HOME=~/.electron-gyp`은 변경할 헤더의 위치를 찾습니다. `--target=0.29.1`은 Electron의 버전입니다.
`--dist-url=...`은 헤더를 다운로드 하는 주소입니다. `--arch=x64`는 64비트 시스템을 타겟으로 빌드 한다는 것을 `node-gyp`에게 알려줍니다.

### npm을 이용한 방법

또한 `npm`을 통해 설치할 수도 있습니다.
환경변수가 필요한 것을 제외하고 일반 Node 모듈을 설치하는 방법과 완전히 똑같습니다:

```bash
export npm_config_disturl=https://atom.io/download/atom-shell
export npm_config_target=0.29.1
export npm_config_arch=x64
HOME=~/.electron-gyp npm install module-name
```
