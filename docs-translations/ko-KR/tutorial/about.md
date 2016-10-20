# Electron 에 대하여

[Electron](http://electron.atom.io) 은 HTML, CSS 와 Javascript 로 크로스플랫폼
데스크톱 애플리케이션을 위해 Github 에서 개발한 오픈소스 라이브러리 입니다.
Electron 은 [Chromium](https://www.chromium.org/Home) 와
[Node.js](https://nodejs.org) 를 단일 실행으로 합치고 앱을 Mac, Windows 와
Linux 용으로 패키지화 할 수 있게 함으로써 이것을 가능하게 합니다.

Electron 은 2013년에 Github 의 해킹 가능한 텍스트 편집기 Atom 의 프레임워크로
시작하였습니다. 이 둘은 2014년에 오픈소스화 됩니다.

그후로 오픈소스 개발자, 스타트업과 안정된 회사에서 인기있는 도구가 되었습니다.
[Electron 을 사용하는 곳을 보세요](/apps).

Electron 의 기여자와 릴리즈에 대한 자세한 내용이나 개발을 시작하려면
[Quick Start Guide](quick-start.md) 를 읽어보세요.

## 코어 팀과 기여자

Electron 은 Gihub 의 팀과 커뮤니티에서
[활동중인 기여자](https://github.com/electron/electron/graphs/contributors)
그룹에 의해 유지됩니다. 일부 기여자는 개인이고, 일부는 Electron 으로 개발을
하는 큰 회사입니다. 프로젝트에 자주 공여하는 분은 기꺼이 메인테이너로
추가하겠습니다.
[Electron 에 기여하기](../project/CONTRIBUTING.md)를 참고하세요.

## 출시

Electron 은 빈번하게 [출시](https://github.com/electron/electron/releases)
됩니다. 중요한 버그 수정, 새 API 추가 또는 Chromium 이나 Node.js 의
업데이트시에 출시합니다.

### 업데이트 의존성

Electron 의 Chromium 버전은 보통 새 버전 출시 이후 1~2 주 후에 업데이트 하는데,
이는 업그레이드에 필요한 작업의 양에 따라 달라집니다.

Node.js 의 새버전이 출시되면, Electron 은 더 안정된 버전을 가져오기 위해 약
한달정도 기다립니다.

Electron 에서, Node.js 와 Chromium 은 단일 V8 인스턴스를 공유합니다--보통
Chromium 이 사용하는 버전. 대부분은 동작하지만 가끔 Node.js 를 패치해야 합니다.


### 버전 관리

Node.js 와 Chromium 에 대한 의존성이 강해서, Electron 은 버전관리가 까다롭고
[`semver`을 따르지 않습니다](http://semver.org). 그러므로 항상 Electron 의 특정
버전을 참조해야 합니다. [Electron 의 버전관리]
(http://electron.atom.io/docs/tutorial/electron-versioning/)를 읽거나
[현재 쓰이는 버전](https://electron.atom.io/#electron-versions)을 보세요.

### LTS

현재 Electron 의 구버전에 대한 장기 지원은 없습니다. Electron 현재 버전이 잘
동작한다면, 그것을 원하는 만큼 유지하세요. 새로운 기능들을 사용하려면
새버전으로 업그레이드 해야합니다.

주 버전은 `v1.0.0` 입니다. 아직 이 버전을 사용중이지 않다면,
[v1.0.0 변화에 대해 읽어보세요](http://electron.atom.io/blog/2016/05/11/electron-1-0).

## 중심 철학

Electron 을 작고 (파일 크기) 지속가능하게 (의존성 및 API 의 확산) 할 목적으로
중심 프로젝트의 범위를 제한합니다.

예를 들어, Electron 은 Chromium 전체가 아닌 렌더링 라이브러리만 사용합니다.
이것은 Chromium 업그레이드를 쉽게 하지만 Google Chrome 에서 볼 수 있는 몇몇
브라우저 기능이 Electron 에서는 빠집니다.

Electron 에 추가된 새로운 기능은 주로 네이티브 API 입니다. 기능은 가능한한
Node.js 모듈로 해야합니다. [커뮤니티에 의해 개발된 Electron 도구들]
(http://electron.atom.io/community)을 보세요.

## 이력

다음은 Electron 이력의 주요 시점입니다.

| :calendar: | :tada: |
| --- | --- |
| **2013년 4월**| [Atom Shell 탄생](https://github.com/electron/electron/commit/6ef8875b1e93787fa9759f602e7880f28e8e6b45).|
| **2014년 5월** | [Atom Shell 오픈소스화](http://blog.atom.io/2014/05/06/atom-is-now-open-source.html). |
| **2015년 4월** | [Electron 으로 개명](https://github.com/electron/electron/pull/1389). |
| **2016년 5월** | [Electron v1.0.0 출시](http://electron.atom.io/blog/2016/05/11/electron-1-0).|
| **2016년 5월** | [Electron 앱이 Mac App Store 와 호환](http://electron.atom.io/docs/tutorial/mac-app-store-submission-guide).|
| **2016년 8월** | [Windows Store 의 Electron 앱 지원](http://electron.atom.io/docs/tutorial/windows-store-guide).|
