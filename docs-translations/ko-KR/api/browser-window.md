# BrowserWindow

`BrowserWindow` 클래스는 브라우저 창(윈도우)을 만드는 역할을 담당합니다.

다음 예제는 윈도우를 생성합니다:

```javascript
// 메인 프로세스에서
const BrowserWindow = require('electron').BrowserWindow;

// 또는 렌더러 프로세스에서
const BrowserWindow = require('electron').remote.BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600, show: false });
win.on('closed', function() {
  win = null;
});

win.loadURL('https://github.com');
win.show();
```

또한 [Frameless Window](frameless-window.md) API를 사용하여 창 테두리가 없는 윈도우
창을 생성할 수 있습니다.

## Class: BrowserWindow

`BrowserWindow`는 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)를
상속받은 클래스 입니다.

`BrowserWindow`는 `options`를 통해 네이티브 속성을 포함한 새로운 윈도우를 생성합니다.

### `new BrowserWindow([options])`

`options` 객체 (optional), 사용할 수 있는 속성들:

* `width` Integer - 윈도우의 가로 너비. 기본값은 `800`입니다.
* `height` Integer - 윈도우의 세로 높이. 기본값은 `600`입니다.
* `x` Integer - 화면을 기준으로 창 좌측을 오프셋 한 위치. 기본값은 `화면중앙`입니다.
* `y` Integer - 화면을 기준으로 창 상단을 오프셋 한 위치. 기본값은 `화면중앙`입니다.
* `useContentSize` Boolean - `width`와 `height`를 웹 페이지의 크기로 사용합니다.
  이 속성을 사용하면 웹 페이지의 크기에 윈도우 프레임 크기가 추가되므로 실제 창은 조금
  더 커질 수 있습니다. 기본값은 `false`입니다.
* `center` Boolean - 윈도우를 화면 정 중앙에 위치시킵니다.
* `minWidth` Integer - 윈도우의 최소 가로 너비. 기본값은 `0`입니다.
* `minHeight` Integer - 윈도우의 최소 세로 높이. 기본값은 `0`입니다.
* `maxWidth` Integer - 윈도우의 최대 가로 너비. 기본값은 `제한없음`입니다.
* `maxHeight` Integer - 윈도우의 최대 세로 높이. 기본값은 `제한없음`입니다.
* `resizable` Boolean - 윈도우의 크기를 재조정 할 수 있는지 여부. 기본값은 `true`
  입니다.
* `movable` Boolean - 윈도우를 이동시킬 수 있는지 여부. Linux에선 구현되어있지
  않습니다. 기본값은 `true` 입니다.
* `minimizable` Boolean - 윈도우를 최소화시킬 수 있는지 여부. Linux에선 구현되어있지
  않습니다. 기본값은 `true` 입니다.
* `maximizable` Boolean - 윈도우를 최대화시킬 수 있는지 여부. Linux에선 구현되어있지
  않습니다. 기본값은 `true` 입니다.
* `closable` Boolean - 윈도우를 닫을 수 있는지 여부. Linux에선 구현되어있지 않습니다.
  기본값은 `true` 입니다.
* `alwaysOnTop` Boolean - 윈도우이 언제나 다른 창들 위에 유지되는지 여부.
  기본값은 `false`입니다.
* `fullscreen` Boolean - 윈도우의 전체화면 활성화 여부. 이 속성을 명시적으로
  `false`로 지정했을 경우, OS X에선 전체화면 버튼이 숨겨지거나 비활성됩니다. 기본값은
  `false` 입니다.
* `fullscreenable` Boolean - OS X의 최대화/줌 버튼이 전체화면 모드 또는 윈도우
  최대화를 토글할 수 있게 할지 여부입니다. 기본값은 `true` 입니다.
* `skipTaskbar` Boolean - 작업표시줄 어플리케이션 아이콘 표시 스킵 여부. 기본값은
  `false`입니다.
* `kiosk` Boolean - Kiosk(키오스크) 모드. 기본값은 `false`입니다.
* `title` String - 기본 윈도우 제목. 기본값은 `"Electron"`입니다.
* `icon` [NativeImage](native-image.md) - 윈도우 아이콘, 생략하면 실행 파일의
  아이콘이 대신 사용됩니다.
* `show` Boolean - 윈도우가 생성되면 보여줄지 여부. 기본값은 `true`입니다.
* `frame` Boolean - `false`로 지정하면 창을 [Frameless Window](frameless-window.md)
  형태로 생성합니다. 기본값은 `true`입니다.
* `acceptFirstMouse` Boolean - 윈도우가 비활성화 상태일 때 내부 컨텐츠 클릭 시
  활성화 되는 동시에 단일 mouse-down 이벤트를 발생시킬지 여부. 기본값은 `false`입니다.
* `disableAutoHideCursor` Boolean - 타이핑중 자동으로 커서를 숨길지 여부. 기본값은
  `false`입니다.
* `autoHideMenuBar` Boolean - `Alt`를 누르지 않는 한 어플리케이션 메뉴바를 숨길지
  여부. 기본값은 `false`입니다.
* `enableLargerThanScreen` Boolean - 윈도우 크기가 화면 크기보다 크게 재조정 될
  수 있는지 여부. 기본값은 `false`입니다.
* `backgroundColor` String - `#66CD00` 와 `#FFF`, `#80FFFFFF` (알파 지원됨) 같이
  16진수로 표현된 윈도우의 배경 색. 기본값은 `#FFF` (white).
* `hasShadow` Boolean - 윈도우가 그림자를 가질지 여부를 지정합니다. 이 속성은
  OS X에서만 구현되어 있습니다. 기본값은 `true`입니다.
* `darkTheme` Boolean - 설정에 상관 없이 무조건 어두운 윈도우 테마를 사용합니다.
  몇몇 GTK+3 데스크톱 환경에서만 작동합니다. 기본값은 `false`입니다.
* `transparent` Boolean - 윈도우를 [투명화](frameless-window.md)합니다. 기본값은
  `false`입니다.
* `type` String - 특정 플랫폼에만 적용되는 윈도우의 종류를 지정합니다. 기본값은
  일반 윈도우 입니다. 사용할 수 있는 창의 종류는 아래를 참고하세요.
* `standardWindow` Boolean - OS X의 표준 윈도우를 텍스쳐 윈도우 대신 사용합니다.
  기본 값은 `true`입니다.
* `titleBarStyle` String, OS X - 윈도우 타이틀 바 스타일을 지정합니다. 자세한 사항은
  아래를 참고하세요.
* `webPreferences` Object - 웹 페이지 기능을 설정합니다. 사용할 수 있는 속성은
  아래를 참고하세요.

`type` 속성에서 사용할 수 있는 값과 동작은 다음과 같으며, 플랫폼에 따라 다릅니다:

* Linux의 경우, `desktop`, `dock`, `toolbar`, `splash`, `notification` 종류를
  사용할 수 있습니다.
* OS X의 경우, `desktop`, `textured` 종류를 사용할 수 있습니다.
  * `textured`는 창에 메탈 그라디언트 외관(`NSTexturedBackgroundWindowMask`)을
    설정합니다.
  * `desktop`은 데스크탑 배경 레벨(`kCGDesktopWindowLevel - 1`)에 윈도우를
    배치합니다. 참고로 이렇게 만들어진 윈도우는 포커스, 키보드, 마우스 이벤트를 받을
    수 없습니다. 하지만 편법으로 `globalShortcut`을 통해 키 입력을 받을 수 있습니다.

`titleBarStyle` 속성은 OS X 10.10 Yosemite 이후 버전만 지원하며, 다음 3가지 종류의
값을 사용할 수 있습니다:

* `default` 또는 미지정: 표준 Mac 회색 불투명 스타일을 사용합니다.
* `hidden`: 타이틀 바를 숨기고 컨텐츠 전체를 윈도우 크기에 맞춥니다.
  타이틀 바는 없어지지만 표준 창 컨트롤 ("신호등 버튼")은 왼쪽 상단에 유지됩니다.
* `hidden-inset`: `hidden` 타이틀 바 속성과 함께 신호등 버튼이 윈도우 모서리로부터
  약간 더 안쪽으로 들어가도록합니다.

`webPreferences` 속성은 다음과 같은 속성을 가질 수 있습니다:

* `nodeIntegration` Boolean - node(node.js) 통합 여부. 기본값은 `true`입니다.
* `preload` String - 스크립트를 지정하면 페이지 내의 다른 스크립트가 작동하기 전에
  로드됩니다. 여기서 지정한 스크립트는 node 통합 활성화 여부에 상관없이 언제나 모든
  node API에 접근할 수 있습니다. 이 속성의 스크립트 경로는 절대 경로로 지정해야
  합니다. node 통합이 비활성화되어있을 경우, preload 스크립트는 node의 global
  심볼들을 다시 global 스코프로 다시 포함 시킬 수 있습니다.
  [여기](process.md#event-loaded)의 예제를 참고하세요.
* `session` [Session](session.md#class-session) - 페이지에서 사용할 세션을
  지정합니다. Session 객체를 직접적으로 전달하는 대신, 파티션 문자열을 받는
  `partition` 옵션을 사용할 수도 있습니다. `session`과 `partition`이 같이
  제공되었을 경우 `session`이 사용됩니다. 기본값은 기본 세션입니다.
  * `partition` String - 페이지에서 사용할 세션을 지정합니다. 만약 `partition`이
    `persist:`로 시작하면 페이지는 지속성 세션을 사용하며 다른 모든 앱 내의
    페이지에서 같은 `partition`을 사용할 수 있습니다. 만약 `persist:` 접두어로
    시작하지 않으면 페이지는 인-메모리 세션을 사용합니다. 여러 페이지에서 같은
    `partition`을 지정하면 같은 세션을 공유할 수 있습니다. `partition`을 지정하지
    않으면 어플리케이션의 기본 세션이 사용됩니다.
* `zoomFactor` Number - 페이지의 기본 줌 값을 지정합니다. 예를 들어 `300%`를
  표현하려면 `3.0`으로 지정합니다. 기본값은 `1.0`입니다.
* `javascript` Boolean - 자바스크립트를 활성화합니다. 기본값은 `false`입니다.
* `webSecurity` Boolean - `false`로 지정하면 same-origin 정책을 비활성화합니다.
  (이 속성은 보통 사람들에 의해 웹 사이트를 테스트할 때 사용합니다) 그리고
  `allowDisplayingInsecureContent`와 `allowRunningInsecureContent` 두 속성을
  사용자가 `true`로 지정되지 않은 경우 `true`로 지정합니다. 기본값은
  `true`입니다.
* `allowDisplayingInsecureContent` Boolean - https 페이지에서 http URL에서
  로드한 이미지 같은 리소스를 표시할 수 있도록 허용합니다. 기본값은 `false`입니다.
* `allowRunningInsecureContent` Boolean - https 페이지에서 http URL에서 로드한
  JavaScript와 CSS 또는 플러그인을 실행시킬 수 있도록 허용합니다. 기본값은
  `false`입니다.
* `images` Boolean - 이미지 지원을 활성화합니다. 기본값은 `true`입니다.
* `textAreasAreResizable` Boolean - HTML TextArea 요소의 크기를 재조정을
  허용합니다. 기본값은 `true`입니다.
* `webgl` Boolean - WebGL 지원을 활성화합니다. 기본값은 `true`입니다.
* `webaudio` Boolean - WebAudio 지원을 활성화합니다. 기본값은 `true`입니다.
* `plugins` Boolean - 플러그인 활성화 여부를 지정합니다. 기본값은 `false`입니다.
* `experimentalFeatures` Boolean - Chrome의 실험적인 기능을 활성화합니다.
  기본값은 `false`입니다.
* `experimentalCanvasFeatures` Boolean - Chrome의 실험적인 캔버스(canvas) 기능을
  활성화합니다. 기본값은 `false`입니다.
* `directWrite` Boolean - Windows에서 폰트 렌더링을 위해 DirectWrite를
  사용하는지를 지정합니다. 기본값은 `true`입니다.
* `blinkFeatures` String - `CSSVariables,KeyboardEventKey`같은 `,`로 구분된
  기능 문자열들의 리스트입니다. 지원하는 전체 기능 문자열들은
  [setFeatureEnabledFromString][blink-feature-string] 함수에서 찾을 수 있습니다.
* `defaultFontFamily` Object - font-family의 기본 폰트를 지정합니다.
  * `standard` String - 기본값 `Times New Roman`.
  * `serif` String - 기본값 `Times New Roman`.
  * `sansSerif` String - 기본값 `Arial`.
  * `monospace` String - 기본값 `Courier New`.
* `defaultFontSize` Integer - 기본값 `16`.
* `defaultMonospaceFontSize` Integer - 기본값 `13`.
* `minimumFontSize` Integer - 기본값 `0`.
* `defaultEncoding` String - 기본값 `ISO-8859-1`.
* `backgroundThrottling` Boolean - 페이지가 백그라운드 상태에 진입할 때 애니메이션과
  타이머에 스로틀을 적용할지 여부입니다. 기본값은 `true`입니다.

## Events

`BrowserWindow` 객체는 다음과 같은 이벤트를 발생시킵니다:

**참고:** 몇몇 이벤트는 라벨에 특정한 OS에서만 작동합니다.

### Event: 'page-title-updated'

Returns:

* `event` Event

문서의 제목이 변경될 때 발생하는 이벤트 입니다. `event.preventDefault()`를 호출하여
네이티브 윈도우의 제목이 변경되는 것을 방지할 수 있습니다.

### Event: 'close'

Returns:

* `event` Event

윈도우가 닫히기 시작할 때 발생하는 이벤트입니다.
이 이벤트는 DOM의 `beforeunload` 와 `unload` 이벤트가 호출되기 전에 발생합니다.
`event.preventDefault()`를 호출하여 윈도우 종료를 취소할 수 있습니다.

보통 창을 닫아야 할지 결정하기 위해 `beforeunload` 이벤트를 사용하려고 할 것입니다.
이 이벤트는 윈도우 컨텐츠를 새로고칠 때도 발생합니다.
Electron에선 빈 문자열 또는 `false`를 전달할 경우 윈도우 종료를 취소합니다.

예시는 다음과 같습니다:

```javascript
window.onbeforeunload = function(e) {
  console.log('I do not want to be closed');

  // 반드시 문자열을 반환해야 하고 사용자에게 페이지 언로드에 대한 확인 창을 보여주는
  // 보통의 브라우저와는 달리 Electron은 개발자에게 더 많은 옵션을 제공합니다.
  // 빈 문자열을 반환하거나 false를 반환하면 페이지 언로드를 방지합니다.
  // 또한 dialog API를 통해 사용자에게 어플리케이션을 종료할지에 대한 확인 창을
  // 보여줄 수도 있습니다.
  e.returnValue = false;
};
```

### Event: 'closed'

윈도우 종료가 완료된 경우 발생하는 이벤트입니다. 이 이벤트가 발생했을 경우 반드시
윈도우의 레퍼런스가 더 이상 사용되지 않도록 제거해야 합니다.

### Event: 'unresponsive'

웹 페이지가 응답하지 않을 때 발생하는 이벤트입니다.

### Event: 'responsive'

응답하지 않는 웹 페이지가 다시 응답하기 시작했을 때 발생하는 이벤트입니다.

### Event: 'blur'

윈도우가 포커스를 잃었을 떄 발생하는 이벤트입니다.

### Event: 'focus'

윈도우가 포커스를 가졌을 때 발생하는 이벤트입니다.

### Event: 'show'

윈도우가 보여진 상태일 때 발생하는 이벤트입니다.

### Event: 'hide'

윈도우가 숨겨진 상태일 때 발생하는 이벤트입니다.

### Event: 'maximize'

윈도우가 최대화됐을 때 발생하는 이벤트입니다.

### Event: 'unmaximize'

윈도우의 최대화 상태가 해제되었을 때 발생하는 이벤트입니다.

### Event: 'minimize'

윈도우가 최소화됐을 때 발생하는 이벤트입니다.

### Event: 'restore'

윈도우가 최소화 상태에서 복구되었을 때 발생하는 이벤트입니다.

### Event: 'resize'

윈도우의 크기가 재조정될 때 발생하는 이벤트입니다.

### Event: 'move'

윈도우가 새로운 위치로 이동될 때 발생하는 이벤트입니다.

__참고__: OS X에선 이 이벤트가 그저 `moved` 이벤트의 별칭(alias)으로 사용됩니다.

### Event: 'moved' _OS X_

윈도우가 새로운 위치로 이동되었을 때 발생하는 이벤트입니다. (한 번만)

### Event: 'enter-full-screen'

윈도우가 풀 스크린 모드로 진입할 때 발생하는 이벤트입니다.

### Event: 'leave-full-screen'

윈도우가 풀 스크린 모드에서 해제될 때 발생하는 이벤트입니다.

### Event: 'enter-html-full-screen'

윈도우가 HTML API에 의해 풀 스크린 모드로 진입할 때 발생하는 이벤트입니다.

### Event: 'leave-html-full-screen'

윈도우가 HTML API에 의해 풀 스크린 모드에서 해제될 때 발생하는 이벤트입니다.

### Event: 'app-command' _Windows_

Returns:

* `event` Event
* `command` String

[App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)가
호출됐을 때 발생하는 이벤트입니다. 이 이벤트는 일반적으로 키보드 미디어 키 또는
브라우저 커맨드(기본 동작 키)에 관련되어 있습니다. 예를 들어 Windows에서 작동하는
몇몇 마우스는 "뒤로가기" 같은 동작을 포함하고 있습니다.

반환되는 커맨드들은 모두 소문자화되며 언더스코어(`_`)는 하이픈(`-`)으로 변경되며
`APPCOMMAND_` 접두어는 제거됩니다.
e.g. `APPCOMMAND_BROWSER_BACKWARD` 는 `browser-backward`와 같이 반환됩니다.

```javascript
someWindow.on('app-command', function(e, cmd) {
  // 마우스의 뒤로가기 버튼을 눌렀을 때 뒤로가기 탐색을 실행합니다
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack();
  }
});
```

### Event: 'scroll-touch-begin' _OS X_

스크롤 휠 이벤트가 동작하기 시작했을 때 발생하는 이벤트입니다.

### Event: 'scroll-touch-end' _OS X_

스크롤 휠 이벤트가 동작을 멈췄을 때 발생하는 이벤트입니다.

### Event: 'swipe' _OS X_

Returns:

* `event` Event
* `direction` String

3-손가락 스와이프가 작동할 때 발생하는 이벤트입니다. 방향은 `up`, `right`, `down`,
`left`가 될 수 있습니다.

## Methods

`BrowserWindow` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `BrowserWindow.getAllWindows()`

열려있는 모든 브라우저 윈도우의 배열을 반환합니다.

### `BrowserWindow.getFocusedWindow()`

어플리케이션에서 포커스된 윈도우를 반환합니다. 포커스된 윈도우가 없을 경우 `null`을
반환합니다.

### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](web-contents.md)

`webContents`를 소유하고 있는 윈도우를 찾습니다.

### `BrowserWindow.fromId(id)`

* `id` Integer

ID에 해당하는 윈도우를 찾습니다.

### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

`path`에 있는 개발자 도구 확장 기능을 추가합니다. 그리고 확장 기능의 이름을 반환합니다.

확장 기능은 기억됩니다. 따라서 API는 단 한 번만 호출되어야 합니다.
이 API는 실제 프로그램 작성에 사용할 수 없습니다.

### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

`name`에 해당하는 개발자 도구 확장 기능을 제거합니다.

## Instance Properties

`new BrowserWindow`로 생성한 객체는 다음과 같은 속성을 가지고 있습니다:

```javascript
// `win`은 BrowserWindow의 인스턴스입니다
var win = new BrowserWindow({ width: 800, height: 600 });
```

### `win.webContents`

윈도우의 `WebContents` 객체입니다. 모든 웹 페이지와 관련된 이벤트와 작업이 이 객체를
통해 수행됩니다.

메서드나 이벤트에 대한 자세한 내용은 [`webContents` 문서](web-contents.md)를
참고하세요.

### `win.id`

윈도우의 유일 ID입니다.

## Instance Methods

`new BrowserWindow`로 생성한 객체는 다음과 같은 메서드들을 가지고 있습니다:

**참고:** 몇몇 메서드들은 라벨에서 특정한 운영체제 시스템에서만 작동합니다.

### `win.destroy()`

윈도우를 강제로 닫습니다. 웹 페이지의 `unload` 와 `beforeunload` 이벤트는 일어나지
않습니다. 또한 이 윈도우의 `close`도 일어나지 않습니다. 하지만 `closed` 이벤트는
반드시 발생함을 보장합니다.

### `win.close()`

윈도우의 종료를 시도합니다. 이 메서드는 사용자가 윈도우의 닫기 버튼을 클릭했을 때와
같은 효과를 냅니다. 웹 페이지는 로드가 취소되고 종료됩니다. 자세한 내용은
[close 이벤트](#event-close)를 참고하세요.

### `win.focus()`

윈도우에 포커스를 맞춥니다.

### `win.blur()`

윈도우의 포커스를 없앱니다.

### `win.isFocused()`

윈도우가 포커스되었는지 여부를 반환합니다.

### `win.show()`

윈도우를 표시하고 포커스합니다.

### `win.showInactive()`

윈도우를 표시만 하고 포커스하지 않습니다.

### `win.hide()`

윈도우를 숨깁니다.

### `win.isVisible()`

윈도우가 사용자에게 표시되고 있는지 여부를 반환합니다.

### `win.maximize()`

윈도우를 최대화 시킵니다.

### `win.unmaximize()`

윈도우 최대화를 취소합니다.

### `win.isMaximized()`

윈도우가 최대화 되어있는지 여부를 반환합니다.

### `win.minimize()`

윈도우를 최소화 시킵니다. 어떤 플랫폼은 최소화된 윈도우가 Dock에 표시됩니다.

### `win.restore()`

최소화된 윈도우를 이전 상태로 되돌립니다.

### `win.isMinimized()`

윈도우가 최소화되었는지 여부를 반환합니다.

### `win.setFullScreen(flag)`

* `flag` Boolean

윈도우의 전체화면 상태를 지정합니다.

### `win.isFullScreen()`

윈도우가 전체화면 모드 상태인지 여부를 반환합니다.

### `win.setAspectRatio(aspectRatio[, extraSize])` _OS X_

* `aspectRatio` 유지하려 하는 컨텐츠 뷰 일부의 종횡비
* `extraSize` Object (optional) - 종횡비를 유지하는 동안 포함되지 않을 엑스트라 크기.
  * `width` Integer
  * `height` Integer

이 메서드는 윈도우의 종횡비를 유지하는 기능을 수행합니다. 엑스트라 크기는 개발자가
픽셀로 특정한 공간이 있을 때 종횡비 계산에서 제외됩니다. 이 API는 윈도우의 크기와
컨텐츠 사이즈의 차이를 이미 고려하고 있습니다.

일반 윈도우에서 작동하는 HD 비디오 플레이어와 관련된 컨트롤을 고려합니다.
만약 15 픽셀의 컨트롤이 왼쪽 가장자리에 있고 25 픽셀의 컨트롤이 오른쪽 가장자리에
있으며 50 픽셀의 컨트롤이 플레이어 밑에 있을 때 플레이어 자체가 16:9 종횡비(HD의 표준
종횡비는 @1920x1080)를 유지하기 위해선 이 함수를 16/9, [ 40, 50 ] 인수와 함께
호출해야 합니다. 두번째 인수 엑스트라 크기는 존재하는 크기만 관여하고 컨텐츠 뷰 내의
크기는 관여하지 않습니다. 그저 전체 컨텐츠 뷰 내에 있는 모든 엑스트라 너비, 높이 영역이
합해집니다.

### `win.setBounds(options[, animate])`

* `options` Object

  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer

* `animate` Boolean (optional) _OS X_

윈도우를 지정한 `width`, `height`, `x`, `y`로 크기 재조정 및 이동합니다.

### `win.getBounds()`

윈도우의 width, height, x, y 값을 가지는 객체를 반환합니다.

### `win.setSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _OS X_

`width`와 `height` 값으로 윈도우 크기를 재조정합니다. (너비, 높이)

### `win.getSize()`

윈도우의 너비, 높이값을 가지는 배열을 반환합니다.

### `win.setContentSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _OS X_

윈도우 클라이언트 영역(웹 페이지)의 크기를 `width`, `height`로 재조정합니다.

### `win.getContentSize()`

윈도우 클라이언트 영역의 너비, 높이 크기를 배열로 반환합니다.

### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

윈도우의 최소 `width`, `height` 크기를 지정합니다.

### `win.getMinimumSize()`

윈도우의 최소 너비, 높이 크기를 배열로 반환합니다.

### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

윈도우의 최대 `width`, `height` 크기를 지정합니다.

### `win.getMaximumSize()`

윈도우의 최대 너비, 높이 크기를 배열로 반환합니다.

### `win.setResizable(resizable)`

* `resizable` Boolean

사용자에 의해 윈도우의 크기가 재조정될 수 있는지를 지정합니다.

### `win.isResizable()`

사용자에 의해 윈도우의 크기가 재조정될 수 있는지 여부를 반환합니다.

### `win.setMovable(movable)` _OS X_ _Windows_

* `movable` Boolean

사용자에 의해 윈도우를 이동시킬 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

### `win.isMovable()` _OS X_ _Windows_

사용자에 의해 윈도우를 이동시킬 수 있는지 여부를 반환합니다. Linux에선 항상 `true`를
반환합니다.

### `win.setMinimizable(minimizable)` _OS X_ _Windows_

* `minimizable` Boolean

사용자에 의해 윈도우를 최소화시킬 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

### `win.isMinimizable()` _OS X_ _Windows_

사용자에 의해 윈도우를 최소화시킬 수 있는지 여부를 반환합니다. Linux에선 항상 `true`를
반환합니다.

### `win.setMaximizable(maximizable)` _OS X_ _Windows_

* `maximizable` Boolean

사용자에 의해 윈도우를 최대화시킬 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

### `win.isMaximizable()` _OS X_ _Windows_

사용자에 의해 윈도우를 최대화시킬 수 있는지 여부를 반환합니다. Linux에선 항상 `true`를
반환합니다.

### `win.setFullScreenable(fullscreenable)`

* `fullscreenable` Boolean

최대화/줌 버튼이 전체화면 모드 또는 윈도우 최대화를 토글할 수 있게 할지 여부를
지정합니다.

### `win.isFullScreenable()`

최대화/줌 버튼이 전체화면 모드 또는 윈도우 최대화를 토글할 수 있게 할지 여부를
반환합니다.

### `win.setClosable(closable)` _OS X_ _Windows_

* `closable` Boolean

사용자에 의해 윈도우가 수동적으로 닫힐 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

### `win.isClosable()` _OS X_ _Windows_

사용자에 의해 윈도우가 수동적으로 닫힐 수 있는지 여부를 반환합니다. Linux에선 항상
`true`를 반환합니다.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

윈도우가 언제나 다른 윈도우들 위에 표시되는지 여부를 지정합니다. 이 설정을 활성화 하면
윈도우는 포커스 될 수 없는 툴박스 윈도우가 아닌 일반 윈도우로 유지됩니다.

### `win.isAlwaysOnTop()`

윈도우가 언제나 다른 윈도우들 위에 표시되는지 여부를 반환합니다.

### `win.center()`

윈도우를 화면 정 중앙으로 이동합니다.

### `win.setPosition(x, y[, animate])`

* `x` Integer
* `y` Integer
* `animate` Boolean (optional) _OS X_

윈도우의 위치를 `x`, `y`로 이동합니다.

### `win.getPosition()`

윈도우의 위치를 배열로 반환합니다.

### `win.setTitle(title)`

* `title` String

`title`을 네이티브 윈도우의 제목으로 지정합니다.

### `win.getTitle()`

윈도우의 제목을 반환합니다.

**참고:** 웹 페이지의 제목과 네이티브 윈도우의 제목은 서로 다를 수 있습니다.

### `win.setSheetOffset(offset)` _OS X_

Mac OS X에서 시트를 부착할 위치를 지정합니다. 기본적으로 시트는 윈도우의 프레임 바로
아래의 위치에 부착됩니다. 아마도 이 기능은 보통 다음과 같이 HTML 렌더링된 툴바 밑에
표시하기 위해 사용할 것입니다:

```javascript
var toolbarRect = document.getElementById('toolbar').getBoundingClientRect();
win.setSheetOffset(toolbarRect.height);
```

### `win.flashFrame(flag)`

* `flag` Boolean

사용자가 윈도우에 관심을 가질 수 있도록 창을 깜빡이거나 이를 중지합니다.

### `win.setSkipTaskbar(skip)`

* `skip` Boolean

어플리케이션 아이콘을 작업표시줄에 보이지 않도록 설정합니다.

### `win.setKiosk(flag)`

* `flag` Boolean

Kiosk(키오스크) 모드를 설정합니다.

### `win.isKiosk()`

현재 윈도우가 kiosk 모드인지 여부를 반환합니다.

### `win.getNativeWindowHandle()`

`Buffer` 상의 플랫폼에 따른 윈도우 핸들을 반환합니다.

핸들의 타입에 따라 적절히 캐스팅됩니다. Windows의 `HWND`, OS X의 `NSView*`, Linux의
`Window` (`unsigned long`)를 예로 들 수 있습니다.

### `win.hookWindowMessage(message, callback)` _Windows_

* `message` Integer
* `callback` Function

Windows 메시지 훅을 등록합니다. `callback`은 WndProc에서 메시지를 받았을 때
호출됩니다.

### `win.isWindowMessageHooked(message)` _Windows_

* `message` Integer

지정한 메시지가 후킹됬는지 여부를 반환합니다.

### `win.unhookWindowMessage(message)` _Windows_

* `message` Integer

지정한 메시지 훅을 등록 해제합니다.

### `win.unhookAllWindowMessages()` _Windows_

모든 메시지 훅을 등록 해제합니다.

### `win.setRepresentedFilename(filename)` _OS X_

* `filename` String

윈도우 대표 파일의 경로명을 설정합니다. 파일의 아이콘이 윈도우 타이틀 바에 표시됩니다.

### `win.getRepresentedFilename()` _OS X_

윈도우 대표 파일의 경로명을 반환합니다.

### `win.setDocumentEdited(edited)` _OS X_

* `edited` Boolean

윈도우의 문서가 변경되었는지 여부를 설정합니다. 그리고 `true`로 설정했을 때 타이틀 바의
아이콘이 회색으로 표시됩니다.

### `win.isDocumentEdited()` _OS X_

윈도우의 문서가 변경되었는지 여부를 반환합니다.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (optional) - 캡쳐할 페이지의 영역
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

페이지의 스크린샷을 `rect`에 설정한 만큼 캡처합니다. 캡처가 완료되면 `callback`이
`callback(image)` 형식으로 호출됩니다. `image`는 [NativeImage](native-image.md)의
인스턴스이며 스크린샷 데이터를 담고있습니다. `rect`를 생략하면 페이지 전체를 캡처합니다.

### `win.print([options])`

`webContents.print([options])` API와 같습니다.

### `win.printToPDF(options, callback)`

`webContents.printToPDF(options, callback)` API와 같습니다.

### `win.loadURL(url[, options])`

`webContents.loadURL(url[, options])` API와 같습니다.

### `win.reload()`

`webContents.reload` API와 같습니다.

### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

지정한 `menu`를 윈도우의 메뉴로 설정합니다 `null`을 설정하면 메뉴를 제거합니다.

### `win.setProgressBar(progress)`

* `progress` Double

작업표시줄에 표시되고 있는 어플리케이션 아이콘에 진행 상태를 표시합니다. [0, 1.0]
사이의 값을 지정할 수 있습니다.

진행 상태가 < 0 이 되면 진행 상태 표시를 제거합니다.
진행 상태가 > 1 이 되면 불확정 상태 표시로 전환합니다.

Linux 플랫폼에선 Unity 데스크톱 환경만 지원합니다. 그리고 이 기능을 사용하려면
`*.desktop` 파일을 생성한 후 `package.json`의 `desktopName` 필드에 파일 이름을
지정해야 합니다. 기본적으로 `app.getName().desktop`을 통해 접근합니다.

### `win.setOverlayIcon(overlay, description)` _Windows 7+_

* `overlay` [NativeImage](native-image.md) - 작업표시줄 아이콘의 우측 하단에 표시될
아이콘입니다. `null`로 지정하면 빈 오버레이가 사용됩니다
* `description` String - 접근성 설정에 의한 스크린 리더에 제공될 설명입니다

현재 작업표시줄 아이콘에 16 x 16 픽셀 크기의 오버레이를 지정합니다. 보통 이 기능은
어플리케이션의 여러 상태를 사용자에게 소극적으로 알리기 위한 방법으로 사용됩니다.

### `win.setHasShadow(hasShadow)` _OS X_

* `hasShadow` (Boolean)

윈도우가 그림자를 가질지 여부를 지정합니다. Windows와 Linux에선 아무 일도 일어나지
않습니다.

### `win.hasShadow()` _OS X_

윈도우가 그림자를 가지고 있는지 여부를 반환합니다. Windows와 Linux에선 항상 `true`를
반환합니다.

### `win.setThumbarButtons(buttons)` _Windows 7+_

* `buttons` - Array

윈도우 작업표시줄 버튼 레이아웃의 미리보기 이미지 영역에 미리보기 툴바와 버튼 세트를
추가합니다. 반환되는 `Boolean` 값은 미리보기 툴바가 성공적으로 추가됬는지를 알려줍니다.

미리보기 이미지 영역의 제한된 크기로 인해 미리보기 툴바에 추가될 수 있는 최대 버튼의
개수는 7개이며 이 이상 추가될 수 없습니다. 플랫폼의 제약으로 인해 미리보기 툴바는 한 번
설정되면 삭제할 수 없습니다. 하지만 이 API에 빈 배열을 전달하여 버튼들을 제거할 수
있습니다.

`buttons`는 `Button` 객체의 배열입니다:

* `Button` 객체
  * `icon` [NativeImage](native-image.md) - 미리보기 툴바에 보여질 아이콘.
  * `tooltip` String (optional) - 버튼의 툴팁 텍스트.
  * `flags` Array (optional) - 버튼의 특정 동작 및 상태 제어. 기본적으로 `enabled`이
    사용됩니다. 이 속성은 다음 문자열들을 포함할 수 있습니다:
    * `enabled` - 사용자가 사용할 수 있도록 버튼이 활성화 됩니다.
    * `disabled` - 버튼이 비활성화 됩니다. 버튼은 표시되지만 시각적인 상태는 사용자의
      동작에 응답하지 않는 비활성화 상태로 표시됩니다.
    * `dismissonclick` - 버튼이 클릭되면 작업표시줄 버튼의 미리보기(flyout)가 즉시
      종료됩니다.
    * `nobackground` - 버튼의 테두리를 표시하지 않습니다. 이미지에만 사용할 수 있습니다.
    * `hidden` - 버튼을 사용자에게 표시되지 않도록 숨깁니다.
    * `noninteractive` - 버튼은 활성화되어 있지만 반응이 제거되며 버튼을 눌러도
      눌려지지 않은 상태를 유지합니다. 이 값은 버튼을 알림의 용도로 사용하기 위해
      만들어졌습니다.

### `win.showDefinitionForSelection()` _OS X_

페이지의 선택된 단어에 대한 사전 검색 결과 팝업을 표시합니다.

### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

메뉴 막대 자동 숨김 기능을 활성화 합니다.
숨겨진 메뉴는 사용자가 `Alt` 키를 단일 입력했을 때만 표시됩니다.

메뉴 막대가 이미 표시되고 있을 때 `setAutoHideMenuBar(true)`를 호출한다고 해서
메뉴가 즉시 숨겨지지는 않습니다.

### `win.isMenuBarAutoHide()`

메뉴 막대 자동 숨김 상태인지 여부를 반환합니다.

### `win.setMenuBarVisibility(visible)`

* `visible` Boolean

메뉴 막대의 표시 여부를 설정합니다. 만약 메뉴 막대 자동 숨김 상태라면 여전히 사용자가
`Alt` 키를 입력하여 메뉴 막대를 표시되도록 할 수 있습니다.

**역주:** 기본 메뉴 막대를 완전히 없애려면 `win.setMenu(null)`를 호출해야 합니다.
단순히 이 API를 사용하면 여전히 메뉴에 등록된 핫 키가 작동합니다.

### `win.isMenuBarVisible()`

메뉴 막대가 표시되고 있는지 여부를 반환합니다.

### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

윈도우가 모든 워크스페이스에서 표시될지 여부를 설정합니다.

**참고:** 이 API는 Windows에서 아무 일도 하지 않습니다.

### `win.isVisibleOnAllWorkspaces()`

윈도우가 모든 워크스페이스에서 표시될지 여부를 반환합니다.

**참고:** 이 API는 Windows에서 언제나 false를 반환합니다.

### `win.setIgnoreMouseEvents(ignore)` _OS X_

* `ignore` Boolean

윈도우에서 일어나는 모든 마우스 이벤트를 무시합니다.

[blink-feature-string]: https://code.google.com/p/chromium/codesearch#chromium/src/out/Debug/gen/blink/platform/RuntimeEnabledFeatures.cpp&sq=package:chromium&type=cs&l=576
