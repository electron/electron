# 네이티브 node 모듈 사용하기

__역자주: 현재 Electron은 node.js대신 io.js를 사용합니다. 문서에 기재된 버전과 다를 수 있습니다__

Electron에선 node.js 네이티브 모듈이 지원됩니다. 하지만 Electron은 공식 node.js의 V8 엔진과는 달리 다른 V8 버전을 사용합니다.
그런 이유로 네이티브 모듈을 사용하기 위해선 Electron의 V8 버전에 맞춰 네이티브 모듈을 다시 빌드하고 헤더를 변경해야 합니다.

## 네이티브 node 모듈 호환성

Node v0.11.x 버전부터는 V8 API의 중대한 변경이 있었습니다. 하지만 일반적으로 모든 네이티브 모듈은 Node v0.10.x 버전을 타겟으로 작성 되었기 때문에
Node v0.11.x 버전에선 작동하지 않습니다. Electron은 내부적으로 Node v0.11.13 버전을 사용합니다. 그래서 위에서 설명한 문제가 발생합니다.

이 문제를 해결하기 위해, 모듈이 Node v0.11.x 버전을 지원할 수 있도록 해야합니다.
현재 [많은 모듈들](https://www.npmjs.org/browse/depended/nan)이 안정적으로 두 버전 모두 지원하고 있지만,
오래된 모듈의 경우 Node v0.10.x 버전만을 지원하고 있습니다.
예를들어 [nan](https://github.com/rvagg/nan) 모듈을 사용해야 하는 경우, Node v0.11.x 버전으로 포팅 할 필요가 있습니다.

## 네이티브 모듈 설치하는 방법

### 쉬운 방법 - 권장

[`electron-rebuild`](https://github.com/paulcbetts/electron-rebuild) 패키지를 사용하면 아주 빠르고 정확하게 네이티브 모듈을 다시 빌드할 수 있습니다.
다음의 간단한 절차를 통해 자동으로 헤더를 다운로드하고 네이티브 모듈을 빌드할 수 있습니다:

```sh
npm install --save-dev electron-rebuild

# 필요한 네이티브 모듈을 `npm install`로 설치한 후 다음 작업을 실행하세요:
./node_modules/.bin/electron-rebuild
```

### node-gyp을 이용한 방법

Node 모듈을 `node-gyp`를 사용하여 Electron을 타겟으로 빌드할 땐, `node-gyp`에 헤더 다운로드 주소와 버전을 알려주어야합니다:

```bash
$ cd /path-to-module/
$ HOME=~/.electron-gyp node-gyp rebuild --target=0.25.0 --arch=ia64 --dist-url=https://atom.io/download/atom-shell
```



`HOME=~/.electron-gyp`은 변경할 헤더의 위치를 찾습니다. `--target=0.25.0`은 Electron의 버전입니다.
`--dist-url=...`은 헤더를 다운로드 하는 주소입니다. `--arch=ia64`는 64비트 시스템을 타겟으로 빌드 한다는 것을 `node-gyp`에게 알려줍니다.

### npm을 이용한 방법

또한 `npm`을 사용하여 모듈을 설치할 수도 있습니다.
환경변수가 필요한 것을 제외하고는 일반 Node 모듈을 설치하는 방법과 완전히 똑같습니다:

```bash
export npm_config_disturl=https://atom.io/download/atom-shell
export npm_config_target=0.25.0
export npm_config_arch=x64
HOME=~/.electron-gyp npm install module-name
```
