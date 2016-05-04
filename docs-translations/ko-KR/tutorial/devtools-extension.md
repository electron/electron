# 개발자 도구 확장 기능

어플리케이션의 디버깅을 쉽게 하기 위해 Electron은 기본적으로
[Chrome DevTools Extension][devtools-extension]을 지원합니다.

개발자 도구 확장 기능은 간단하게 사용할 확장 기능 플러그인의 소스 코드를 다운로드한 후
`BrowserWindow.addDevToolsExtension` API를 통해 어플리케이션 내에 로드할 수 있습니다.
한가지 주의할 점은 확장 기능 사용시 창이 생성될 때 마다 일일이 해당 API를 호출할 필요는
없습니다.

**주의: 현재 React DevTools은 작동하지 않습니다. https://github.com/electron/electron/issues/915 이슈를 참고하세요!**

다음 예시는 [React DevTools Extension](https://github.com/facebook/react-devtools)을
사용합니다.

먼저 소스코드를 다운로드 받습니다:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

[`react-devtools/shells/chrome/Readme.md`](https://github.com/facebook/react-devtools/blob/master/shells/chrome/Readme.md)를
통해 확장 기능을 개발하는 방법을 알아볼 수 있습니다.

그리고 개발자 도구에서 다음 코드를 입력하면 확장 기능을 로드할 수 있습니다:

```javascript
const BrowserWindow = require('electron').remote.BrowserWindow;
BrowserWindow.addDevToolsExtension('/some-directory/react-devtools/shells/chrome');
```

확장 기능을 언로드 하고 콘솔을 다시 열 때 해당 확장 기능이 로드되지 않도록 하려면
`BrowserWindow.removeDevToolsExtension` API를 사용하면 됩니다:

```javascript
BrowserWindow.removeDevToolsExtension('React Developer Tools');
```

## 개발자 도구 확장 기능의 구성 형식

모든 개발자 도구 확장은 완벽히 Chrome 브라우저를 위해 작성되었기 때문에 Electron에서도
로드할 수 있습니다. 하지만 반드시 확장 기능은 소스 코드 디렉터리(폴더) 형태여야 합니다.
그래서 `crx` 등의 포맷으로 패키징된 확장 기능의 경우 사용자가 직접 해당 패키지의 압축을
풀어서 로드하지 않는 이상 Electron에서 해당 확장 기능의 압축을 풀 방법이 없습니다.

## 백그라운드 페이지

현재 Electron은 Chrome에서 지원하는 백그라운드 페이지(background pages)를 지원하지
않습니다. 몇몇 확장 기능은 이 기능에 의존하는 경우가 있는데, 이 때 해당 확장 기능은
Electron에서 작동하지 않을 수 있습니다.

## `chrome.*` API

몇몇 Chrome 확장 기능은 특정 기능을 사용하기 위해 `chrome.*` API를 사용하는데, 이
API들을 구현하기 위해 노력했지만 안타깝게도 아직 모든 API가 구현되지는 않았습니다.

아직 모든 API가 구현되지 않았기 때문에 확장 기능에서 `chrome.devtools.*` 대신
`chrome.*` API를 사용할 경우 확장 기능이 제대로 작동하지 않을 수 있음을 감안해야 합니다.
만약 문제가 발생할 경우 Electron의 GitHub 저장소에 관련 이슈를 올리면 해당 API를
추가하는데 많은 도움이 됩니다.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
