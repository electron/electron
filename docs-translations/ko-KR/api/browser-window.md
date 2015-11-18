# BrowserWindow

`BrowserWindow` 클래스는 브라우저 창(윈도우 창)을 만드는 역할을 담당합니다.

다음 예제는 윈도우 창을 생성합니다:

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600, show: false });
win.on('closed', function() {
  win = null;
});

win.loadURL('https://github.com');
win.show();
```

또한 [Frameless Window](frameless-window.md) API를 사용하여 창 테두리가 없는 윈도우 창을 생성할 수 있습니다.

## Class: BrowserWindow

`BrowserWindow`는 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)를 상속받은 클래스 입니다.

`BrowserWindow`는 `options`를 통해 네이티브 속성을 포함한 새로운 윈도우 창을 생성합니다.

### `new BrowserWindow(options)`

`options` 객체에서 사용할 수 있는 속성들:

* `width` Integer - 윈도우 창의 가로 너비.
* `height` Integer - 윈도우 창의 세로 높이.
* `x` Integer - 화면을 기준으로 창 좌측을 오프셋 한 위치.
* `y` Integer - 화면을 기준으로 창 상단을 오프셋 한 위치.
* `useContentSize` Boolean - `width`와 `height`를 웹 페이지의 크기로 사용합니다.
  이 속성을 사용하면 웹 페이지의 크기에 윈도우 프레임 크기가 추가되므로 실제 창은 조금 더 커질 수 있습니다.
* `center` Boolean - 윈도우 창을 화면 정 중앙에 위치시킵니다.
* `minWidth` Integer - 윈도우 창의 최소 가로 너비.
* `minHeight` Integer - 윈도우 창의 최소 세로 높이.
* `maxWidth` Integer - 윈도우 창의 최대 가로 너비.
* `maxHeight` Integer - 윈도우 창의 최대 세로 높이.
* `resizable` Boolean - 윈도우 창의 크기를 재조정 할 수 있는지 여부.
* `alwaysOnTop` Boolean - 윈도우 창이 언제나 다른 창들 위에 유지되는지 여부.
* `fullscreen` Boolean - 윈도우 창의 전체화면 활성화 여부.
  `false`로 지정했을 경우 OS X에선 전체화면 버튼이 숨겨지거나 비활성화됩니다.
* `skipTaskbar` Boolean - 작업 표시줄 어플리케이션 아이콘 표시 여부.
* `kiosk` Boolean - Kiosk(키오스크) 모드.
* `title` String - 기본 윈도우 창 제목.
* `icon` [NativeImage](native-image.md) - 윈도우 아이콘, 생략하면 실행 파일의 아이콘이 대신 사용됩니다.
* `show` Boolean - 윈도우가 생성되면 보여줄지 여부.
* `frame` Boolean - `false`로 지정하면 창을 [Frameless Window](frameless-window.md) 형태로 생성합니다.
* `acceptFirstMouse` Boolean - 윈도우가 비활성화 상태일 때 내부 컨텐츠 클릭 시 활성화 되는 동시에 단일 mouse-down 이벤트를 발생시킬지 여부.
* `disableAutoHideCursor` Boolean - 파이핑중 자동으로 커서를 숨길지 여부.
* `autoHideMenuBar` Boolean - `Alt`를 누르지 않는 한 어플리케이션 메뉴바를 숨길지 여부.
* `enableLargerThanScreen` Boolean - 윈도우 창 크기가 화면 크기보다 크게 재조정 될 수 있는지 여부.
* `backgroundColor` String - 16진수로 표현된 윈도우의 배경 색. `#66CD00` 또는 `#FFF`가 사용될 수 있습니다.
  이 속성은 Linux와 Windows에만 구현되어 있습니다.
* `darkTheme` Boolean - 설정에 상관 없이 무조건 어두운 윈도우 테마를 사용합니다. 몇몇 GTK+3 데스크톱 환경에서만 작동합니다.
* `transparent` Boolean - 윈도우 창을 [투명화](frameless-window.md)합니다.
* `type` String - 윈도우 창 종류를 지정합니다.
  사용할 수 있는 창 종류는 `desktop`, `dock`, `toolbar`, `splash`, `notification`와 같습니다.
  이 속성은 Linux에서만 작동합니다.
* `standardWindow` Boolean - OS X의 표준 윈도우를 텍스쳐 윈도우 대신 사용합니다. 기본 값은 `true`입니다.
* `titleBarStyle` String, OS X - 윈도우 타이틀 바 스타일을 지정합니다. 이 속성은 OS X 10.10 Yosemite 이후 버전만 지원합니다.
  다음 3가지 종류의 값을 사용할 수 있습니다:
  * `default` 또는 미지정: 표준 Mac 회색 불투명 스타일을 사용합니다.
  * `hidden`: 타이틀 바를 숨기고 컨텐츠 전체를 윈도우 크기에 맞춥니다.
    타이틀 바는 없어지지만 표준 창 컨트롤 ("신호등 버튼")은 왼쪽 상단에 유지됩니다.
  * `hidden-inset`: `hidden` 타이틀 바 속성과 함께 신호등 버튼이 윈도우 모서리로부터 약간 더 안쪽으로 들어가도록합니다.
* `webPreferences` Object - 웹 페이지 기능을 설정합니다. 사용할 수 있는 속성은 다음과 같습니다:
  * `nodeIntegration` Boolean - node(node.js) 통합 여부. 기본값은 `true`입니다.
  * `preload` String - 스크립트를 지정하면 페이지 내의 다른 스크립트가 작동하기 전에 로드됩니다.
    여기서 지정한 스크립트는 페이지의 node 통합 활성화 여부에 상관없이 언제나 모든 node API에 접근할 수 있습니다.
    그리고 `preload` 스크립트의 경로는 절대 경로로 지정해야 합니다.
  * `partition` String - 페이지에서 사용할 세션을 지정합니다. 만약 `partition`이
    `persist:`로 시작하면 페이지는 지속성 세션을 사용하며 다른 모든 앱 내의 페이지에서 같은 `partition`을 사용할 수 있습니다.
    만약 `persist:` 접두어로 시작하지 않으면 페이지는 인-메모리 세션을 사용합니다. 여러 페이지에서 같은 `partition`을 지정하면
    같은 세션을 공유할 수 있습니다. `partition`을 지정하지 않으면 어플리케이션의 기본 세션이 사용됩니다.
  * `zoomFactor` Number - 페이지의 기본 줌 값을 지정합니다. 예를 들어 `300%`를 표현하려면 `3.0`으로 지정합니다.
  * `javascript` Boolean
  * `webSecurity` Boolean - `false`로 지정하면 same-origin 정책을 비활성화합니다. (이 속성은 보통 사람에 의해 웹 사이트를 테스트할 때 사용합니다)
    그리고 `allowDisplayingInsecureContent`와 `allowRunningInsecureContent`이 사용자로부터 `true`로 지정되지 않은 경우 `true`로 지정합니다.
  * `allowDisplayingInsecureContent` Boolean - https 페이지에서 http URL에서 로드한 이미지 같은 리소스를 표시할 수 있도록 허용합니다.
  * `allowRunningInsecureContent` Boolean - https 페이지에서 http URL에서 로드한 JavaScript와 CSS 또는 플러그인을 실행시킬 수 있도록 허용합니다.
  * `images` Boolean
  * `java` Boolean
  * `textAreasAreResizable` Boolean
  * `webgl` Boolean
  * `webaudio` Boolean
  * `plugins` Boolean - 어떤 플러그인이 활성화되어야 하는지 지정합니다.
  * `experimentalFeatures` Boolean
  * `experimentalCanvasFeatures` Boolean
  * `overlayScrollbars` Boolean
  * `overlayFullscreenVideo` Boolean
  * `sharedWorker` Boolean
  * `directWrite` Boolean - Windows에서 폰트 랜더링을 위해 DirectWrite를 사용하는지를 지정합니다.
  * `pageVisibility` Boolean - 현재 윈도우의 가시성을 반영하는 대신 페이지가 visible 또는 hidden 중 지정된 상태를 계속 유지하도록 합니다.
    이 속성을 `true`로 지정하면 DOM 타이머의 스로틀링을 방지할 수 있습니다.

## Events

`BrowserWindow` 객체는 다음과 같은 이벤트를 발생시킵니다:

**참고:** 몇몇 이벤트는 라벨에 특정한 OS에서만 작동합니다.

### Event: 'page-title-updated'

Returns:

* `event` Event

문서의 제목이 변경될 때 발생하는 이벤트 입니다.
`event.preventDefault()`를 호출하여 네이티브 윈도우의 제목이 변경되는 것을 방지할 수 있습니다.

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

  // 반드시 문자열을 반환해야 하고 사용자에게 페이지 언로드에 대한 확인 창을 보여주는 보통의 브라우저와는 달리
  // Electron은 개발자에게 더 많은 옵션을 제공합니다.
  // 빈 문자열을 반환하거나 false를 반환하면 페이지 언로드를 방지합니다.
  // 또한 dialog API를 통해 사용자에게 어플리케이션을 종료할지에 대한 확인 창을 보여줄 수도 있습니다.
  e.returnValue = false;
};
```

### Event: 'closed'

윈도우 종료가 완료된 경우 발생하는 이벤트입니다.
이 이벤트가 발생했을 경우 반드시 윈도우 창의 레퍼런스가 더 이상 사용되지 않도록 제거해야 합니다.

### Event: 'unresponsive'

웹 페이지가 응답하지 않을 때 발생하는 이벤트입니다.

### Event: 'responsive'

응답하지 않는 웹 페이지가 다시 응답하기 시작했을 때 발생하는 이벤트입니다.

### Event: 'blur'

윈도우가 포커스를 잃었을 떄 발생하는 이벤트입니다.

### Event: 'focus'

윈도우가 포커스를 가졌을 때 발생하는 이벤트입니다.

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

### Event: 'app-command' _WINDOWS_

[App Command](https://msdn.microsoft.com/en-us/library/windows/desktop/ms646275(v=vs.85).aspx)가 호출됐을 때 발생하는 이벤트입니다.
이 이벤트는 일반적으로 키보드 미디어 키 또는 브라우저 커맨드(기본 동작 키)에 관련되어 있습니다.
예를 들어 Windows에서 작동하는 몇몇 마우스는 "뒤로가기" 같은 동작을 포함하고 있습니다.

```javascript
someWindow.on('app-command', function(e, cmd) {
  // 마우스의 뒤로가기 버튼을 눌렀을 때 뒤로가기 탐색을 실행합니다
  if (cmd === 'browser-backward' && someWindow.webContents.canGoBack()) {
    someWindow.webContents.goBack();
  }
});
```

## Methods

`BrowserWindow` 객체는 다음과 같은 메서드를 가지고 있습니다:

### `BrowserWindow.getAllWindows()`

열려있는 모든 브라우저 윈도우의 배열을 반환합니다.

### `BrowserWindow.getFocusedWindow()`

어플리케이션에서 포커스된 윈도우를 반환합니다.

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

윈도우의 `WebContents` 객체입니다. 모든 웹 페이지와 관련된 이벤트와 작업이 이 객체를 통해 수행됩니다.

메서드나 이벤트에 대한 자세한 내용은 [`webContents` 문서](web-contents.md)를 참고하세요.

### `win.id`

윈도우의 유일 ID입니다.

## Instance Methods

`new BrowserWindow`로 생성한 객체는 다음과 같은 메서드들을 가지고 있습니다:

**참고:** 몇몇 메서드들은 라벨에서 특정한 운영체제 시스템에서만 작동합니다.

### `win.destroy()`

윈도우를 강제로 닫습니다. 웹 페이지의 `unload` 와 `beforeunload` 이벤트는 일어나지 않습니다.
또한 이 윈도우의 `close`도 일어나지 않습니다. 하지만 `closed` 이벤트는 반드시 발생함을 보장합니다.

이 메서드는 렌더러 프로세스가 예기치 않게 크래시가 일어났을 경우에만 사용해야 합니다.

### `win.close()`

윈도우의 종료를 시도합니다. 이 메서드는 사용자가 윈도우의 닫기 버튼을 클릭했을 때와 같은 효과를 냅니다.
웹 페이지는 로드가 취소되고 종료됩니다. 자세한 내용은 [close 이벤트](#event-close)를 참고하세요.

### `win.focus()`

윈도우에 포커스를 맞춥니다.

### `win.isFocused()`

윈도우가 포커스 되었는지 여부를 반환합니다.

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

윈도우가 최소화 되었는지 여부를 반환합니다.

### `win.setFullScreen(flag)`

* `flag` Boolean

윈도우의 전체화면 상태를 지정합니다.

### `win.isFullScreen()`

윈도우가 전체화면 모드 상태인지 여부를 반환합니다.

### `win.setAspectRatio(aspectRatio[, extraSize])` _OS X_

* `aspectRatio` The aspect ratio we want to maintain for some portion of the
content view.
* `extraSize` Object (optional) - The extra size not to be included while
maintaining the aspect ratio. Properties:
  * `width` Integer
  * `height` Integer

This will have a window maintain an aspect ratio. The extra size allows a
developer to have space, specified in pixels, not included within the aspect
ratio calculations. This API already takes into account the difference between a
window's size and its content size.

Consider a normal window with an HD video player and associated controls.
Perhaps there are 15 pixels of controls on the left edge, 25 pixels of controls
on the right edge and 50 pixels of controls below the player. In order to
maintain a 16:9 aspect ratio (standard aspect ratio for HD @1920x1080) within
the player itself we would call this function with arguments of 16/9 and
[ 40, 50 ]. The second argument doesn't care where the extra width and height
are within the content view--only that they exist. Just sum any extra width and
height areas you have within the overall content view.

### `win.setBounds(options)`

`options` Object, properties:

* `x` Integer
* `y` Integer
* `width` Integer
* `height` Integer

윈도우를 지정한 `width`, `height`, `x`, `y`로 크기 재조정 및 이동합니다.

### `win.getBounds()`

윈도우의 width, height, x, y 값을 가지는 객체를 반환합니다.

### `win.setSize(width, height)`

* `width` Integer
* `height` Integer

`width`와 `height` 값으로 윈도우 크기를 재조정합니다. (너비, 높이)

### `win.getSize()`

윈도우의 너비, 높이값을 가지는 배열을 반환합니다.

### `win.setContentSize(width, height)`

* `width` Integer
* `height` Integer

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

윈도우의 크기가 사용자에 의해 재조정될 수 있는지를 지정합니다.

### `win.isResizable()`

윈도우의 크기가 사용자에 의해 재조정될 수 있는지 여부를 반환합니다.

### `win.setAlwaysOnTop(flag)`

* `flag` Boolean

윈도우가 언제나 다른 윈도우들 위에 표시되는지 여부를 지정합니다.
이 설정을 활성화 하면 윈도우는 포커스 될 수 없는 툴박스 윈도우가 아닌 일반 윈도우로 유지됩니다.

### `win.isAlwaysOnTop()`

윈도우가 언제나 다른 윈도우들 위에 표시되는지 여부를 반환합니다.

### `win.center()`

윈도우를 화면 정 중앙으로 이동합니다.

### `win.setPosition(x, y)`

* `x` Integer
* `y` Integer

윈도우의 위치를 `x`, `y`로 이동합니다.

### `win.getPosition()`

윈도우의 위치를 배열로 반환합니다.

### `win.setTitle(title)`

* `title` String

`title`을 네이티브 윈도우의 제목으로 지정합니다.

### `win.getTitle()`

윈도우의 제목을 반환합니다.

**참고:** 웹 페이지의 제목과 네이티브 윈도우의 제목은 서로 다를 수 있습니다.

### `win.flashFrame(flag)`

* `flag` Boolean

사용자가 윈도우에 관심을 가질 수 있도록 창을 깜빡이거나 이를 중지합니다.

### `win.setSkipTaskbar(skip)`

* `skip` Boolean

어플리케이션 아이콘을 작업 표시줄에 보이지 않도록 설정합니다.

### `win.setKiosk(flag)`

* `flag` Boolean

Kiosk(키오스크) 모드를 설정합니다.

### `win.isKiosk()`

현재 윈도우가 kiosk 모드인지 여부를 반환합니다.

### `win.hookWindowMessage(message, callback)` _WINDOWS_

* `message` Integer
* `callback` Function

Windows 메시지를 후킹합니다. `callback`은 WndProc에서 메시지를 받았을 때 호출됩니다.

### `win.isWindowMessageHooked(message)` _WINDOWS_

* `message` Integer

Returns `true` or `false` depending on whether the message is hooked.

### `win.unhookWindowMessage(message)` _WINDOWS_

* `message` Integer

Unhook the window message.

### `win.unhookAllWindowMessages()` _WINDOWS_

Unhooks all of the window messages.

### `win.setRepresentedFilename(filename)` _OS X_

* `filename` String

Sets the pathname of the file the window represents, and the icon of the file
will show in window's title bar.

### `win.getRepresentedFilename()` _OS X_

Returns the pathname of the file the window represents.

### `win.setDocumentEdited(edited)` _OS X_

* `edited` Boolean

Specifies whether the window’s document has been edited, and the icon in title
bar will become grey when set to `true`.

### `win.isDocumentEdited()` _OS X_

Whether the window's document has been edited.

### `win.focusOnWebView()`

### `win.blurWebView()`

### `win.capturePage([rect, ]callback)`

* `rect` Object (optional)- The area of page to be captured, properties:
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

Captures a snapshot of the page within `rect`. Upon completion `callback` will
be called with `callback(image)`. The `image` is an instance of
[NativeImage](native-image.md) that stores data of the snapshot. Omitting
`rect` will capture the whole visible page.

### `win.print([options])`

Same as `webContents.print([options])`

### `win.printToPDF(options, callback)`

Same as `webContents.printToPDF(options, callback)`

### `win.loadURL(url[, options])`

Same as `webContents.loadURL(url[, options])`.

### `win.reload()`

Same as `webContents.reload`.

### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

Sets the `menu` as the window's menu bar, setting it to `null` will remove the
menu bar.

### `win.setProgressBar(progress)`

* `progress` Double

Sets progress value in progress bar. Valid range is [0, 1.0].

Remove progress bar when progress < 0;
Change to indeterminate mode when progress > 1.

On Linux platform, only supports Unity desktop environment, you need to specify
the `*.desktop` file name to `desktopName` field in `package.json`. By default,
it will assume `app.getName().desktop`.

### `win.setOverlayIcon(overlay, description)` _Windows 7+_

* `overlay` [NativeImage](native-image.md) - the icon to display on the bottom
right corner of the taskbar icon. If this parameter is `null`, the overlay is
cleared
* `description` String - a description that will be provided to Accessibility
screen readers

Sets a 16px overlay onto the current taskbar icon, usually used to convey some
sort of application status or to passively notify the user.


### `win.setThumbarButtons(buttons)` _Windows 7+_

`buttons` Array of `button` Objects:

`button` Object, properties:

* `icon` [NativeImage](native-image.md) - The icon showing in thumbnail
  toolbar.
* `tooltip` String (optional) - The text of the button's tooltip.
* `flags` Array (optional) - Control specific states and behaviors
  of the button. By default, it uses `enabled`. It can include following
  Strings:
  * `enabled` - The button is active and available to the user.
  * `disabled` - The button is disabled. It is present, but has a visual
    state indicating it will not respond to user action.
  * `dismissonclick` - When the button is clicked, the taskbar button's
    flyout closes immediately.
  * `nobackground` - Do not draw a button border, use only the image.
  * `hidden` - The button is not shown to the user.
  * `noninteractive` - The button is enabled but not interactive; no
    pressed button state is drawn. This value is intended for instances
    where the button is used in a notification.
* `click` - Function

Add a thumbnail toolbar with a specified set of buttons to the thumbnail image
of a window in a taskbar button layout. Returns a `Boolean` object indicates
whether the thumbnail has been added successfully.

The number of buttons in thumbnail toolbar should be no greater than 7 due to
the limited room. Once you setup the thumbnail toolbar, the toolbar cannot be
removed due to the platform's limitation. But you can call the API with an empty
array to clean the buttons.

### `win.showDefinitionForSelection()` _OS X_

Shows pop-up dictionary that searches the selected word on the page.

### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

Sets whether the window menu bar should hide itself automatically. Once set the
menu bar will only show when users press the single `Alt` key.

If the menu bar is already visible, calling `setAutoHideMenuBar(true)` won't
hide it immediately.

### `win.isMenuBarAutoHide()`

Returns whether menu bar automatically hides itself.

### `win.setMenuBarVisibility(visible)`

* `visible` Boolean

Sets whether the menu bar should be visible. If the menu bar is auto-hide, users
can still bring up the menu bar by pressing the single `Alt` key.

### `win.isMenuBarVisible()`

Returns whether the menu bar is visible.

### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

Sets whether the window should be visible on all workspaces.

**참고:** 이 API는 Windows에서 아무 일도 하지 않습니다.

### `win.isVisibleOnAllWorkspaces()`

Returns whether the window is visible on all workspaces.

**참고:** 이 API는 Windows에서 언제나 false를 반환합니다.
