# 소스 코드 디렉터리 구조

Electron의 소스 코드는 몇 개의 파트로 분리되어 있습니다. 그리고 Chromium의 분리
규칙(separation conventions)을 주로 따르고 있습니다.

이미 [Chromium의 멀티 프로세스 아키텍쳐](http://dev.chromium.org/developers/design-documents/multi-process-architecture)에
익숙해져 있다면 소스 코드를 이해하기 쉬울 것입니다.

## 소스 코드 구조

```
Electron
├── atom - C++ 소스 코드.
|   ├── app - 시스템 엔트리 코드.
|   ├── browser - 주 윈도우를 포함한 프론트엔드, UI, 그리고 메인 프로세스에 관련된
|   |   코드와 렌더러 및 웹 페이지 관리 관련 코드.
|   |   ├── ui - 서로 다른 플랫폼에 대한 UI 관련 구현 코드.
|   |   |   ├── cocoa - Cocoa 특정 소스 코드.
|   |   |   ├── gtk - GTK+ 특정 소스 코드.
|   |   |   └── win - Windows GUI 특정 소스 코드.
|   |   ├── api - 메인 프로세스 API의 구현.
|   |   ├── net - 네트워킹 관련 코드.
|   |   ├── mac - Mac 특정 Objective-C 소스 코드.
|   |   └── resources - 아이콘들, 플랫폼 종속성 파일들, 기타 등등..
|   ├── renderer - 렌더러 프로세스에서 작동하는 코드.
|   |   └── api - 렌더러 프로세스 API의 구현.
|   └── common - 메인과 렌더러 프로세스에서 모두 사용하는 코드, 몇가지 유틸리티
|       함수들이 포함되어 있고 node의 메시지 루프와 Chromium의 메시지 루프를 통합.
|       └── api - 공통 API 구현들, 기초 Electron 빌트-인 모듈들.
├── chromium_src - Chromium에서 복사하여 가져온 소스코드.
├── default_app - Electron에 앱이 제공되지 않았을 때 보여지는 기본 페이지.
├── docs - 참조 문서.
├── lib  - JavaScript 소스 코드.
|   ├── browser - Javascript 메인 프로세스 초기화 코드.
|   |   └── api - Javascript API 구현 코드.
|   ├── common - 메인과 렌더러 프로세스에서 모두 사용하는 JavaScript
|   |   └── api - Javascript API 구현 코드.
|   └── renderer - Javascript 렌더러 프로세스 초기화 코드.
|       └── api - Javascript API 구현 코드.
├── spec - 자동화 테스트.
├── atom.gyp - Electron의 빌드 규칙.
└── common.gypi - 컴파일러 설정 및 `node` 와 `breakpad` 등의 구성요소를 위한 빌드
    규칙.
```

## 그외 디렉터리 구조

* **script** - 개발목적으로 사용되는 빌드, 패키징, 테스트, 기타등을 실행하는 스크립트.
* **tools** - gyp 파일에서 사용되는 헬퍼 스크립트 `script`와는 다르게 유저로부터 직접
  실행되지 않는 스크립트들을 이곳에 넣습니다.
* **vendor** - 소스코드의 서드파티 종속성 코드 소스 코드 디렉터리가 겹쳐 혼란을 일으킬
  수 있기 때문에 `third_party`와 같은 Chromium 소스 코드 디렉터리에서 사용된 폴더
  이름은 사용하지 않았습니다.
* **node_modules** - 빌드에 사용되는 node 서드파티 모듈.
* **out** - `ninja`의 임시 출력 디렉터리.
* **dist** - 배포용 바이너리를 빌드할 때 사용하는 `script/create-dist.py`
  스크립트로부터 만들어지는 임시 디렉터리.
* **external_binaries** - `gyp` 빌드를 지원하지 않아 따로 다운로드된 서드파티
  프레임워크 바이너리들.

## Git 서브 모듈 최신 버전으로 유지

Electron 저장소는 몇 가지 외부 벤더 종속성을 가지고 있으며 [/vendor][vendor]
디렉터리에서 확인할 수 있습니다. 때때로 `git status`를 실행했을 때 아마 다음과 같은
메시지를 흔히 목격할 것입니다:

```sh
$ git status

	modified:   vendor/brightray (new commits)
	modified:   vendor/node (new commits)
```

이 외부 종속성 모듈들을 업데이트 하려면, 다음 커맨드를 실행합니다:

```sh
git submodule update --init --recursive
```

만약 자기 자신이 너무 이 커맨드를 자주 사용하는 것 같다면, `~/.gitconfig` 파일을
생성하여 편하게 업데이트할 수 있습니다:

```
[alias]
	su = submodule update --init --recursive
```

[vendor]: https://github.com/electron/electron/tree/master/vendor
