# 개발자 도구 확장 기능

애플리케이션의 디버깅을 쉽게 하기 위해 Electron은 기본적으로
[Chrome DevTools Extension][devtools-extension]을 지원합니다.

Electron은 유명한 웹 프레임워크를 디버깅하기 위해 사용할 수 있는 개발자 도구 확장
기능을 사용할 수 있도록 [Chrome 개발자 도구 확장 기능][devtools-extension]을
지원합니다.

## 개발자 도구는 어떻게 로드하나요

이 문서는 확장 기능을 수동으로 로드하는 방법의 과정을 설명합니다.
[electron-devtools-installer](https://github.com/GPMDP/electron-devtools-installer)와
같은 Chrome WebStore에서 자동으로 확장 기능을 다운로드하는 서드-파티 도구를 사용할 수도
있습니다.

Electron에 확장 기능을 로드하려면, Chrome 브라우저에서 다운로드 해야 하며, 파일 시스템 경로를 지정해야 합니다. 그리고 `BrowserWindow.addDevToolsExtension(extension)`를 호출함으로써 기능을 로드할 수 있습니다.

예시로 [React Developer Tools][react-devtools]를 사용한다면:

1. Chrome 브라우저를 설치합니다.
2. `chrome://extensions`로 이동한 후 해시된 `fmkadmapgofadopljbjfkapdkoienihi`
  같이 생긴 확장 기능의 ID를 찾습니다.
3. Chrome에서 사용하는 확장 기능을 저장해둔 파일 시스템 경로를 찾습니다:
   * Windows에선 `%LOCALAPPDATA%\Google\Chrome\User Data\Default\Extensions`;
   * Linux에선:
     * `~/.config/google-chrome/Default/Extensions/`
     * `~/.config/google-chrome-beta/Default/Extensions/`
     * `~/.config/google-chrome-canary/Default/Extensions/`
     * `~/.config/chromium/Default/Extensions/`
   * macOS에선 `~/Library/Application Support/Google/Chrome/Default/Extensions`.
4. 확장 기능의 경로를 `BrowserWindow.addDevToolsExtension` API로 전달합니다.
   React Developer Tools의 경우 다음과 비슷해야 합니다:
   `~/Library/Application Support/Google/Chrome/Default/Extensions/fmkadmapgofadopljbjfkapdkoienihi/0.15.0_0`

**참고:** `BrowserWindow.addDevToolsExtension` API는 `app` 모듈의 `ready` 이벤트가
발생하기 전까지 사용할 수 없습니다.

확장 기능의 이름은 `BrowserWindow.addDevToolsExtension`에서 반환되며, 이 이름을
`BrowserWindow.removeDevToolsExtension` API로 전달함으로써 해당하는 확장 기능을
언로드할 수 있습니다.

## 지원하는 개발자 도구 확장 기능

Electron은 아주 제한적인 `chrome.*` API만을 지원하므로 확장 기능이 지원하지 않는
`chrome.*` API를 사용한다면 해당 기능은 작동하지 않을 것입니다. 다음 개발자 도구들은
Electron에서 정상적으로 작동하는 것을 확인했으며 작동 여부를 보장할 수 있는 확장
기능입니다:

* [Ember Inspector](https://chrome.google.com/webstore/detail/ember-inspector/bmdblncegkenkacieihfhpjfppoconhi)
* [React Developer Tools](https://chrome.google.com/webstore/detail/react-developer-tools/fmkadmapgofadopljbjfkapdkoienihi)
* [Backbone Debugger](https://chrome.google.com/webstore/detail/backbone-debugger/bhljhndlimiafopmmhjlgfpnnchjjbhd)
* [jQuery Debugger](https://chrome.google.com/webstore/detail/jquery-debugger/dbhhnnnpaeobfddmlalhnehgclcmjimi)
* [AngularJS Batarang](https://chrome.google.com/webstore/detail/angularjs-batarang/ighdmehidhipcmcojjgiloacoafjmpfk)
* [Vue.js devtools](https://chrome.google.com/webstore/detail/vuejs-devtools/nhdogjmejiglipccpnnnanhbledajbpd)

### 개발자 도구가 작동하지 않을 때 어떻게 해야 하나요?

먼저 해당 확장 기능이 확실히 계속 유지되고 있는지를 확인하세요. 몇몇 확장 기능들은
최신 버전의 Chrome 브라우저에서도 작동하지 않습니다. 그리고 이러한 확장 기능에 대해선
Electron 개발팀에 해줄 수 있는 것이 아무것도 없습니다.

위와 같은 상황이 아니라면 Electron의 이슈 리스트에 버그 보고를 추가한 후 예상한 것과
달리 확장 기능의 어떤 부분의 정상적으로 작동하지 않았는지 설명하세요.

[devtools-extension]: https://developer.chrome.com/extensions/devtools
[react-devtools]: https://chrome.google.com/webstore/detail/react-developer-tools/fmkadmapgofadopljbjfkapdkoienihi
