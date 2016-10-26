# BrowserWindow

> 브라우저 윈도우를 생성하고 제어합니다.

```javascript
// 메인 프로세스에서
const {BrowserWindow} = require('electron')

// 또는 렌더러 프로세스에서
// const {BrowserWindow} = require('electron').remote

let win = new BrowserWindow({width: 800, height: 600})
win.on('closed', () => {
  win = null
})

// 원격 URL 로드
win.loadURL('https://github.com')

// 또는 로컬 HTML 로드
win.loadURL(`file://${__dirname}/app/index.html`)
```

## Frameless 윈도우

Frameless 윈도우를 만들거나 일정한 모양의 투명한 윈도우를 만드려면,
[Frameless 윈도우](frameless-window.md) API를 사용할 수 있습니다.

## 우아하게 윈도우 표시하기

윈도우에서 페이지를 로딩할 때, 사용자는 페이지가 로드되는 모습을 볼 것입니다.
네이티브 어플리케이션으로써 좋지 않은 경험입니다. 윈도우가 시각적인 깜빡임 없이
표시되도록 만드려면, 서로 다른 상황을 위해 두 가지 방법이 있습니다.

### `ready-to-show` 이벤트 사용하기

페이지가 로딩되는 동안, `ready-to-show` 이벤트가 랜더러 프로세스가 랜더링이 완료되었을
때 처음으로 발생합니다. 이 이벤트 이후로 윈도우를 표시하면 시각적인 깜빡임 없이 표시할
수 있습니다.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({show: false})
win.once('ready-to-show', () => {
  win.show()
})
```

이 이벤트는 보통 `did-finish-load` 이벤트 이후에 발생하지만, 페이지가 너무 많은 외부
리소스를 가지고 있다면, `did-finish-load` 이벤트가 발생하기 이전에 발생할 수도
있습니다.

### `backgroundColor` 설정하기

복잡한 어플리케이션에선, `ready-to-show` 이벤트가 너무 늦게 발생할 수 있습니다.
이는 사용자가 어플리케이션이 느리다고 생각할 수 있습니다. 이러한 경우 어플리케이션
윈도우를 바로 보이도록 하고 어플리케이션의 배경과 가까운 배경색을 `backgroundColor`을
통해 설정합니다:

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({backgroundColor: '#2e2c29'})
win.loadURL('https://github.com')
```

참고로 `ready-to-show` 이벤트를 사용하더라도 어플리케이션을 네이티브 느낌이 나도록
하기 위해 `backgroundColor`도 같이 설정하는 것을 권장합니다.

## 부모와 자식 윈도우

`parent` 옵션을 사용하면 자식 윈도우를 만들 수 있습니다:

```javascript
const {BrowserWindow} = require('electron')

let top = new BrowserWindow()
let child = new BrowserWindow({parent: top})
child.show()
top.show()
```

`child` 윈도우는 언제나 `top` 윈도우의 상위에 표시됩니다.

### 모달 윈도우

모달 윈도우는 부모 윈도우를 비활성화 시키는 자식 윈도우입니다. 모달 윈도우를 만드려면
`parent`, `modal` 옵션을 동시에 설정해야 합니다:

```javascript
const {BrowserWindow} = require('electron')

let child = new BrowserWindow({parent: top, modal: true, show: false})
child.loadURL('https://github.com')
child.once('ready-to-show', () => {
  child.show()
})
```

### 플랫폼별 특이사항

* macOS 에서 부모창이 이동할 때 자식창은 부모창과의 상대적 위치를 유지합니다. 윈도우즈와
  리눅스는 자식창이 움직이지 않습니다.
* 윈도우즈에서 parent 를 동적으로 변경할 수 없습니다.
* 리눅스에서 모달창의 타입이 `dialog`로 변경됩니다.
* 리눅스에서 많은 데스크톱 환경이 모달창 숨김을 지원하지 않습니다.

## Class: BrowserWindow

`BrowserWindow`는 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)를
상속받은 클래스 입니다.

`BrowserWindow`는 `options`를 통해 네이티브 속성을 포함한 새로운 윈도우를 생성합니다.

### `new BrowserWindow([options])`

`options` 객체 (optional), 사용할 수 있는 속성들:

* `width` Integer - 윈도우의 가로 너비. 기본값은 `800`입니다.
* `height` Integer - 윈도우의 세로 높이. 기본값은 `600`입니다.
* `x` Integer (**required** y가 사용되면) - 화면을 기준으로 창 좌측을 오프셋 한
  위치. 기본값은 `화면중앙`입니다.
* `y` Integer (**required** x가 사용되면) - 화면을 기준으로 창 상단을 오프셋 한
  위치. 기본값은 `화면중앙`입니다.
* `useContentSize` Boolean - `width`와 `height`를 웹 페이지의 크기로 사용합니다.
  이 속성을 사용하면 웹 페이지의 크기에 윈도우 프레임 크기가 추가되므로 실제
  창은 조금 더 커질 수 있습니다. 기본값은 `false`입니다.
* `center` Boolean - 윈도우를 화면 정 중앙에 위치시킵니다.
* `minWidth` Integer - 윈도우의 최소 가로 너비. 기본값은 `0`입니다.
* `minHeight` Integer - 윈도우의 최소 세로 높이. 기본값은 `0`입니다.
* `maxWidth` Integer - 윈도우의 최대 가로 너비. 기본값은 `제한없음`입니다.
* `maxHeight` Integer - 윈도우의 최대 세로 높이. 기본값은 `제한없음`입니다.
* `resizable` Boolean - 윈도우의 크기를 재조정 할 수 있는지 여부. 기본값은
  `true` 입니다.
* `movable` Boolean - 윈도우를 이동시킬 수 있는지 여부. Linux에선 구현되어있지
  않습니다. 기본값은 `true` 입니다.
* `minimizable` Boolean - 윈도우를 최소화시킬 수 있는지 여부. Linux에선
  구현되어있지 않습니다. 기본값은 `true` 입니다.
* `maximizable` Boolean - 윈도우를 최대화시킬 수 있는지 여부. Linux에선
  구현되어있지 않습니다. 기본값은 `true` 입니다.
* `closable` Boolean - 윈도우를 닫을 수 있는지 여부. Linux에선 구현되어있지
  않습니다. 기본값은 `true` 입니다.
* `focusable` Boolean - 윈도우가 포커스될 수 있는지 여부입니다. 기본값은
  `true`입니다. Windows에선 `focusable: false`를 설정함으로써 암시적으로
  `skipTaskbar: true`도 설정됩니다. Linux에선 `focusable: false`를 설정함으로써
  윈도우가 wm과 함께 반응을 중지하며 모든 작업 영역에서 윈도우가 언제나 최상단에
  있게 됩니다.
* `alwaysOnTop` Boolean - 윈도우이 언제나 다른 창들 위에 유지되는지 여부.
  기본값은 `false`입니다.
* `fullscreen` Boolean - 윈도우의 전체화면 활성화 여부. 이 속성을 명시적으로
  `false`로 지정했을 경우, macOS에선 전체화면 버튼이 숨겨지거나 비활성됩니다.
  기본값은 `false` 입니다.
* `fullscreenable` Boolean - 윈도우가 전체화면 모드로 전환될 수 있는지
  여부입니다. 또한 macOS에선, 최대화/줌 버튼이 전체화면 모드 또는 윈도우
  최대화를 실행할지 여부도 포함됩니다. 기본값은 `true`입니다.
* `skipTaskbar` Boolean - 작업표시줄 애플리케이션 아이콘 표시 스킵 여부.
  기본값은 `false`입니다.
* `kiosk` Boolean - Kiosk(키오스크) 모드. 기본값은 `false`입니다.
* `title` String - 기본 윈도우 제목. 기본값은 `"Electron"`입니다.
* `icon` [NativeImage](native-image.md) - 윈도우 아이콘, 생략하면 실행 파일의
  아이콘이 대신 사용됩니다.
* `icon` [NativeImage](native-image.md) - 윈도우 아이콘. Windows에선 가장 좋은
  시각적 효과를 얻기 위해 `ICO`를 사용하는 것을 권장하며, 또한 undefined로
  남겨두면 실행 파일의 아이콘이 대신 사용됩니다.
On Windows it is
  recommended to use `ICO` icons to get best visual effects, you can also
  leave it undefined so the executable's icon will be used.
* `show` Boolean - 윈도우가 생성되면 보여줄지 여부. 기본값은 `true`입니다.
* `frame` Boolean - `false`로 지정하면 창을 [Frameless Window](frameless-window.md)
  형태로 생성합니다. 기본값은 `true`입니다.
* `parent` BrowserWindow - 부모 윈도우를 설정합니다. 기본 값은 `null`입니다.
* `modal` Boolean - 이 윈도우가 모달 윈도우인지 여부를 설정합니다. 이 옵션은
  자식 윈도우에서만 작동합니다. 기본값은 `false`입니다.
* `acceptFirstMouse` Boolean - 윈도우가 비활성화 상태일 때 내부 콘텐츠 클릭 시
  활성화 되는 동시에 단일 mouse-down 이벤트를 발생시킬지 여부. 기본값은 `false`
  입니다.
* `disableAutoHideCursor` Boolean - 타이핑중 자동으로 커서를 숨길지 여부.
  기본값은 `false`입니다.
* `autoHideMenuBar` Boolean - `Alt`를 누르지 않는 한 애플리케이션 메뉴바를
  숨길지 여부. 기본값은 `false`입니다.
* `enableLargerThanScreen` Boolean - 윈도우 크기가 화면 크기보다 크게 재조정 될
  수 있는지 여부. 기본값은 `false`입니다.
* `backgroundColor` String - `#66CD00` 와 `#FFF`, `#80FFFFFF` (알파 지원됨) 같이
  16진수로 표현된 윈도우의 배경 색. 기본값은 `#FFF` (white).
* `hasShadow` Boolean - 윈도우가 그림자를 가질지 여부를 지정합니다. 이 속성은
  macOS에서만 구현되어 있습니다. 기본값은 `true`입니다.
* `darkTheme` Boolean - 설정에 상관 없이 무조건 어두운 윈도우 테마를 사용합니다.
  몇몇 GTK+3 데스크톱 환경에서만 작동합니다. 기본값은 `false`입니다.
* `transparent` Boolean - 윈도우를 [투명화](frameless-window.md)합니다. 기본값은
  `false`입니다.
* `type` String - 특정 플랫폼에만 적용되는 윈도우의 종류를 지정합니다. 기본값은
  일반 윈도우 입니다. 사용할 수 있는 창의 종류는 아래를 참고하세요.
* `standardWindow` Boolean - macOS의 표준 윈도우를 텍스쳐 윈도우 대신
  사용합니다. 기본 값은 `true`입니다.
* `titleBarStyle` String, macOS - 윈도우 타이틀 바 스타일을 지정합니다. 기본값은
  `default` 입니다. 가능한 값은 다음과 같습니다:
  * `default` - 표준 Mac 회색 불투명 스타일을 사용합니다.
  * `hidden` - 타이틀 바를 숨기고 콘텐츠 전체를 윈도우 크기에 맞춥니다.
    타이틀 바는 없어지지만 표준 창 컨트롤 ("신호등 버튼")은 왼쪽 상단에
    유지됩니다.
  * `hidden-inset` - `hidden` 타이틀 바 속성과 함께 신호등 버튼이 윈도우
    모서리로부터 약간 더 안쪽으로 들어가도록합니다. 10.9 Mavericks에선 지원되지
    않고 `hidden`으로 폴백합니다.
* `thickFrame` Boolean - Windows에서 테두리 없는 윈도우를 위해 표준 윈도우
  프레임을 추가하는 `WS_THICKFRAME` 스타일을 사용합니다. `false`로 지정하면
  윈도우의 그림자와 애니메이션을 삭제합니다. 기본값은 `true`입니다.
* `webPreferences` Object - 웹 페이지 기능을 설정합니다.
  * `nodeIntegration` Boolean - node(node.js) 통합 여부. 기본값은 `true`입니다.
  * `preload` String - 스크립트를 지정하면 페이지 내의 다른 스크립트가 작동하기
    전에 로드됩니다. 여기서 지정한 스크립트는 node 통합 활성화 여부에 상관없이
    언제나 모든 node API에 접근할 수 있습니다. 이 속성의 스크립트 경로는 절대
    경로로 지정해야 합니다. node 통합이 비활성화되어있을 경우, 미리 로드되는
    스크립트는 node의 전역 심볼들을 다시 전역 범위로 다시 포함 시킬 수 있습니다.
    [여기](process.md#event-loaded)의 예시를 참고하세요.
  * `session` [Session](session.md#class-session) - 페이지에서 사용할 세션을
    지정합니다. Session 객체를 직접적으로 전달하는 대신, 파티션 문자열을 받는
    `partition` 옵션을 사용할 수도 있습니다. `session`과 `partition`이 같이
    제공되었을 경우 `session`이 사용됩니다. 기본값은 기본 세션입니다.
    * `partition` String - 페이지에서 사용할 세션을 지정합니다. 만약 `partition`
      이 `persist:`로 시작하면 페이지는 지속성 세션을 사용하며 다른 모든 앱 내의
      페이지에서 같은 `partition`을 사용할 수 있습니다. 만약 `persist:` 접두어로
      시작하지 않으면 페이지는 인-메모리 세션을 사용합니다. 여러 페이지에서 같은
      `partition`을 지정하면 같은 세션을 공유할 수 있습니다. `partition`을
      지정하지 않으면 애플리케이션의 기본 세션이 사용됩니다.
  * `zoomFactor` Number - 페이지의 기본 줌 값을 지정합니다. 예를 들어 `300%`를
    표현하려면 `3.0`으로 지정합니다. 기본값은 `1.0`입니다.
  * `javascript` Boolean - 자바스크립트를 활성화합니다. 기본값은 `false`입니다.
  * `webSecurity` Boolean - `false`로 지정하면 same-origin 정책을 비활성화
    합니다. (이 속성은 보통 사람들에 의해 웹 사이트를 테스트할 때 사용합니다)
    그리고 `allowDisplayingInsecureContent`와 `allowRunningInsecureContent` 두
    속성을 사용자가 `true`로 지정되지 않은 경우 `true`로 지정합니다. 기본값은
    `true`입니다.
  * `allowDisplayingInsecureContent` Boolean - https 페이지에서 http URL에서
    로드한 이미지 같은 리소스를 표시할 수 있도록 허용합니다. 기본값은 `false`
    입니다.
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
  * `experimentalCanvasFeatures` Boolean - Chrome의 실험적인 캔버스(canvas)
    기능을 활성화합니다. 기본값은 `false`입니다.
  * `directWrite` Boolean - Windows에서 폰트 렌더링을 위해 DirectWrite를
    사용하는지를 지정합니다. 기본값은 `true`입니다.
  * `scrollBounce` Boolean - macOS에서 스크롤 튕기기 효과 (탄성 밴딩)를 활성화
    합니다. 기본값은 `false`입니다.
  * `blinkFeatures` String - 활성화 할 `CSSVariables,KeyboardEventKey`같이 `,`로
    구분된 기능 문자열들의 리스트입니다. [RuntimeEnabledFeatures.in][blink-feature-string]
    파일에서 찾을 수 있습니다.
  * `disableBlinkFeatures` String - 비활성화 할 `CSSVariables,KeyboardEventKey`같이
    `,`로 구분된 기능 문자열들의 리스트입니다. [RuntimeEnabledFeatures.in][blink-feature-string]
    파일에서 찾을 수 있습니다.
  * `defaultFontFamily` Object - font-family의 기본 폰트를 지정합니다.
    * `standard` String - 기본값 `Times New Roman`.
    * `serif` String - 기본값 `Times New Roman`.
    * `sansSerif` String - 기본값 `Arial`.
    * `monospace` String - 기본값 `Courier New`.
  * `defaultFontSize` Integer - 기본값 `16`.
  * `defaultMonospaceFontSize` Integer - 기본값 `13`.
  * `minimumFontSize` Integer - 기본값 `0`.
  * `defaultEncoding` String - 기본값 `ISO-8859-1`.
  * `backgroundThrottling` Boolean - 페이지가 백그라운드 상태에 진입할 때
    애니메이션과 타이머에 스로틀을 적용할지 여부입니다. 기본값은 `true`입니다.
  * `offscreen` Boolean - 브라우저 윈도우에 오프 스크린 랜더링을 적용할지 여부를
    지정합니다. 기본값은 `false`입니다.
  * `sandbox` Boolean - Chromium 운영체제 수준의 샌드박스 활성화 여부.

`minWidth`/`maxWidth`/`minHeight`/`maxHeight`를 통해 최소 또는 최대 윈도우
크기를 지정한 경우, 이는 사용자만을 제약하며, `setBounds`/`setSize` 또는
`BrowserWindow`의 생성자에서 크기 제약을 따르지 않는 윈도우 크기를 전달하는 것은
막을 수 없습니다.

`type` 속성에서 사용할 수 있는 값과 동작은 다음과 같으며, 플랫폼에 따라 다릅니다:

* Linux의 경우, `desktop`, `dock`, `toolbar`, `splash`, `notification` 종류를
  사용할 수 있습니다.
* macOS의 경우, `desktop`, `textured` 종류를 사용할 수 있습니다.
  * `textured`는 창에 메탈 그라디언트 외관(`NSTexturedBackgroundWindowMask`)을
    설정합니다.
  * `desktop`은 데스크탑 배경 레벨(`kCGDesktopWindowLevel - 1`)에 윈도우를
    배치합니다. 참고로 이렇게 만들어진 윈도우는 포커스, 키보드, 마우스 이벤트를
    받을 수 없습니다. 하지만 편법으로 `globalShortcut`을 통해 키 입력을 받을 수
    있습니다.
* Windows의 경우, 가능한 타입으론 `toolbar`가 있습니다.

### Instance Events

`new BrowserWindow`로 생성된 객체는 다음과 같은 이벤트를 발생시킵니다:

**참고:** 몇몇 이벤트는 라벨에 특정한 OS에서만 작동합니다.

#### Event: 'page-title-updated'

Returns:

* `event` Event
* `title` String

문서의 제목이 변경될 때 발생하는 이벤트입니다. `event.preventDefault()`를 호출하여
네이티브 윈도우의 제목이 변경되는 것을 방지할 수 있습니다.

#### Event: 'close'

Returns:

* `event` Event
* `title` String

윈도우가 닫히기 시작할 때 발생하는 이벤트입니다.
이 이벤트는 DOM의 `beforeunload` 와 `unload` 이벤트 전에 발생합니다.
`event.preventDefault()`를 호출하여 윈도우 종료를 취소할 수 있습니다.

보통 창을 닫아야 할지 결정하기 위해 `beforeunload` 이벤트를 사용하려고 할 것입니다.
이 이벤트는 윈도우 콘텐츠를 새로고칠 때도 발생합니다.
Electron에선 `undefined`가 아닌 이외의 값을 전달할 경우 윈도우 종료를 취소합니다.
예시는 다음과 같습니다:

```javascript
window.onbeforeunload = (e) => {
  console.log('I do not want to be closed')

  // 일반적인 브라우저와는 달리 사용자에게 확인 창을 보여주지 않고, non-void 값을 반환하면
  // 조용히 닫기를 취소합니다.
  // Dialog API를 통해 사용자가 애플리케이션을 종료할지 정할 수 있도록 확인 창을 표시하는 것을
  // 추천합니다.
  e.returnValue = false
}
```

#### Event: 'closed'

윈도우 종료가 완료된 경우 발생하는 이벤트입니다. 이 이벤트가 발생했을 경우 반드시
윈도우의 레퍼런스가 더 이상 사용되지 않도록 제거해야 합니다.

#### Event: 'unresponsive'

웹 페이지가 응답하지 않을 때 발생하는 이벤트입니다.

#### Event: 'responsive'

응답하지 않는 웹 페이지가 다시 응답하기 시작했을 때 발생하는 이벤트입니다.

#### Event: 'blur'

윈도우가 포커스를 잃었을 때 발생하는 이벤트입니다.

#### Event: 'focus'

윈도우가 포커스를 가졌을 때 발생하는 이벤트입니다.

#### Event: 'show'

윈도우가 보여진 상태일 때 발생하는 이벤트입니다.

#### Event: 'hide'

윈도우가 숨겨진 상태일 때 발생하는 이벤트입니다.

#### Event: 'ready-to-show'

웹 페이지가 완전히 랜더링되어 윈도우가 시각적인 깜빡임없이 콘텐츠를 보여줄 수 있을 때
발생하는 이벤트입니다.

#### Event: 'maximize'

윈도우가 최대화됐을 때 발생하는 이벤트입니다.

#### Event: 'unmaximize'

윈도우의 최대화 상태가 해제되었을 때 발생하는 이벤트입니다.

#### Event: 'minimize'

윈도우가 최소화됐을 때 발생하는 이벤트입니다.

#### Event: 'restore'

윈도우가 최소화 상태에서 복구되었을 때 발생하는 이벤트입니다.

#### Event: 'resize'

윈도우의 크기가 재조정될 때 발생하는 이벤트입니다.

#### Event: 'move'

윈도우가 새로운 위치로 이동될 때 발생하는 이벤트입니다.

__참고__: macOS에선 이 이벤트가 그저 `moved` 이벤트의 별칭(alias)으로 사용됩니다.

#### Event: 'moved' _macOS_

윈도우가 새로운 위치로 이동되었을 때 발생하는 이벤트입니다. (한 번만)

#### Event: 'enter-full-screen'

윈도우가 풀 스크린 모드로 진입할 때 발생하는 이벤트입니다.

#### Event: 'leave-full-screen'

윈도우가 풀 스크린 모드에서 해제될 때 발생하는 이벤트입니다.

#### Event: 'enter-html-full-screen'

윈도우가 HTML API에 의해 풀 스크린 모드로 진입할 때 발생하는 이벤트입니다.

#### Event: 'leave-html-full-screen'

윈도우가 HTML API에 의해 풀 스크린 모드에서 해제될 때 발생하는 이벤트입니다.

#### Event: 'app-command' _Windows_

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
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.on('app-command', (e, cmd) => {
  // Navigate the window back when the user hits their mouse back button
  if (cmd === 'browser-backward' && win.webContents.canGoBack()) {
    win.webContents.goBack()
  }
})
```

#### Event: 'scroll-touch-begin' _macOS_

스크롤 휠 이벤트가 동작하기 시작했을 때 발생하는 이벤트입니다.

#### Event: 'scroll-touch-end' _macOS_

스크롤 휠 이벤트가 동작을 멈췄을 때 발생하는 이벤트입니다.

#### Event: 'scroll-touch-edge' _macOS_

스크롤 휠 이벤트로 요소의 끝에 도달했을 때 발생하는 이벤트입니다.

#### Event: 'swipe' _macOS_

Returns:

* `event` Event
* `direction` String

3-손가락 스와이프가 작동할 때 발생하는 이벤트입니다. 방향은 `up`, `right`, `down`,
`left`가 될 수 있습니다.

### Static Methods

`BrowserWindow` 클래스는 다음과 같은 정적 메서드를 가지고 있습니다:

#### `BrowserWindow.getAllWindows()`

Returns `BrowserWindow[]` - 열려있는 모든 브라우저 윈도우의 배열.

#### `BrowserWindow.getFocusedWindow()`

Returns `BrowserWindow` - 애플리케이션에서 포커스된 윈도우. 없을 경우 `null`.

#### `BrowserWindow.fromWebContents(webContents)`

* `webContents` [WebContents](web-contents.md)

Returns `BrowserWindow` - `webContents` 를 소유한 윈도우.

#### `BrowserWindow.fromId(id)`

* `id` Integer

Returns `BrowserWindow` - `id` 에 해당하는 윈도우.

#### `BrowserWindow.addDevToolsExtension(path)`

* `path` String

`path`에 있는 개발자 도구 확장 기능을 추가합니다. 그리고 확장 기능의 이름을 반환합니다.

확장 기능은 기억됩니다. 따라서 API는 단 한 번만 호출되어야 합니다. 이 API는 실제
프로그램 작성에 사용할 수 없습니다. 만약 이미 로드된 확장 기능을 추가하려 한다면, 이
메서드는 아무것도 반환하지 않고 콘솔에 경고가 로그됩니다.

**참고:** 이 API는 `app` 모듈의 `ready` 이벤트가 발생하기 전까지 사용할 수 없습니다.

#### `BrowserWindow.removeDevToolsExtension(name)`

* `name` String

`name`에 해당하는 개발자 도구 확장 기능을 제거합니다.

**참고:** 이 API는 `app` 모듈의 `ready` 이벤트가 발생하기 전까지 사용할 수 없습니다.

#### `BrowserWindow.getDevToolsExtensions()`

Returns `Object` - 키는 확장 기능 이름을 값은 `name`과 `version` 속성을 포함하는
객체를 가집니다.

개발자 도구 확장 기능이 설치되었는지 확인하려면 다음과 같이 실행할 수 있습니다:

```javascript
const {BrowserWindow} = require('electron')

let installed = BrowserWindow.getDevToolsExtensions().hasOwnProperty('devtron')
console.log(installed)
```

**참고:** 이 API는 `app` 모듈의 `ready` 이벤트가 발생하기 전까지 사용할 수 없습니다.

### Instance Properties

`new BrowserWindow`로 생성한 객체는 다음과 같은 속성을 가지고 있습니다:

```javascript
const {BrowserWindow} = require('electron')
// `win`은 BrowserWindow의 인스턴스입니다
let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('https://github.com')
```

#### `win.webContents`

윈도우의 `WebContents` 객체입니다. 모든 웹 페이지와 관련된 이벤트와 작업이 이 객체를
통해 수행됩니다.

메서드나 이벤트에 대한 자세한 내용은 [`webContents` 문서](web-contents.md)를
참고하세요.

#### `win.id`

`Integer` 형식의 윈도우 고유 ID 입니다.

### Instance Methods

`new BrowserWindow`로 생성한 객체는 다음과 같은 메서드들을 가지고 있습니다:

**참고:** 몇몇 메서드들은 라벨에서 특정한 운영체제 시스템에서만 작동합니다.

#### `win.destroy()`

윈도우를 강제로 닫습니다. 웹 페이지의 `unload` 와 `beforeunload` 이벤트는 일어나지
않습니다. 또한 이 윈도우의 `close`도 일어나지 않습니다. 하지만 `closed` 이벤트는
반드시 발생함을 보장합니다.

#### `win.close()`

윈도우의 종료를 시도합니다. 이 메서드는 사용자가 윈도우의 닫기 버튼을 클릭했을 때와
같은 효과를 냅니다. 웹 페이지는 로드가 취소되고 종료됩니다. 자세한 내용은
[close 이벤트](#event-close)를 참고하세요.

#### `win.focus()`

윈도우에 포커스를 맞춥니다.

#### `win.blur()`

윈도우의 포커스를 없앱니다.

#### `win.isFocused()`

Returns `Boolean` - 윈도우가 포커스되었는지 여부.

#### `win.isDestroyed()`

Returns `Boolean` - 윈도우가 소멸되었는지 여부.

#### `win.show()`

윈도우를 표시하고 포커스합니다.

#### `win.showInactive()`

윈도우를 표시만 하고 포커스하지 않습니다.

#### `win.hide()`

윈도우를 숨깁니다.

#### `win.isVisible()`

Returns `Boolean` - 윈도우가 사용자에게 표시되고 있는지 여부.

#### `win.isModal()`

Returns `Boolean` - 현재 윈도우가 모달 윈도우인지 여부.

#### `win.maximize()`

윈도우를 최대화 시킵니다.

#### `win.unmaximize()`

윈도우 최대화를 취소합니다.

#### `win.isMaximized()`

Returns `Boolean` - 윈도우가 최대화 되어있는지 여부.

#### `win.minimize()`

윈도우를 최소화 시킵니다. 어떤 플랫폼은 최소화된 윈도우가 Dock에 표시됩니다.

#### `win.restore()`

최소화된 윈도우를 이전 상태로 되돌립니다.

#### `win.isMinimized()`

Returns `Boolean` - 윈도우가 최소화되었는지 여부.

#### `win.setFullScreen(flag)`

* `flag` Boolean

윈도우의 전체화면 상태를 지정합니다.

#### `win.isFullScreen()`

Returns `Boolean` - 윈도우가 전체화면 모드인지 여부.

#### `win.setAspectRatio(aspectRatio[, extraSize])` _macOS_

* `aspectRatio` Float - 유지하려 하는 콘텐츠 뷰 일부의 종횡비.
* `extraSize` Object (optional) - 종횡비를 유지하는 동안 포함되지 않을 엑스트라 크기.
  * `width` Integer
  * `height` Integer

이 메서드는 윈도우의 종횡비를 유지하는 기능을 수행합니다. 엑스트라 크기는 개발자가
픽셀로 특정한 공간이 있을 때 종횡비 계산에서 제외됩니다. 이 API는 윈도우의 크기와
콘텐츠 사이즈의 차이를 이미 고려하고 있습니다.

일반 윈도우에서 작동하는 HD 비디오 플레이어와 관련된 컨트롤을 고려합니다.
만약 15 픽셀의 컨트롤이 왼쪽 가장자리에 있고 25 픽셀의 컨트롤이 오른쪽 가장자리에
있으며 50 픽셀의 컨트롤이 플레이어 밑에 있을 때 플레이어 자체가 16:9 종횡비(HD의 표준
종횡비는 @1920x1080)를 유지하기 위해선 이 함수를 16/9, [ 40, 50 ] 인수와 함께
호출해야 합니다. 두번째 인수 엑스트라 크기는 존재하는 크기만 관여하고 콘텐츠 뷰 내의
크기는 관여하지 않습니다. 그저 전체 콘텐츠 뷰 내에 있는 모든 엑스트라 너비, 높이 영역이
합해집니다.

#### `win.previewFile(path[, displayName])` _macOS_

* `path` String - QuickLook 으로 미리 볼 파일에 대한 절대 경로. Quick Look 이
  열기 위한 파일의 컨텐츠 형식을 결정하는데 경로의 파일명과 확장자를 사용하기
 때문에 중요합니다.
* `displayName` String (Optional) - Quick Look 모달 뷰에 표시되는 파일의 이름.
  이것은 순전히 보여주는 용도이며 파일의 컨텐츠 형식에 영향을 주지 않습니다.
  기본값은 `path` 입니다.

주어진 경로의 파일을 미리 보여주기 위해 [Quick Look][quick-look] 을 사용하세요.

#### `win.setBounds(bounds[, animate])`

* `bounds` [Rectangle](structures/rectangle.md)
* `animate` Boolean (optional) _macOS_

윈도우를 주어진 영역으로 크기 재조정 및 이동합니다.

#### `win.getBounds()`

Returns [`Rectangle`](structures/rectangle.md)

#### `win.setContentBounds(bounds[, animate])`

* `bounds` [Rectangle](structures/rectangle.md)
* `animate` Boolean (optional) _macOS_

윈도우의 내부 영역 (예. 웹페이지) 을 주어진 영역으로 크기 재조정 및 이동합니다.

#### `win.getContentBounds()`

* `bounds` [`Rectangle`](structures/rectangle.md)

윈도우의 클라이언트 영역 (웹 페이지)의 너비, 높이, x, y 값을 포함하는 객체를
반환합니다.

#### `win.setSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _macOS_

`width`와 `height` 값으로 윈도우 크기를 재조정합니다. (너비, 높이)

#### `win.getSize()`

Returns `Integer[]` - 윈도우의 너비, 높이를 포함.

#### `win.setContentSize(width, height[, animate])`

* `width` Integer
* `height` Integer
* `animate` Boolean (optional) _macOS_

윈도우 클라이언트 영역(웹 페이지)의 크기를 `width`, `height`로 재조정합니다.

#### `win.getContentSize()`

Returns `Integer[]` - 윈도우 내부 영역의 너비, 높이를 포함.

#### `win.setMinimumSize(width, height)`

* `width` Integer
* `height` Integer

윈도우의 최소 `width`, `height` 크기를 지정합니다.

#### `win.getMinimumSize()`

Returns `Integer[]` - 윈도우의 최소 너비, 높이를 포함.

#### `win.setMaximumSize(width, height)`

* `width` Integer
* `height` Integer

윈도우의 최대 `width`, `height` 크기를 지정합니다.

#### `win.getMaximumSize()`

Returns `Integer[]` - 윈도우의 최대 너비, 높이를 포함.

#### `win.setResizable(resizable)`

* `resizable` Boolean

사용자에 의해 윈도우의 크기가 재조정될 수 있는지를 지정합니다.

#### `win.isResizable()`

Returns `Boolean` - 사용자에 의해 윈도우의 크기가 재조정될 수 있는지 여부.

#### `win.setMovable(movable)` _macOS_ _Windows_

* `movable` Boolean

사용자에 의해 윈도우를 이동시킬 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

#### `win.isMovable()` _macOS_ _Windows_

Returns `Boolean` - 사용자에 의해 윈도우를 이동시킬 수 있는지 여부.

Linux에선 항상 `true`를 반환합니다.

#### `win.setMinimizable(minimizable)` _macOS_ _Windows_

* `minimizable` Boolean

사용자에 의해 윈도우를 최소화시킬 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

#### `win.isMinimizable()` _macOS_ _Windows_

Returns `Boolean` - 사용자에 의해 윈도우를 최소화시킬 수 있는지 여부.

Linux에선 항상 `true`를 반환합니다.

#### `win.setMaximizable(maximizable)` _macOS_ _Windows_

* `maximizable` Boolean

사용자에 의해 윈도우를 최대화시킬 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

#### `win.isMaximizable()` _macOS_ _Windows_

Returns `Boolean` - 사용자에 의해 윈도우를 최대화시킬 수 있는지 여부.

Linux에선 항상 `true`를 반환합니다.

#### `win.setFullScreenable(fullscreenable)`

* `fullscreenable` Boolean

최대화/줌 버튼이 전체화면 모드 또는 윈도우 최대화를 토글할 수 있게 할지 여부를
지정합니다.

#### `win.isFullScreenable()`

Returns `Boolean` - 최대화/줌 버튼이 전체화면 모드 또는 윈도우 최대화를 토글할
수 있는지 여부.

#### `win.setClosable(closable)` _macOS_ _Windows_

* `closable` Boolean

사용자에 의해 윈도우가 수동적으로 닫힐 수 있는지 여부를 지정합니다. Linux에선 아무 일도
일어나지 않습니다.

#### `win.isClosable()` _macOS_ _Windows_

Returns `Boolean` - 사용자에 의해 윈도우가 수동적으로 닫힐 수 있는지 여부.

Linux에선 항상 `true`를 반환합니다.

#### `win.setAlwaysOnTop(flag[, level])`

* `flag` Boolean
* `level` String (optional) _macOS_ - 이 값은 `normal`, `floating`,
  `torn-off-menu`, `modal-panel`, `main-menu`, `status`, `pop-up-menu`,
  `screen-saver`, `dock` 을 포함합니다. 기본값은 `floating` 입니다. 자세한
  내용은 [macOS 문서][window-levels] 를 보세요.

윈도우가 언제나 다른 윈도우들 위에 표시되는지 여부를 지정합니다. 이 설정을 활성화 하면
윈도우는 포커스 될 수 없는 툴박스 윈도우가 아닌 일반 윈도우로 유지됩니다.

#### `win.isAlwaysOnTop()`

윈도우가 언제나 다른 윈도우들 위에 표시되는지 여부를 반환합니다.

#### `win.center()`

윈도우를 화면 정 중앙으로 이동합니다.

#### `win.setPosition(x, y[, animate])`

* `x` Integer
* `y` Integer
* `animate` Boolean (optional) _macOS_

윈도우의 위치를 `x`, `y`로 이동합니다.

#### `win.getPosition()`

Returns `Integer[]` - 윈도우의 현재 위치.

#### `win.setTitle(title)`

* `title` String

`title`을 네이티브 윈도우의 제목으로 지정합니다.

#### `win.getTitle()`

Returns `String` - 네이티브 윈도우의 제목.

**참고:** 웹 페이지의 제목과 네이티브 윈도우의 제목은 서로 다를 수 있습니다.

#### `win.setSheetOffset(offsetY[, offsetX])` _macOS_

* `offsetY` Float
* `offsetX` Float (optional)

macOS에서 시트를 부착할 위치를 지정합니다. 기본적으로 시트는 윈도우의 프레임 바로
아래의 위치에 부착됩니다. 아마도 이 기능은 보통 다음과 같이 HTML 렌더링된 툴바 밑에
표시하기 위해 사용할 것입니다:

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

let toolbarRect = document.getElementById('toolbar').getBoundingClientRect()
win.setSheetOffset(toolbarRect.height)
```

#### `win.flashFrame(flag)`

* `flag` Boolean

사용자가 윈도우에 관심을 가질 수 있도록 창을 깜빡이거나 이를 중지합니다.

#### `win.setSkipTaskbar(skip)`

* `skip` Boolean

애플리케이션 아이콘을 작업표시줄에 보이지 않도록 설정합니다.

#### `win.setKiosk(flag)`

* `flag` Boolean

Kiosk(키오스크) 모드를 설정합니다.

#### `win.isKiosk()`

Returns `Boolean' - 현재 윈도우가 kiosk 모드인지 여부.

#### `win.getNativeWindowHandle()`

Returns `Buffer` - 플랫폼 별 윈도우의 핸들.

핸들의 타입에 따라 적절히 캐스팅됩니다. Windows의 `HWND`, macOS의 `NSView*`, Linux의
`Window` (`unsigned long`)를 예로 들 수 있습니다.

#### `win.hookWindowMessage(message, callback)` _Windows_

* `message` Integer
* `callback` Function

Windows 메시지 훅을 등록합니다. `callback`은 WndProc에서 메시지를 받았을 때
호출됩니다.

#### `win.isWindowMessageHooked(message)` _Windows_

* `message` Integer

Returns `Boolean` - 지정한 메시지가 후킹됐는지에 따라 `true` 또는 `false`.

#### `win.unhookWindowMessage(message)` _Windows_

* `message` Integer

지정한 메시지 훅을 등록 해제합니다.

#### `win.unhookAllWindowMessages()` _Windows_

모든 메시지 훅을 등록 해제합니다.

#### `win.setRepresentedFilename(filename)` _macOS_

* `filename` String

윈도우 대표 파일의 경로명을 설정합니다. 파일의 아이콘이 윈도우 타이틀 바에 표시됩니다.

#### `win.getRepresentedFilename()` _macOS_

Returns `String` - 윈도우 대표 파일의 경로.

#### `win.setDocumentEdited(edited)` _macOS_

* `edited` Boolean

윈도우의 문서가 변경되었는지 여부를 설정합니다. 그리고 `true`로 설정했을 때 타이틀 바의
아이콘이 회색으로 표시됩니다.

#### `win.isDocumentEdited()` _macOS_

Returns `Boolean` - 윈도우의 문서가 변경되었는지 여부.

#### `win.focusOnWebView()`

#### `win.blurWebView()`

#### `win.capturePage([rect, ]callback)`

* `rect` [Rectangle](structures/rectangle.md) (optional) - 캡쳐될 페이지의 영역
* `callback` Function
  * `image` [NativeImage](native-image.md)

`webContents.capturePage([rect, ]callback)`와 같습니다.

#### `win.loadURL(url[, options])`

* `url` URL
* `options` Object (optional)
  * `httpReferrer` String - HTTP 리퍼러 URL.
  * `userAgent` String - 요청을 보낸 사용자의 user agent.
  * `extraHeaders` String - "\n"로 구분된 부가적인 헤더

`webContents.loadURL(url[, options])` API와 같습니다.

`url`은 원격 주소 (e.g. `http://`)가 되거나 `file://` 프로토콜을 사용하는 로컬 HTML
경로가 될 수 있습니다.

파일 URL이 올바른 형식으로 지정되었는지 확인하려면 Node의
[`url.format`](https://nodejs.org/api/url.html#url_url_format_urlobject) 메서드를
사용하는 것을 권장합니다.

```javascript
let url = require('url').format({
  protocol: 'file',
  slashes: true,
  pathname: require('path').join(__dirname, 'index.html')
})

win.loadURL(url)
```

#### `win.reload()`

`webContents.reload` API와 같습니다.

#### `win.setMenu(menu)` _Linux_ _Windows_

* `menu` Menu

지정한 `menu`를 윈도우의 메뉴로 설정합니다 `null`을 설정하면 메뉴를 제거합니다.

#### `win.setProgressBar(progress[, options])`

* `progress` Double
* `options` Object (optional)
  * `mode` String _Windows_ - 프로그레스 막대의 모드 (`none`, `normal`,
    `indeterminate`, `error`, `paused`)

작업표시줄에 표시되고 있는 애플리케이션 아이콘에 진행 상태를 표시합니다. [0, 1.0]
사이의 값을 지정할 수 있습니다.

진행 상태가 < 0 이 되면 진행 상태 표시를 제거합니다.
진행 상태가 > 1 이 되면 불확정 상태 표시로 전환합니다.

Linux 플랫폼에선 Unity 데스크톱 환경만 지원합니다. 그리고 이 기능을 사용하려면
`*.desktop` 파일을 생성한 후 `package.json`의 `desktopName` 필드에 파일 이름을
지정해야 합니다. 기본적으로 `app.getName().desktop`을 통해 접근합니다.

Windows에선 모드를 전달할 수 있습니다. 사용할 수 있는 값은 `none`, `normal`,
`indeterminate`, `error`, 그리고 `paused`가 있습니다. 만약 모드 설정 없이
`setProgressBar`를 호출하면 (올바른 범위의 값을 전달했을 때), `normal`이 기본적으로
설정됩니다.

#### `win.setOverlayIcon(overlay, description)` _Windows_

* `overlay` [NativeImage](native-image.md) - 작업표시줄 아이콘의 우측 하단에 표시될
아이콘입니다. `null`로 지정하면 빈 오버레이가 사용됩니다
* `description` String - 접근성 설정에 의한 스크린 리더에 제공될 설명입니다

현재 작업표시줄 아이콘에 16 x 16 픽셀 크기의 오버레이를 지정합니다. 보통 이 기능은
애플리케이션의 여러 상태를 사용자에게 소극적으로 알리기 위한 방법으로 사용됩니다.

#### `win.setHasShadow(hasShadow)` _macOS_

* `hasShadow` Boolean

윈도우가 그림자를 가질지 여부를 지정합니다. Windows와 Linux에선 아무 일도 일어나지
않습니다.

#### `win.hasShadow()` _macOS_

Returns `Boolean` - 윈도우가 그림자를 가지고 있는지 여부.

Windows와 Linux에선 항상 `true`를 반환합니다.

#### `win.setThumbarButtons(buttons)` _Windows_

* `buttons` [ThumbarButton[]](structures/thumbar-button.md)

Returns `Boolean` - 버튼이 성공적으로 추가되었는지 여부

윈도우 작업표시줄 버튼 레이아웃의 미리보기 이미지 영역에 미리보기 툴바와 버튼 세트를
추가합니다. 반환되는 `Boolean` 값은 미리보기 툴바가 성공적으로 추가됬는지를 알려줍니다.

미리보기 이미지 영역의 제한된 크기로 인해 미리보기 툴바에 추가될 수 있는 최대 버튼의
개수는 7개이며 이 이상 추가될 수 없습니다. 플랫폼의 제약으로 인해 미리보기 툴바는 한 번
설정되면 삭제할 수 없습니다. 하지만 이 API에 빈 배열을 전달하여 버튼들을 제거할 수
있습니다.

`buttons`는 `Button` 객체의 배열입니다:

* `Button` 객체
  * `icon` [NativeImage](native-image.md) - 미리보기 툴바에 보여질 아이콘.
  * `click` Function
  * `tooltip` String (optional) - 버튼의 툴팁 텍스트.
  * `flags` String[] (optional) - 버튼의 특정 동작 및 상태 제어. 기본적으로
    `enabled`이 사용됩니다.

`flags` 는 다음 `String` 들을 포함할 수 있는 배열입니다:
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

#### `win.setThumbnailClip(region)` _Windows_

* `region` [Rectangle](structures/rectangle.md) - 윈도우의 영역

작업 표시줄에 윈도우의 섬네일이 표시될 때 섬네일 이미지로 사용할 윈도우의 영역을
지정합니다. 빈 영역을 지정하는 것으로 전체 윈도우의 섬네일로 초기화할 수 있습니다:
`{x: 0, y: 0, width: 0, height: 0}`.

#### `win.setThumbnailToolTip(toolTip)` _Windows_

* `toolTip` String

작업 표시줄의 윈도우 섬네일 위에 표시될 툴팁을 설정합니다.

#### `win.showDefinitionForSelection()` _macOS_

`webContents.showDefinitionForSelection()`와 같습니다.

#### `win.setIcon(icon)` _Windows_ _Linux_

* `icon` [NativeImage](native-image.md)

윈도우 아이콘을 변경합니다.

#### `win.setAutoHideMenuBar(hide)`

* `hide` Boolean

메뉴 막대 자동 숨김 기능을 활성화 합니다.
숨겨진 메뉴는 사용자가 `Alt` 키를 단일 입력했을 때만 표시됩니다.

메뉴 막대가 이미 표시되고 있을 때 `setAutoHideMenuBar(true)`를 호출한다고 해서
메뉴가 즉시 숨겨지지는 않습니다.

#### `win.isMenuBarAutoHide()`

Returns `Boolean` - 메뉴 막대 자동 숨김 상태 여부.

#### `win.setMenuBarVisibility(visible)` _Windows_ _Linux_

* `visible` Boolean

메뉴 막대의 표시 여부를 설정합니다. 만약 메뉴 막대 자동 숨김 상태라면 여전히 사용자가
`Alt` 키를 입력하여 메뉴 막대를 표시되도록 할 수 있습니다.

**역자주:** 기본 메뉴 막대를 완전히 없애려면 `win.setMenu(null)`를 호출해야 합니다.
단순히 이 API를 사용하면 여전히 메뉴에 등록된 핫 키가 작동합니다.

#### `win.isMenuBarVisible()`

Returns `Boolean` - 메뉴 막대가 표시되고 있는지 여부.

#### `win.setVisibleOnAllWorkspaces(visible)`

* `visible` Boolean

윈도우가 모든 워크스페이스에서 표시될지 여부를 설정합니다.

**참고:** 이 API는 Windows에서 아무 일도 하지 않습니다.

#### `win.isVisibleOnAllWorkspaces()`

Returns `Boolean` - 윈도우가 모든 워크스페이스에서 표시될지 여부.

**참고:** 이 API는 Windows에서 언제나 false를 반환합니다.

#### `win.setIgnoreMouseEvents(ignore)`

* `ignore` Boolean

윈도우가 모든 마우스 이벤트를 무시하게 만듭니다.

이 윈도우에서 일어나는 모든 마우스 이벤트가 이 윈도우 밑의 윈도우로 전달됩니다. 하지만
이 윈도우가 포커스되어 있다면, 여전히 키보드 이벤트는 받을 수 있습니다.

#### `win.setContentProtection(enable)` _macOS_ _Windows_

* `enable` Boolean

윈도우 콘텐츠가 외부 어플리케이션에 의해 캡쳐되는 것을 막습니다.

macOS에선 NSWindow의 sharingType을 NSWindowSharingNone로 설정합니다.
Windows에선 `WDA_MONITOR`와 함께 SetWindowDisplayAffinity를 호출합니다.

#### `win.setFocusable(focusable)` _Windows_

* `focusable` Boolean

윈도우가 포커스될 수 있는지 여부를 변경합니다.

#### `win.setParentWindow(parent)` _Linux_ _macOS_

* `parent` BrowserWindow
`
`parent` 인수를 현재 윈도우의 부모 윈도우로 설정합니다. `null`로 설정하면
현재 윈도우를 상위 윈도우로 전환합니다.

#### `win.getParentWindow()`

Returns `BrowserWindow` - 부모 윈도우.

#### `win.getChildWindows()`

Returns `BrowserWindow[]` - 모든 자식 윈도우.

[blink-feature-string]: https://cs.chromium.org/chromium/src/third_party/WebKit/Source/platform/RuntimeEnabledFeatures.in
[window-levels]: https://developer.apple.com/reference/appkit/nswindow/1664726-window_levels
[quick-look]: https://en.wikipedia.org/wiki/Quick_Look
