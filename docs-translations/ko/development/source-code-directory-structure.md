# 소스 코드 디렉터리 구조

Electron의 소스 코드는 몇 개의 파트로 분리되어 있습니다. 그리고 Chromium의 분리 규칙(separation conventions)을 주로 따르고 있습니다.

이미 [Chromium의 멀티 프로세스 아키텍쳐](http://dev.chromium.org/developers/design-documents/multi-process-architecture)에 익숙해져 있다면 소스 코드를 이해하기 쉬울 것입니다.

## 소스 코드 구조

```
Electron
├──atom - Electron의 소스코드.
|  ├── app - 시스템 엔트리 코드.
|  ├── browser - 주 윈도우를 포함한 프론트엔드, UI, 그리고 메인 프로세스에 관련된 것들.
|  |   |         랜더러와 웹 페이지 관리 관련 코드.
|  |   ├── lib - 메인 프로세스 초기화 코드의 자바스크립트 파트.
|  |   ├── ui - 크로스 플랫폼에 대응하는 UI 구현.
|  |   |   ├── cocoa - 코코아 특정 소스 코드.
|  |   |   ├── gtk - GTK+ 특정 소스 코드.
|  |   |   └── win - Windows GUI 특정 소스 코드.
|  |   ├── default_app - 어플리케이션이 제공되지 않으면 기본으로 사용되는 페이지.
|  |   ├── api - 메인 프로세스 API의 구현.
|  |   |   └── lib - API 구현의 자바스크립트 파트.
|  |   ├── net - 네트워크 관련 코드.
|  |   ├── mac - Mac 특정 Objective-C 소스 코드.
|  |   └── resources - 아이콘들, 플랫폼 종속성 파일들, 기타 등등.
|  ├── renderer - 랜더러 프로세스에서 작동하는 코드들.
|  |   ├── lib - 랜더러 프로세스 초기화 코드의 자바스크립트 파트.
|  |   └── api - 랜더러 프로세스 API들의 구현.
|  |       └── lib - API 구현의 자바스크립트 파트.
|  └── common - 메인 프로세스와 랜더러 프로세스에서 공유하는 코드.
|      |        유틸리티 함수와 node 메시지 루프를 Chromium의 메시지 루프에 통합시킨 코드도 포함.
|      ├── lib - 공통 자바스크립트 초기화 코드.
|      └── api - 공통 API들의 구현, Electron의 빌트인 모듈 기초 코드들.
|          └── lib - API 구현의 자바스크립트 파트.
├── chromium_src - 복사된 Chromium 소스 코드.
├── docs - 참조 문서.
├── spec - 자동화된 테스트.
├── atom.gyp - Electron의 빌드 규칙.
└── common.gypi - 컴파일러 설정 및 `node` 와 `breakpad` 등의 구성요소를 위한 빌드 규칙.
```

## 그외 디렉터리 구조

* **script** - 개발목적으로 사용되는 빌드, 패키징, 테스트, 기타등을 실행하는 스크립트.
* **tools** - gyp 파일에서 사용되는 헬퍼 스크립트 `script`와는 다르게 유저로부터 직접 실행되지 않는 스크립트들을 이곳에 넣습니다.
* **vendor** - 소스코드의 서드파티 종속성 코드 소스 코드 디렉터리가 겹쳐 혼란을 일으킬 수 있기 때문에
  우리는 `third_party`와 같은 Chromium 소스 코드 디렉터리에서 사용된 폴더 이름을 사용하지 않았습니다.
* **node_modules** - 빌드에 사용되는 node 서드파티 모듈.
* **out** - `ninja`의 임시 출력 디렉터리.
* **dist** - 배포용 바이너리를 빌드할 때 사용하는 `script/create-dist.py` 스크립트로부터 만들어지는 임시 디렉터리.
* **external_binaries** - `gyp` 빌드를 지원하지 않아 따로 다운로드된 서드파티 프레임워크 바이너리들.
