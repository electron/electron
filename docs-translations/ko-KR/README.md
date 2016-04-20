반드시 사용하는 Electron 버전과 문서 버전을 일치시켜야 합니다. 버전 숫자는 문서 페이지
URL에 포함되어 있습니다. 만약 그렇지 않다면, 아마 현재 보고 있는 문서는 개발 중인
브랜치의 문서를 보고 있을 가능성이 있으며 해당 문서는 추후 API의 변경 가능성이 있고
현재 사용하고 있는 Electron의 버전과 호환되지 않을 수 있습니다. 이 경우 atom.io의
[사용할 수 있는 버전](http://electron.atom.io/docs/) 목록에서 다른 버전으로 변경할
수 있습니다. 또한 GitHub 인터페이스의 "Switch branches/tags" 드롭다운 메뉴에서도
사용 중인 Electron 버전으로 변경할 수 있습니다.

**역주:** 한국어 번역 문서는 `atom.io`에 반영되어 있지 않습니다. 따라서 번역 문서는
GitHub 프로젝트내에서만 볼 수 있으며 `master` 브랜치의 문서는 현재 개발중인 프로젝트의
문서입니다. 한국어 번역 문서는 현재 `upstream` 원본 문서의 변경에 따라 최대한 문서의
버전을 맞추려고 노력하고 있지만 가끔 누락된 번역이 존재할 수 있습니다.

## FAQ

Electron에 대해 자주 묻는 질문이 있습니다. 이슈를 생성하기 전에 다음 문서를 먼저
확인해 보세요:

* [Electron FAQ](faq/electron-faq.md)

## 개발 가이드

* [지원하는 플랫폼](tutorial/supported-platforms.md)
* [어플리케이션 배포](tutorial/application-distribution.md)
* [Mac 앱스토어 어플리케이션 제출 가이드](tutorial/mac-app-store-submission-guide.md)
* [어플리케이션 패키징](tutorial/application-packaging.md)
* [네이티브 Node 모듈 사용하기](tutorial/using-native-node-modules.md)
* [메인 프로세스 디버깅하기](tutorial/debugging-main-process.md)
* [Selenium 과 WebDriver 사용하기](tutorial/using-selenium-and-webdriver.md)
* [개발자 도구 확장 기능](tutorial/devtools-extension.md)
* [Pepper 플래시 플러그인 사용하기](tutorial/using-pepper-flash-plugin.md)
* [Widevine CDM 플러그인 사용하기](tutorial/using-widevine-cdm-plugin.md)
* [Headless CI 시스템에서 테스팅하기 (Travis, Jenkins)](tutorial/testing-on-headless-ci.md)

## 튜토리얼

* [시작하기](tutorial/quick-start.md)
* [데스크톱 환경 통합](tutorial/desktop-environment-integration.md)
* [온라인/오프라인 이벤트 감지](tutorial/online-offline-events.md)

## API 레퍼런스

* [개요](api/synopsis.md)
* [Process 객체](api/process.md)
* [크롬 명령줄 스위치 지원](api/chrome-command-line-switches.md)
* [환경 변수](api/environment-variables.md)

### 커스텀 DOM elements:

* [`File` 객체](api/file-object.md)
* [`<webview>` 태그](api/web-view-tag.md)
* [`window.open` 함수](api/window-open.md)

### 메인 프로세스에서 사용할 수 있는 모듈:

* [app](api/app.md)
* [autoUpdater](api/auto-updater.md)
* [BrowserWindow](api/browser-window.md)
* [contentTracing](api/content-tracing.md)
* [dialog](api/dialog.md)
* [globalShortcut](api/global-shortcut.md)
* [ipcMain](api/ipc-main.md)
* [Menu](api/menu.md)
* [MenuItem](api/menu-item.md)
* [powerMonitor](api/power-monitor.md)
* [powerSaveBlocker](api/power-save-blocker.md)
* [protocol](api/protocol.md)
* [session](api/session.md)
* [webContents](api/web-contents.md)
* [Tray](api/tray.md)

### 렌더러 프로세스에서 사용할 수 있는 모듈 (웹 페이지):

* [desktopCapturer](api/desktop-capturer.md)
* [ipcRenderer](api/ipc-renderer.md)
* [remote](api/remote.md)
* [webFrame](api/web-frame.md)

### 두 프로세스 모두 사용할 수 있는 모듈:

* [clipboard](api/clipboard.md)
* [crashReporter](api/crash-reporter.md)
* [nativeImage](api/native-image.md)
* [screen](api/screen.md)
* [shell](api/shell.md)

## 개발자용

* [코딩 스타일](development/coding-style.md)
* [소스 코드 디렉터리 구조](development/source-code-directory-structure.md)
* [NW.js(node-webkit)와 기술적으로 다른점](development/atom-shell-vs-node-webkit.md)
* [빌드 시스템 개요](development/build-system-overview.md)
* [빌드 설명서 (OS X)](development/build-instructions-osx.md)
* [빌드 설명서 (Windows)](development/build-instructions-windows.md)
* [빌드 설명서 (Linux)](development/build-instructions-linux.md)
* [디버그 설명서 (Windows)](development/debug-instructions-windows.md)
* [디버거 심볼 서버 설정](development/setting-up-symbol-server.md)
