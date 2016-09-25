# 네이티브 Node 모듈 사용하기

Electron에선 네이티브 Node 모듈을 지원합니다. 하지만 Electron이 시스템에 설치된
Node의 버전과 전혀 다른 V8 버전을 사용하고 있을 가능성이 높은 관계로, 네이티브 모듈을
빌드할 때 Electron의 헤더를 수동으로 지정해 주어야 합니다.

## 네이티브 모듈을 설치하는 방법

네이티브 모듈을 설치하는 방법에는 세 가지 방법이 있습니다:

### `npm` 사용하기

몇 가지 환경 변수를 설치하는 것으로, 직접적으로 `npm`을 모듈을 설치하는데 사용할 수
있습니다.

다음 예시는 Electron에 대한 모든 종속성을 설치하는 예시입니다:

```bash
# Electron의 버전.
export npm_config_target=1.2.3
# Electron의 아키텍쳐, ia32 또는 x64가 될 수 있습니다.
export npm_config_arch=x64
export npm_config_target_arch=x64
# Electron에 대한 헤더 다운로드 링크.
export npm_config_disturl=https://atom.io/download/atom-shell
# node-pre-gyp에 Electron을 빌드한다는 것을 알려줍니다.
export npm_config_runtime=electron
# node-pre-gyp에 소스 코드로부터 모듈을 빌드한다는 것을 알려줍니다.
export npm_config_build_from_source=true
# 모든 종속성을 설치하고 캐시를 ~/.electron-gyp에 저장합니다.
HOME=~/.electron-gyp npm install
```

### 모듈을 설치하고 Electron을 위해 다시 빌드하기

다른 Node 프로젝트와 같이 모듈을 설치하는 것을 선택할 수도 있습니다. 그리고
[`electron-rebuild`][electron-rebuild] 패키지와 함께 Electron에 대해 모듈을 다시
빌드할 수 있습니다. 이 모듈은 Electron의 버전을 가져올 수 있고 헤더를 다운로드 하거나
네이티브 모듈을 빌드하는 등의 작업을 자동으로 실행할 수 있습니다.

다음 예시는 `electron-rebuild`을 설치하고 자동으로 모듈을 빌드하는 예시입니다:

```bash
npm install --save-dev electron-rebuild

# "npm install"을 실행할 때마다 다음 명령을 실행하세요:
./node_modules/.bin/electron-rebuild

# Windows에서 문제가 발생하면 다음 명령을 대신 실행하세요:
.\node_modules\.bin\electron-rebuild.cmd
```

**역자주:** `npm script`의 `postinstall`을 사용하면 이 작업을 자동화 할 수 있습니다.

### Electron을 위해 직접적으로 빌드하기

현재 본인이 네이티브 모듈을 개발하고 있는 개발자이고 Electron에 대해 실험하고 싶다면,
직접적으로 모듈을 Electron에 대해 다시 빌드하고 싶을 것입니다. `node-gyp`를 통해
직접적으로 Electron에 대해 빌드할 수 있습니다.

```bash
cd /path-to-module/
HOME=~/.electron-gyp node-gyp rebuild --target=1.2.3 --arch=x64 --dist-url=https://atom.io/download/atom-shell
```

`HOME=~/.electron-gyp`은 변경할 헤더의 위치를 찾습니다. `--target=0.29.1`은
Electron의 버전입니다. `--dist-url=...`은 헤더를 다운로드 하는 주소입니다.
`--arch=x64`는 64비트 시스템을 타겟으로 빌드 한다는 것을 `node-gyp`에게 알려줍니다.

## 문제 해결

네이티브 모듈을 설치했는데 잘 작동하지 않았다면, 다음 몇 가지를 확인해봐야 합니다:

* 모듈의 아키텍쳐가 Electron의 아키텍쳐와 일치합니다. (ia32 또는 x64)
* Electron을 업그레이드한 후, 보통은 모듈을 다시 빌드해야 합니다.
* 문제가 의심된다면, 먼저 `electron-rebuild`를 실행하세요.

## 모듈이 `node-pre-gyp`에 의존합니다

[`node-pre-gyp` 툴][node-pre-gyp]은 미리 빌드된 바이너리와 함께 네이티브 Node 모듈을
배치하는 방법을 제공하며 수 많은 유명한 모듈들이 이를 사용합니다.

보통 이 모듈은 Electron과 함께 잘 작동하지만, Electron이 Node보다 새로운 V8 버전을
사용할 때, 만약 ABI가 변경되었다면, 때때로 안 좋은 일이 일어납니다. 그래서 일반적으론
언제나 네이티브 모듈의 소스 코드를 가져와서 빌드하는 것을 권장합니다.

`npm`을 통한 방법을 따르고 있다면, 기본적으로 이 소스를 받아와서 빌드합니다. 만약
그렇지 않다면, `npm`에 `--build-from-source`를 전달해 주어야 합니다. 아니면
`npm_config_build_from_source`를 환경 변수에 설정하는 방법도 있습니다.

[electron-rebuild]: https://github.com/paulcbetts/electron-rebuild
[node-pre-gyp]: https://github.com/mapbox/node-pre-gyp
