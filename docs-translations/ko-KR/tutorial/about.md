> 이 문서는 아직 Electron 기여자가 번역하지 않았습니다.
>
> Electron에 기여하고 싶다면 [기여 가이드](https://github.com/electron/electron/blob/master/docs-translations/ko-KR/project/CONTRIBUTING.md)를
> 참고하세요.
>
> 문서의 번역이 완료되면 이 틀을 삭제해주세요.

# Electron 에 대하여

[Electron](http://electron.atom.io) 은 HTML, CSS 와 Javascript 로 크로스플랫폼
데스크톱 애플리케이션을 위해 Github 에서 개발한 오픈소스 라이브러리 입니다.
Electron 은 [Chromium](https://www.chromium.org/Home) 와
[Node.js](https://nodejs.org) 를 단일 실행으로 합치고 앱을 Mac, Windows 와
Linux 용으로 패키지화 할 수 있게 함으로써 이것을 가능하게 합니다.

Electron 은 2013년에 Github 의 해킹 가능한 텍스트 편집기 Atom 의 프레임워크로
시작하였습니다. 이 둘은 2014년에 오픈소스화 됩니다.

그후로 오픈소스 개발자, 스타트업과 안정된 회사에서 인기있는 도구가 되었습니다.
[Electron 을 사용하는 곳을 보세요.](/apps).

Electron 의 공헌자와 릴리즈에 대한 자세한 내용이나 개발을 시작하려면
[Quick Start Guide](quick-start.md) 를 읽어보세요.

## 코어 팀과 공헌자

Electron 은 Gihub 의 팀과 커뮤니티에서
[활동중인 공헌자](https://github.com/electron/electron/graphs/contributors)
그룹에 의해 유지됩니다. 일부 공헌자는 개인이고, 일부는 Electron 으로 개발을
하는 큰 회사입니다. 프로젝트에 자주 공여하는 분은 기꺼이 메인테이너로
추가하겠습니다.
[Electron 에 공헌하기](../project/CONTRIBUTING.md)를 참고하세요..

## Releases

[Electron 출시](https://github.com/electron/electron/releases)는 빈번합니다.
중요한 버그 수정, 새 API 추가 또는 Chromium 이나 Node.js 의 업데이트시에
출시합니다.

### Updating Dependencies

Electron's version of Chromium is usually updated within one or two weeks after a new stable Chromium version is released, depending on the effort involved in the upgrade.

When a new version of Node.js is released, Electron usually waits about a month before upgrading in order to bring in a more stable version.

In Electron, Node.js and Chromium share a single V8 instance—usually the version that Chromium is using. Most of the time this _just works_ but sometimes it means patching Node.js.


### Versioning

Due to the hard dependency on Node.js and Chromium, Electron is in a tricky versioning position and [does not follow `semver`](http://semver.org). You should therefore always reference a specific version of Electron. [Read more about Electron's versioning](http://electron.atom.io/docs/tutorial/electron-versioning/) or see the [versions currently in use](https://electron.atom.io/#electron-versions).

### LTS

Long term support of older versions of Electron does not currently exist. If your current version of Electron works for you, you can stay on it for as long as you'd like. If you want to make use of new features as they come in you should upgrade to a newer version.

A major update came with version `v1.0.0`. If you're not yet using this version, you should [read more about the `v1.0.0` changes](http://electron.atom.io/blog/2016/05/11/electron-1-0).

## 중심 철학

In order to keep Electron small (file size) and sustainable (the spread of dependencies and APIs) the project limits the scope of the core project.

For instance, Electron uses just the rendering library from Chromium rather than all of Chromium. This makes it easier to upgrade Chromium but also means some browser features found in Google Chrome do not exist in Electron.

New features added to Electron should primarily be native APIs. If a feature can be its own Node.js module, it probably should be. See the [Electron tools built by the community](http://electron.atom.io/community).

## 이력

다음은 Electron 이력의 주요 시점입니다.

| :calendar: | :tada: |
| --- | --- |
| **2013년 4월**| [Atom Shell 탄생](https://github.com/electron/electron/commit/6ef8875b1e93787fa9759f602e7880f28e8e6b45).|
| **2014년 5월** | [Atom Shell 오픈소스화](http://blog.atom.io/2014/05/06/atom-is-now-open-source.html). |
| **2015년 4월** | [Electron 으로 개](https://github.com/electron/electron/pull/1389). |
| **2016년 5월** | [Electron v1.0.0 출시](http://electron.atom.io/blog/2016/05/11/electron-1-0).|
| **2016년 5월** | [Electron 앱이 Mac App Store 와 호환](http://electron.atom.io/docs/tutorial/mac-app-store-submission-guide).|
| **2016년 8월** | [Windows Store 의 Electron 앱 지원](http://electron.atom.io/docs/tutorial/windows-store-guide).|
