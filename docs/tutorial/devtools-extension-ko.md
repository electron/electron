# 개발자 콘솔 확장

어플리케이션의 디버깅을 쉽게 하기 위해, Electron은 기본적으로 [Chrome DevTools Extension][devtools-extension]을 지원합니다.

개발자 콘솔 확장기능은 간단하게 사용할 확장기능 플러그인의 소스코드를 다운로드한 후 `BrowserWindow.addDevToolsExtension` API를 이용하여
어플리케이션 내에 로드할 수 있습니다. 한가지 주의할 점은 확장기능 사용시 창이 생성될 때 마다 일일이 해당 API를 호출할 필요는 없습니다.

예시로 [React DevTools Extension](https://github.com/facebook/react-devtools)을 사용하자면, 먼저 소스코드를 다운로드 받아야합니다:

```bash
$ cd /some-directory
$ git clone --recursive https://github.com/facebook/react-devtools.git
```

그리고 개발자 콘솔이 열린 창에서 다음의 코드를 콘솔에 입력하면 확장기능을 로드할 수 있습니다:

```javascript
require('remote').require('browser-window').addDevToolsExtension('/some-directory/react-devtools');
```

확장기능을 unload 하려면, `BrowserWindow.removeDevToolsExtension` API를 사용하여 다음에 콘솔을 다시 열 때 해당 확장기능이 로드되지 않도록 할 수 있습니다:

```javascript
require('remote').require('browser-window').removeDevToolsExtension('React Developer Tools');
```

## 개발자 콘솔 확장기능의 구성 형식

완벽하게 모든 개발자 콘솔 확장은 Chrome 브라우저를 위해 작성되었기 때문에 Electron에서도 로드할 수 있습니다.
하지만 반드시 확장기능은 소스코드 그대로의 디렉터리(폴더) 형태여야 합니다. 그래서 `crx` 등의 포맷으로 패키징된 확장기능의 경우
사용자가 직접 해당 패키지의 압축을 풀어서 로드하지 않는 이상은 Electron에서 해당 확장기능의 압축을 풀 방법이 없습니다.

## 백그라운드 페이지

현재 Electron은 Chrome에서 지원하는 백그라운드 페이지(background pages)를 지원하지 않습니다.
몇몇 확장기능은 이 기능에 의존하는 경우가 있는데, 이 경우 해당 확장기능은 Electron에서 작동하지 않을 수 있습니다.

## `chrome.*` API

몇몇 Chrome 확장기능은 특정 기능을 사용하기 위해 `chrome.*` API를 사용하는데, Electron에선 이 API들을 구현하기 위해 노력했지만,
안타깝게도 아직 모든 API가 구현되지는 않았습니다.

아직 모든 API가 구현되지 않았기 때문에 확장기능에서 `chrome.devtools.*` 대신 `chrome.*` API를 사용할 경우 확장기능이 제대로 작동하지 않을 수 있음을 감안해야 합니다.
만약 문제가 발생할 경우 Electron의 GitHub repo에 이슈를 올리면 해당 API를 추가하는데 많은 도움이 됩니다.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
