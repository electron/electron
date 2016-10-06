# webContents

> 웹 페이지를 렌더링하고 제어합니다.

`webContents`는 [EventEmitter](http://nodejs.org/api/events.html#events_class_events_eventemitter)를
상속받았습니다. 웹 페이지의 렌더링과 관리를 책임지며
[`BrowserWindow`](browser-window.md)의 속성입니다. 다음은 `webContents` 객체에
접근하는 예시입니다:

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({width: 800, height: 1500})
win.loadURL('http://github.com')

let contents = win.webContents
console.log(contents)
```

## Methods

다음 메서드는 `webContents` 모듈에서 접근할 수 있는 메서드입니다:

```javascript
const {webContents} = require('electron')
console.log(webContents)
```

#### `webContents.getAllWebContents()`

Returns `WebContents[]` - 모든 `WebContents` 인스턴스의 배열. 이 배열은 윈도우,
웹뷰, 열린 개발자 도구 그리고 백그라운드 페이지의 개발자 도구 확장 기능의 모든
웹 콘텐츠를 포함합니다.

#### `webContents.getFocusedWebContents()`

Returns `WebContents` - 이 애플리케이션에서 포커스되어있는 웹 콘텐츠. 없을 경우
`null` 을 반환합니다.

### `webContents.fromId(id)`

* `id` Integer

Returns `WebContents` - ID 에 해당하는 WebContens 인스턴스.

## Class: WebContents

> BrowserWindow 인스턴스의 콘텐츠를 표시하고 제어합니다.

### Instance Events

#### Event: 'did-finish-load'

탐색 작업이 끝났을 때 발생하는 이벤트입니다. 브라우저의 탭의 스피너가 멈추고 `onload`
이벤트가 발생했을 때를 말합니다.

### Event: 'did-fail-load'

Returns:

* `event` Event
* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String
* `isMainFrame` Boolean

이 이벤트는 `did-finish-load`와 비슷하나, 로드가 실패했거나 취소되었을 때 발생합니다.
예를 들면 `window.stop()`이 실행되었을 때 발생합니다. 발생할 수 있는 전체 에러 코드의
목록과 설명은 [여기서](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h)
확인할 수 있습니다. 참고로 리다이렉트 응답은 `errorCode` -3과 함께 발생합니다; 이
에러는 명시적으로 무시할 수 있습니다.

#### Event: 'did-frame-finish-load'

Returns:

* `event` Event
* `isMainFrame` Boolean

프레임(Frame)이 탐색을 끝냈을 때 발생하는 이벤트입니다.

#### Event: 'did-start-loading'

브라우저 탭의 스피너가 회전을 시작한 때와 같은 시점에 대응하는 이벤트입니다.

#### Event: 'did-stop-loading'

브라우저 탭의 스피너가 회전을 멈추었을 때와 같은 시점에 대응하는 이벤트입니다.

#### Event: 'did-get-response-details'

Returns:

* `event` Event
* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

요청한 리소스에 관련된 자세한 정보를 사용할 수 있을 때 발생하는 이벤트입니다.
`status`는 리소스를 다운로드하기 위한 소켓 연결을 나타냅니다.

#### Event: 'did-get-redirect-request'

Returns:

* `event` Event
* `oldURL` String
* `newURL` String
* `isMainFrame` Boolean
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

리소스를 요청하는 동안에 리다이렉트 응답을 받았을 때 발생하는 이벤트입니다.

#### Event: 'dom-ready'

Returns:

* `event` Event

주어진 프레임의 문서가 로드되었을 때 발생하는 이벤트입니다.

#### Event: 'page-favicon-updated'

Returns:

* `event` Event
* `favicons` String[] - URL 배열

페이지가 favicon(파비콘) URL을 받았을 때 발생하는 이벤트입니다.

#### Event: 'new-window'

Returns:

* `event` Event
* `url` String
* `frameName` String
* `disposition` String - `default`, `foreground-tab`, `background-tab`,
  `new-window`, `save-to-disk`, `other`중 하나일 수 있습니다.
* `options` Object - 새로운 `BrowserWindow` 객체를 만들 때 사용되는 옵션 객체입니다.
* `additionalFeatures` Array - `window.open()` 에 주어진 (Chromium 또는 Electron
  에 의해 처리되지 않는) 비표준 기능.

페이지가 `url`에 대하여 새로운 윈도우를 열기위해 요청한 경우 발생하는 이벤트입니다.
`window.open`이나 `<a target='_blank'>`과 같은 외부 링크에 의해 요청될 수 있습니다.

기본값으로 `BrowserWindow`는 `url`을 기반으로 생성됩니다.

`event.preventDefault()`를 호출하면 새로운 창이 생성되는 것을 방지할 수
있습니다. 이 경우, `event.newGuest` 는 Electron 의 런타임에 의해 사용할 수 있게
`BrowserWindow` 인스턴스에 대한 참조를 설정할 수 있습니다.

#### Event: 'will-navigate'

Returns:

* `event` Event
* `url` String

사용자 또는 페이지가 새로운 페이지로 이동할 때 발생하는 이벤트입니다.
`window.location` 객체가 변경되거나 사용자가 페이지의 링크를 클릭했을 때 발생합니다.

이 이벤트는 `webContents.loadURL`과 `webContents.back` 같은 API를 이용한
프로그램적으로 시작된 탐색에 대해서는 발생하지 않습니다.

이 이벤트는 앵커 링크를 클릭하거나 `window.location.hash`의 값을 변경하는 등의 페이지
내 탐색시엔 발생하지 않습니다. 대신 `did-navigate-in-page` 이벤트를 사용해야 합니다.

`event.preventDefault()`를 호출하면 탐색을 방지할 수 있습니다.

#### Event: 'did-navigate'

Returns:

* `event` Event
* `url` String

탐색이 완료되면 발생하는 이벤트입니다.

이 이벤트는 앵커 링크를 클릭하거나 `window.location.hash`의 값을 변경하는 등의 페이지
내 탐색시엔 발생하지 않습니다. 대신 `did-navigate-in-page` 이벤트를 사용해야 합니다.

#### Event: 'did-navigate-in-page'

Returns:

* `event` Event
* `url` String
* `isMainFrame` Boolean

페이지 내의 탐색이 완료되면 발생하는 이벤트입니다.

페이지 내의 탐색이 발생하면 페이지 URL이 변경되지만 페이지 밖으로의 탐색은 일어나지
않습니다. 예를 들어 앵커 링크를 클릭했을 때, 또는 DOM `hashchange` 이벤트가 발생했을
때로 볼 수 있습니다.

#### Event: 'crashed'

Returns:

* `event` Event
* `killed` Boolean

렌더러 프로세스가 충돌하거나 종료될 때 발생되는 이벤트입니다.

#### Event: 'plugin-crashed'

Returns:

* `event` Event
* `name` String
* `version` String

플러그인 프로세스가 예기치 못하게 종료되었을 때 발생되는 이벤트입니다.

#### Event: 'destroyed'

`webContents`가 소멸될 때 발생되는 이벤트입니다.

#### Event: 'devtools-opened'

개발자 도구가 열렸을 때 발생되는 이벤트입니다.

#### Event: 'devtools-closed'

개발자 도구가 닫혔을 때 발생되는 이벤트입니다.

#### Event: 'devtools-focused'

개발자 도구에 포커스가 가거나 개발자 도구가 열렸을 때 발생되는 이벤트입니다.

#### Event: 'certificate-error'

Returns:

* `event` Event
* `url` URL
* `error` String - 에러 코드
* `certificate` Object
  * `data` String - PEM 인코딩된 데이터
  * `issuerName` String - 인증서 발급자의 공통 이름
  * `subjectName` String - 대상의 공통 이름
  * `serialNumber` String - 문자열로 표현된 hex 값
  * `validStart` Integer - 초 단위의 인증서가 유효하기 시작한 날짜
  * `validExpiry` Integer - 초 단위의 인증서가 만료되는 날짜
  * `fingerprint` String - 인증서의 지문
* `callback` Function

`url`에 대한 `certificate` 인증서의 유효성 검증에 실패했을 때 발생하는 이벤트입니다.

사용법은 [`app`의 `certificate-error` 이벤트](app.md#event-certificate-error)와
같습니다.

#### Event: 'select-client-certificate'

Returns:

* `event` Event
* `url` URL
* `certificateList` [Objects]
  * `data` String - PEM 인코딩된 데이터
  * `issuerName` String - 인증서 발급자의 공통 이름
  * `subjectName` String - 대상의 공통 이름
  * `serialNumber` String - 문자열로 표현된 hex 값
  * `validStart` Integer - 초 단위의 인증서가 유효하기 시작한 날짜
  * `validExpiry` Integer - 초 단위의 인증서가 만료되는 날짜
  * `fingerprint` String - 인증서의 지문
* `callback` Function

클라이언트 인증이 요청되었을 때 발생하는 이벤트입니다.

사용법은 [`app`의 `select-client-certificate` 이벤트](app.md#event-select-client-certificate)와
같습니다.

#### Event: 'login'

Returns:

* `event` Event
* `request` Object
  * `method` String
  * `url` URL
  * `referrer` URL
* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

`webContents`가 기본 인증을 수행하길 원할 때 발생되는 이벤트입니다.

[`app`의 `login`이벤트](app.md#event-login)와 사용 방법은 같습니다.

#### Event: 'found-in-page'

Returns:

* `event` Event
* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - 더 많은 응답이 따르는 경우를 표시합니다.
  * `activeMatchOrdinal` Integer (optional) - 활성화 일치의 위치.
  * `matches` Integer (optional) - 일치하는 개수.
  * `selectionArea` Object (optional) - 첫 일치 부위의 좌표.

[`webContents.findInPage`] 요청의 결과를
사용할 수 있을 때 발생하는 이벤트입니다.

#### Event: 'media-started-playing'

미디어가 재생되기 시작할 때 발생하는 이벤트입니다.

#### Event: 'media-paused'

미디어가 중지되거나 재생이 완료되었을 때 발생하는 이벤트입니다.

#### Event: 'did-change-theme-color'

페이지의 테마 색이 변경될 때 발생하는 이벤트입니다. 이 이벤트는 보통 meta 태그에
의해서 발생합니다:

```html
<meta name='theme-color' content='#ff0000'>
```

#### Event: 'update-target-url'

Returns:

* `event` Event
* `url` String

마우스나 키보드를 사용해 링크에 포커스할 때 발생하는 이벤트입니다.

#### Event: 'cursor-changed'

Returns:

* `event` Event
* `type` String
* `image` NativeImage (optional)
* `scale` Float (optional) - 커스텀 커서의 스케일링 수치
* `size` Object (optional) - `image`의 사이즈
  * `width` Integer
  * `height` Integer
* `hotspot` Object (optional) - 커스텀 커서의 핫스팟 좌표
  * `x` Integer - x 좌표
  * `y` Integer - y 좌표

커서 종류가 변경될 때 발생하는 이벤트입니다. `type` 인수는 다음 값이 될 수 있습니다:
`default`, `crosshair`, `pointer`, `text`, `wait`, `help`, `e-resize`, `n-resize`,
`ne-resize`, `nw-resize`, `s-resize`, `se-resize`, `sw-resize`, `w-resize`,
`ns-resize`, `ew-resize`, `nesw-resize`, `nwse-resize`, `col-resize`,
`row-resize`, `m-panning`, `e-panning`, `n-panning`, `ne-panning`, `nw-panning`,
`s-panning`, `se-panning`, `sw-panning`, `w-panning`, `move`, `vertical-text`,
`cell`, `context-menu`, `alias`, `progress`, `nodrop`, `copy`, `none`,
`not-allowed`, `zoom-in`, `zoom-out`, `grab`, `grabbing`, `custom`.

만약 `type` 인수가 `custom` 이고 `image` 인수가 `NativeImage`를 통한 커스텀
커서를 지정했을 때, 해당 이미지로 커서가 변경됩니다. 또한 `scale`, `size` 그리고 `hotspot` 인수는
커스텀 커서의 추가적인 정보를 포함합니다.

#### Event: 'context-menu'

Returns:

* `event` Event
* `params` Object
  * `x` Integer - x 좌표
  * `y` Integer - y 좌표
  * `linkURL` String - 컨텍스트 메뉴가 호출된 노드를 둘러싸는 링크의 URL.
  * `linkText` String - 링크에 연관된 텍스트. 콘텐츠의 링크가 이미지인 경우 빈
    문자열이 됩니다.
  * `pageURL` String - 컨텍스트 메뉴가 호출된 상위 수준 페이지의 URL.
  * `frameURL` String - 컨텍스트 메뉴가 호출된 서브 프레임의 URL.
  * `srcURL` String - 컨텍스트 메뉴가 호출된 요소에 대한 소스 URL. 요소와 소스 URL은
    이미지, 오디오, 비디오입니다.
  * `mediaType` String - 컨텍스트 메뉴가 호출된 노드의 종류. 값은 `none`, `image`,
    `audio`, `video`, `canvas`, `file` 또는 `plugin`이 될 수 있습니다.
  * `hasImageContents` Boolean - 컨텍스트 메뉴가 내용이 있는 이미지에서 호출되었는지
    여부.
  * `isEditable` Boolean - 컨텍스트를 편집할 수 있는지 여부.
  * `selectionText` String - 컨텍스트 메뉴가 호출된 부분에 있는 선택된 텍스트.
  * `titleText` String - 컨텍스트 메뉴가 호출된 선택된 제목 또는 알림 텍스트.
  * `misspelledWord` String - 만약 있는 경우, 커서가 가르키는 곳에서 발생한 오타.
  * `frameCharset` String - 메뉴가 호출된 프레임의 문자열 인코딩.
  * `inputFieldType` String - 컨텍스트 메뉴가 입력 필드에서 호출되었을 때, 그 필드의
    종류. 값은 `none`, `plainText`, `password`, `other` 중 한 가지가 될 수 있습니다.
  * `menuSourceType` String - 컨텍스트 메뉴를 호출한 입력 소스. 값은 `none`,
    `mouse`, `keyboard`, `touch`, `touchMenu` 중 한 가지가 될 수 있습니다.
  * `mediaFlags` Object - 컨텍스트 메뉴가 호출된 미디어 요소에 대한 플래그. 자세한
    사항은 아래를 참고하세요.
  * `editFlags` Object - 이 플래그는 렌더러가 어떤 행동을 이행할 수 있는지 여부를
    표시합니다. 자세한 사항은 아래를 참고하세요.

`mediaFlags`는 다음과 같은 속성을 가지고 있습니다:

* `inError` Boolean - 미디어 객체가 크래시되었는지 여부.
* `isPaused` Boolean - 미디어 객체가 일시중지되었는지 여부.
* `isMuted` Boolean - 미디어 객체가 음소거되었는지 여부.
* `hasAudio` Boolean - 미디어 객체가 오디오를 가지고 있는지 여부.
* `isLooping` Boolean - 미디어 객체가 루프중인지 여부.
* `isControlsVisible` Boolean - 미디어 객체의 컨트롤이 보이는지 여부.
* `canToggleControls` Boolean - 미디어 객체의 컨트롤을 토글할 수 있는지 여부.
* `canRotate` Boolean - 미디어 객체를 돌릴 수 있는지 여부.

`editFlags`는 다음과 같은 속성을 가지고 있습니다:

* `canUndo` Boolean - 렌더러에서 실행 취소할 수 있는지 여부.
* `canRedo` Boolean - 렌더러에서 다시 실행할 수 있는지 여부.
* `canCut` Boolean - 렌더러에서 잘라내기를 실행할 수 있는지 여부.
* `canCopy` Boolean - 렌더러에서 복사를 실행할 수 있는지 여부.
* `canPaste` Boolean - 렌더러에서 붙여넣기를 실행할 수 있는지 여부.
* `canDelete` Boolean - 렌더러에서 삭제를 실행할 수 있는지 여부.
* `canSelectAll` Boolean - 렌더러에서 모두 선택을 실행할 수 있는지 여부.

새로운 컨텍스트 메뉴의 제어가 필요할 때 발생하는 이벤트입니다.

#### Event: 'select-bluetooth-device'

Returns:

* `event` Event
* `devices` [Objects]
  * `deviceName` String
  * `deviceId` String
* `callback` Function
  * `deviceId` String

`navigator.bluetooth.requestDevice`의 호출에 의해 블루투스 기기가 선택되어야 할 때
발생하는 이벤트입니다. `navigator.bluetooth` API를 사용하려면 `webBluetooth`가
활성화되어 있어야 합니다. 만약 `event.preventDefault`이 호출되지 않으면, 첫 번째로
사용 가능한 기기가 선택됩니다. `callback`은 반드시 선택될 `deviceId`와 함께
호출되어야 하며, 빈 문자열을 `callback`에 보내면 요청이 취소됩니다.

```javascript
const {app, webContents} = require('electron')
app.commandLine.appendSwitch('enable-web-bluetooth')

app.on('ready', () => {
  webContents.on('select-bluetooth-device', (event, deviceList, callback) => {
    event.preventDefault()
    let result = deviceList.find((device) => {
      return device.deviceName === 'test'
    })
    if (!result) {
      callback('')
    } else {
      callback(result.deviceId)
    }
  })
})
```

#### Event: 'paint'

Returns:

* `event` Event
* `dirtyRect` Object
  * `x` Integer - 이미지의 x 좌표.
  * `y` Integer - 이미지의 y 좌표.
  * `width` Integer - Dirty 영역의 너비.
  * `height` Integer - Dirty 영역의 높이.
* `image` [NativeImage](native-image.md) - 전체 프레임의 이미지 데이터.

새 프레임이 생성되었을 때 발생하는 이벤트입니다. Dirty 영역만이 버퍼로 전달됩니다.

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({webPreferences: {offscreen: true}})
win.webContents.on('paint', (event, dirty, image) => {
  // updateBitmap(dirty, image.toBitmap())
})
win.loadURL('http://github.com')
```

### Instance Methods

#### `contents.loadURL(url[, options])`

* `url` URL
* `options` Object (optional)
  * `httpReferrer` String - HTTP 레퍼러 url.
  * `userAgent` String - 요청을 시작한 유저 에이전트.
  * `extraHeaders` String - "\n"로 구분된 Extra 헤더들.

윈도우에 웹 페이지 `url`을 로드합니다. `url`은 `http://`, `file://`과 같은
프로토콜 접두사를 가지고 있어야 합니다. 만약 반드시 http 캐시를 사용하지 않고 로드해야
하는 경우 `pragma` 헤더를 사용할 수 있습니다.

```javascript
const {webContents} = require('electron')
const options = {extraHeaders: 'pragma: no-cache\n'}
webContents.loadURL(url, options)
```

#### `contents.downloadURL(url)`

* `url` URL

`url`의 리소스를 탐색 없이 다운로드를 시작합니다. `session`의 `will-download`
이벤트가 발생합니다.

#### `contents.getURL()`

현재 웹 페이지의 URL을 반환합니다.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

let currentURL = win.webContents.getURL()
console.log(currentURL)
```

#### `contents.getTitle()`

현재 웹 페이지의 제목을 반환합니다.

#### `win.isDestroyed()`

윈도우가 소멸되었는지 여부를 반환합니다.

#### `contents.isFocused()`

웹 페이지가 포커스되어있는지 여부를 반환합니다.

#### `contents.isLoading()`

현재 웹 페이지가 리소스를 로드중인지 여부를 반환합니다.

#### `contents.isLoadingMainFrame()`

메인 프레임이 여전히 로딩중인지 여부를 반환합니다. (내부 iframe 또는 frame 포함)

#### `contents.isWaitingForResponse()`

현재 웹 페이지가 페이지의 메인 리소스로부터 첫 응답을 기다리고있는지 여부를 반환합니다.

#### `contents.stop()`

대기중인 탐색 작업을 모두 멈춥니다.

#### `contents.reload()`

현재 웹 페이지를 새로고침합니다.

#### `contents.reloadIgnoringCache()`

현재 웹 페이지의 캐시를 무시한 채로 새로고침합니다.

#### `contents.canGoBack()`

브라우저가 이전 웹 페이지로 돌아갈 수 있는지 여부를 반환합니다.

#### `contents.canGoForward()`

브라우저가 다음 웹 페이지로 이동할 수 있는지 여부를 반환합니다.

#### `contents.canGoToOffset(offset)`

* `offset` Integer

웹 페이지가 `offset`로 이동할 수 있는지 여부를 반환합니다.

#### `contents.clearHistory()`

탐색 기록을 삭제합니다.

#### `contents.goBack()`

브라우저가 이전 웹 페이지로 이동하게 합니다.

#### `contents.goForward()`

브라우저가 다음 웹 페이지로 이동하게 합니다.

#### `contents.goToIndex(index)`

* `index` Integer

브라우저가 지정된 절대 웹 페이지 인덱스로 탐색하게 합니다.

#### `contents.goToOffset(offset)`

* `offset` Integer

"current entry"에서 지정된 offset으로 탐색합니다.

#### `contents.isCrashed()`

렌더러 프로세스가 예기치 않게 종료되었는지 여부를 반환합니다.

#### `contents.setUserAgent(userAgent)`

* `userAgent` String

현재 웹 페이지의 유저 에이전트를 덮어씌웁니다.

#### `contents.getUserAgent()`

현재 웹 페이지의 유저 에이전트 문자열을 반환합니다.

#### `contents.insertCSS(css)`

* `css` String

CSS 코드를 현재 웹 페이지에 삽입합니다.

#### `contents.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean (optional)
* `callback` Function (optional) - 스크립트의 실행이 완료되면 호출됩니다.
  * `result`

페이지에서 자바스크립트 코드를 실행합니다.

기본적으로 `requestFullScreen`와 같은 몇몇 HTML API들은 사용자의 조작에 의해서만
호출될 수 있습니다. `userGesture`를 `true`로 설정하면 이러한 제약을 무시할 수
있습니다.

#### `contents.setAudioMuted(muted)`

* `muted` Boolean

현재 웹 페이지의 소리를 음소거합니다.

#### `contents.isAudioMuted()`

현재 페이지가 음소거 되어있는지 여부를 반환합니다.

#### `contents.setZoomFactor(factor)`

* `factor` Number - 줌 수치.

지정한 수치로 줌 수치를 변경합니다. 줌 수치는 100으로 나눈 값이며 300%는 3.0이 됩니다.

#### `contents.getZoomFactor(callback)`

* `callback` Function

현재 줌 수치 값을 요청합니다. `callback`은 `callback(zoomFactor)` 형태로 호출됩니다.

#### `contents.setZoomLevel(level)`

* `level` Number - 줌 레벨.

지정한 수준으로 줌 수준을 변경합니다. 원본 크기는 0이고 각 값의 증가와 감소는 현재 줌을
20% 크거나 작게 표현하고 각 크기는 원본 크기의 300%와 50%로 제한됩니다.

#### `contents.getZoomLevel(callback)`

* `callback` Function

현재 줌 수준 값을 요청합니다. `callback`은 `callback(zoomLevel)` 형태로 호출됩니다.

#### `contents.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

최대와 최소 값의 줌 수준 값을 지정합니다.

#### `contents.undo()`

웹 페이지에서 `undo` 편집 커맨드를 실행합니다.

#### `contents.redo()`

웹 페이지에서 `redo` 편집 커맨드를 실행합니다.

#### `contents.cut()`

웹 페이지에서 `cut` 편집 커맨드를 실행합니다.

#### `contents.copy()`

웹 페이지에서 `copy` 편집 커맨드를 실행합니다.

### `contents.copyImageAt(x, y)`

* `x` Integer
* `y` Integer

주어진 위치에 있는 이미지를 클립보드로 복사합니다.

#### `contents.paste()`

웹 페이지에서 `paste` 편집 커맨드를 실행합니다.

#### `contents.pasteAndMatchStyle()`

웹 페이지에서 `pasteAndMatchStyle` 편집 커맨드를 실행합니다.

#### `contents.delete()`

웹 페이지에서 `delete` 편집 커맨드를 실행합니다.

#### `contents.selectAll()`

웹 페이지에서 `selectAll` 편집 커맨드를 실행합니다.

#### `contents.unselect()`

웹 페이지에서 `unselect` 편집 커맨드를 실행합니다.

#### `contents.replace(text)`

* `text` String

웹 페이지에서 `replace` 편집 커맨드를 실행합니다.

#### `contents.replaceMisspelling(text)`

* `text` String

웹 페이지에서 `replaceMisspelling` 편집 커맨드를 실행합니다.

#### `contents.insertText(text)`

* `text` String

포커스된 요소에 `text`를 삽입합니다.

#### `contents.findInPage(text[, options])`

* `text` String - 찾을 콘텐츠, 반드시 공백이 아니여야 합니다.
* `options` Object (optional)
  * `forward` Boolean - 앞에서부터 검색할지 뒤에서부터 검색할지 여부입니다. 기본값은
    `true`입니다.
  * `findNext` Boolean - 작업을 계속 처리할지 첫 요청만 처리할지 여부입니다. 기본값은
    `false`입니다.
  * `matchCase` Boolean - 검색이 대소문자를 구분할지 여부입니다. 기본값은
    `false`입니다.
  * `wordStart` Boolean - 단어의 시작 부분만 볼 지 여부입니다. 기본값은
    `false`입니다.
  * `medialCapitalAsWordStart` Boolean - `wordStart`와 합쳐질 때, 소문자 또는
    비문자가 따라붙은 대문자로 일치가 시작하는 경우 단어 중간의 일치를 허용합니다.
    여러가지 다른 단어 내의 일치를 허용합니다. 기본값은 `false`입니다.

웹 페이지에서 `text`에 일치하는 모든 대상을 찾는 요청을 시작하고 요청에 사용된 요청을
표현하는 `정수(integer)`를 반환합니다. 요청의 결과는
[`found-in-page`](web-contents.md#event-found-in-page) 이벤트를 통해 취득할 수
있습니다.

#### `contents.stopFindInPage(action)`

* `action` String - [`webContents.findInPage`]
  요청이 종료되었을 때 일어날 수 있는 작업을 지정합니다.
  * `clearSelection` - 선택을 취소합니다.
  * `keepSelection` - 선택을 일반 선택으로 변경합니다.
  * `activateSelection` - 포커스한 후 선택된 노드를 클릭합니다.

제공된 `action`에 대한 `webContents`의 모든 `findInPage` 요청을 중지합니다.

```javascript
const {webContents} = require('electron')
webContents.on('found-in-page', (event, result) => {
  if (result.finalUpdate) webContents.stopFindInPage('clearSelection')
})

const requestId = webContents.findInPage('api')
console.log(requestId)
```

#### `contents.capturePage([rect, ]callback)`

* `rect` Object (optional) - 캡쳐할 페이지의 영역
  * `x` Integer
  * `y` Integer
  * `width` Integer
  * `height` Integer
* `callback` Function

페이지의 스크린샷을 `rect`에 설정한 만큼 캡처합니다. 캡처가 완료되면 `callback`이
`callback(image)` 형식으로 호출됩니다. `image`는 [NativeImage](native-image.md)의
인스턴스이며 스크린샷 데이터를 담고있습니다. `rect`를 생략하면 페이지 전체를 캡처합니다.

#### `contents.hasServiceWorker(callback)`

* `callback` Function

ServiceWorker가 등록되어있는지 확인하고 `callback`에 대한 응답으로 boolean 값을
반환합니다.

#### `contents.unregisterServiceWorker(callback)`

* `callback` Function

ServiceWorker가 존재하면 모두 등록을 해제하고 JS Promise가 만족될 때 `callback`에
대한 응답으로 boolean을 반환하거나 JS Promise가 만족되지 않을 때 `false`를 반환합니다.

#### `contents.print([options])`

`options` Object (optional)
  * `silent` Boolean - 사용자에게 프린트 설정을 묻지 않습니다. 기본값을 `false`입니다.
  * `printBackground` Boolean - 웹 페이지의 배경 색과 이미지를 출력합니다. 기본값은
  	`false`입니다.

윈도우의 웹 페이지를 프린트합니다. `silent`가 `true`로 지정되어있을 땐, Electron이
시스템의 기본 프린터와 기본 프린터 설정을 가져옵니다.

웹 페이지에서 `window.print()`를 호출하는 것은
`webContents.print({silent: false, printBackground: false})`를 호출하는 것과
같습니다.

#### `contents.printToPDF(options, callback)`

* `options` Object
  * `marginsType` Integer - 사용할 마진의 종류를 지정합니다. 0 부터 2 사이 값을 사용할
    수 있고 각각 기본 마진, 마진 없음, 최소 마진입니다.
  * `pageSize` String - 생성되는 PDF의 페이지 크기를 지정합니다. 값은 `A3`, `A4`,
    `A5`, `Legal`, `Letter`, `Tabloid` 또는 마이크론 단위의 `height` & `width`가
    포함된 객체를 사용할 수 있습니다.
  * `printBackground` Boolean - CSS 배경을 프린트할지 여부를 정합니다.
  * `printSelectionOnly` Boolean - 선택된 영역만 프린트할지 여부를 정합니다.
  * `landscape` Boolean - landscape을 위해선 `true`를, portrait를 위해선 `false`를
  	사용합니다.
* `callback` Function - `(error, data) => {}`

Chromium의 미리보기 프린팅 커스텀 설정을 이용하여 윈도우의 웹 페이지를 PDF로
프린트합니다.

`callback`은 작업이 완료되면 `callback(error, data)` 형식으로 호출됩니다. `data`는
생성된 PDF 데이터를 담고있는 `Buffer`입니다.

기본으로 비어있는 `options`은 다음과 같이 여겨지게 됩니다:

```javascript
{
  marginsType: 0,
  printBackground: false,
  printSelectionOnly: false,
  landscape: false
}
```

다음은 `webContents.printToPDF`의 예시입니다:

```javascript
const {BrowserWindow} = require('electron')
const fs = require('fs')

let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

win.webContents.on('did-finish-load', () => {
  // 기본 프린트 옵션을 사용합니다
  win.webContents.printToPDF({}, (error, data) => {
    if (error) throw error
    fs.writeFile('/tmp/print.pdf', data, (error) => {
      if (error) throw error
      console.log('Write PDF successfully.')
    })
  })
})
```

#### `contents.addWorkSpace(path)`

* `path` String

특정 경로를 개발자 도구의 워크스페이스에 추가합니다. 반드시 개발자 도구의 생성이 완료된
이후에 사용해야 합니다.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()
win.webContents.on('devtools-opened', () => {
  win.webContents.addWorkSpace(__dirname)
})
```

#### `contents.removeWorkSpace(path)`

* `path` String

특정 경로를 개발자 도구의 워크스페이스에서 제거합니다.

#### `contents.openDevTools([options])`

* `options` Object (optional)
  * `detach` Boolean - 새 창에서 개발자 도구를 엽니다.
  * `mode` String - 개발자 도구 표시 상태를 지정합니다. 옵션은 "right", "bottom",
    "undocked", "detach"가 될 수 있습니다. 기본값은 마지막 표시 상태를
    사용합니다. `undocked` 모드에선 다시 도킹할 수 있습니다. 하지만 `detach`
    모드에선 할 수 없습니다.

개발자 도구를 엽니다.

#### `contents.closeDevTools()`

개발자 도구를 닫습니다.

#### `contents.isDevToolsOpened()`

개발자 도구가 열려있는지 여부를 반환합니다.

#### `contents.isDevToolsFocused()`

개발자 도구에 포커스 되어있는지 여부를 반환합니다.

#### `contents.toggleDevTools()`

개발자 도구를 토글합니다.

#### `contents.inspectElement(x, y)`

* `x` Integer
* `y` Integer

(`x`, `y`)위치의 요소를 조사합니다.

#### `contents.inspectServiceWorker()`

서비스 워커 컨텍스트(service worker context)를 위한 개발자 도구를 엽니다.

#### `contents.send(channel[, arg1][, arg2][, ...])`

* `channel` String

`channel`을 통하여 렌더러 프로세스에 비동기 메시지를 보냅니다. 임의의 인수를 보낼수도
있습니다. 인수들은 내부적으로 JSON 포맷으로 직렬화 되며, 이후 함수와 프로토타입 체인은
포함되지 않게 됩니다.

렌더러 프로세스는 `ipcRenderer` 모듈을 통하여 `channel`를 리스닝하여 메시지를 처리할
수 있습니다.

메인 프로세스에서 렌더러 프로세스로 메시지를 보내는 예시 입니다:

```javascript
// In the main process.
const {app, BrowserWindow} = require('electron')
let win = null
app.on('ready', () => {
  win = new BrowserWindow({width: 800, height: 600})
  win.loadURL(`file://${__dirname}/index.html`)
  win.webContents.on('did-finish-load', () => {
    win.webContents.send('ping', 'whoooooooh!')
  })
})
```

```html
<!-- index.html -->
<html>
<body>
  <script>
    require('electron').ipcRenderer.on('ping', (event, message) => {
      console.log(message)  // "whoooooooh!" 출력
    });
  </script>
</body>
</html>
```

#### `contents.enableDeviceEmulation(parameters)`

* `parameters` Object
  * `screenPosition` String - 에뮬레이트 할 화면 종료를 지정합니다
      (기본값: `desktop`)
    * `desktop` String - Desktop screen type
    * `mobile` String - Mobile screen type
  * `screenSize` Object - 에뮬레이트 화면의 크기를 지정합니다 (screenPosition ==
    mobile)
    * `width` Integer - 에뮬레이트 화면의 너비를 지정합니다
    * `height` Integer - 에뮬레이트 화면의 높이를 지정합니다
  * `viewPosition` Object - 화면에서 뷰의 위치 (screenPosition == mobile) (기본값:
    `{x: 0, y: 0}`)
    * `x` Integer - 좌상단 모서리로부터의 x 축의 오프셋
    * `y` Integer - 좌상단 모서리로부터의 y 축의 오프셋
  * `deviceScaleFactor` Float - 디바이스의 스케일 팩터(scale factor)를 지정합니다.
  	(0일 경우 기본 디바이스 스케일 팩터를 기본으로 사용합니다. 기본값: `0`)
  * `viewSize` Object - 에뮬레이트 된 뷰의 크기를 지정합니다 (빈 값은 덮어쓰지 않는
    다는 것을 의미합니다)
    * `width` Integer - 에뮬레이트 된 뷰의 너비를 지정합니다
    * `height` Integer - 에뮬레이트 된 뷰의 높이를 지정합니다
  * `fitToView` Boolean - 에뮬레이트의 뷰가 사용 가능한 공간에 맞추어 스케일 다운될지를
  		지정합니다 (기본값: `false`)
  * `offset` Object - 사용 가능한 공간에서 에뮬레이트 된 뷰의 오프셋을 지정합니다 (fit
    to view 모드 외에서) (기본값: `{x: 0, y: 0}`)
    * `x` Float - 좌상단 모서리에서 x 축의 오프셋을 지정합니다
    * `y` Float - 좌상단 모서리에서 y 축의 오프셋을 지정합니다
  * `scale` Float - 사용 가능한 공간에서 에뮬레이드 된 뷰의 스케일 (fit to view 모드
    외에서, 기본값: `1`)

`parameters`로 디바이스 에뮬레이션을 사용합니다.

#### `contents.disableDeviceEmulation()`

`webContents.enableDeviceEmulation`로 활성화된 디바이스 에뮬레이선을 비활성화 합니다.

#### `contents.sendInputEvent(event)`

* `event` Object
  * `type` String (**required**) - 이벤트의 종류. 다음 값들을 사용할 수 있습니다:
    `mouseDown`,     `mouseUp`, `mouseEnter`, `mouseLeave`, `contextMenu`,
    `mouseWheel`, `mouseMove`, `keyDown`, `keyUp`, `char`.
  * `modifiers` String[] - 이벤트의 수정자(modifier)들에 대한 배열. 다음 값들을 포함
    할 수 있습니다: `shift`, `control`, `alt`, `meta`, `isKeypad`, `isAutoRepeat`,
    `leftButtonDown`, `middleButtonDown`, `rightButtonDown`, `capsLock`,
    `numLock`, `left`, `right`.

Input `event`를 웹 페이지로 전송합니다.

키보드 이벤트들에 대해서는 `event` 객체는 다음 속성들을 사용할 수 있습니다:

* `keyCode` String (**required**) - 키보드 이벤트가 발생할 때 보내질 문자.
  [Accelerator](accelerator.md)의 올바른 키 코드만 사용해야 합니다.

마우스 이벤트들에 대해서는 `event` 객체는 다음 속성들을 사용할 수 있습니다:

* `x` Integer (**required**)
* `y` Integer (**required**)
* `button` String - 눌린 버튼. 다음 값들이 가능합니다. `left`, `middle`, `right`
* `globalX` Integer
* `globalY` Integer
* `movementX` Integer
* `movementY` Integer
* `clickCount` Integer

`mouseWheel` 이벤트에 대해서는 `event` 객체는 다음 속성들을 사용할 수 있습니다:

* `deltaX` Integer
* `deltaY` Integer
* `wheelTicksX` Integer
* `wheelTicksY` Integer
* `accelerationRatioX` Integer
* `accelerationRatioY` Integer
* `hasPreciseScrollingDeltas` Boolean
* `canScroll` Boolean

#### `contents.beginFrameSubscription([onlyDirty ,]callback)`

* `onlyDirty` Boolean (optional) - 기본값은 `false`입니다.
* `callback` Function

캡처된 프레임과 프레젠테이션 이벤트를 구독하기 시작합니다. `callback`은
프레젠테이션 이벤트가 발생했을 때 `callback(frameBuffer, dirtyRect)` 형태로
호출됩니다.

`frameBuffer`는 raw 픽셀 데이터를 가지고 있는 `Buffer` 객체입니다. 많은 장치에서
32비트 BGRA 포맷을 사용하여 효율적으로 픽셀 데이터를 저장합니다. 하지만 실질적인
데이터 저장 방식은 프로세서의 엔디안 방식에 따라서 달라집니다. (따라서 현대의 많은
프로세서에선 little-endian 방식을 사용하므로 위의 포맷을 그대로 표현합니다. 하지만
몇몇 프로세서는 big-endian 방식을 사용하는데, 이 경우 32비트 ARGB 포맷을 사용합니다)

`dirtyRect`는 페이지의 어떤 부분이 다시 그려졌는지를 표현하는 `x, y, width, height`
속성을 포함하는 객체입니다. 만약 `onlyDirty`가 `true`로 지정되어 있으면,
`frameBuffer`가 다시 그려진 부분만 포함합니다. `onlyDirty`의 기본값은 `false`입니다.

#### `contents.endFrameSubscription()`

프레임 프레젠테이션 이벤트들에 대한 구독을 중지합니다.

#### `contents.startDrag(item)`

* `item` object
  * `file` String
  * `icon` [NativeImage](native-image.md)

현재 진행중인 드래그-드롭에 `item`을 드래그 중인 아이템으로 설정합니다. `file`은
드래그될 파일의 절대 경로입니다. 그리고 `icon`은 드래그 도중 커서 밑에 표시될
이미지입니다.

#### `contents.savePage(fullPath, saveType, callback)`

* `fullPath` String - 전체 파일 경로.
* `saveType` String - 저장 종류를 지정합니다.
  * `HTMLOnly` - 페이지의 HTML만 저장합니다.
  * `HTMLComplete` - 페이지의 완성된 HTML을 저장합니다.
  * `MHTML` - 페이지의 완성된 HTML을 MHTML로 저장합니다.
* `callback` Function - `(error) => {}`.
  * `error` Error

만약 페이지를 저장하는 프로세스가 성공적으로 끝났을 경우 true를 반환합니다.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

win.loadURL('https://github.com')

win.webContents.on('did-finish-load', () => {
  win.webContents.savePage('/tmp/test.html', 'HTMLComplete', (error) => {
    if (!error) console.log('Save page successfully')
  })
})
```

#### `contents.showDefinitionForSelection()` _macOS_

페이지에서 선택된 단어에 대한 사전 검색 결과 팝업을 표시합니다.

#### `contents.isOffscreen()`

*오프 스크린 렌더링* 이 활성화되었는지 여부를 표시합니다.

#### `contents.startPainting()`

*오프 스크린 렌더링* 이 활성화되었고 페인팅 상태가 아니라면 페인팅을 시작합니다.

#### `contents.stopPainting()`

*오프 스크린 렌더링* 이 활성화되었고 페인팅 상태라면 페인팅을 중지합니다.

#### `contents.isPainting()`

*오프 스크린 렌더링* 이 활성화된 경우 현재 패인팅 상태를 반환합니다.

#### `contents.setFrameRate(fps)`

* `fps` Integer

*오프 스크린 렌더링* 이 활성화된 경우 프레임 레이트를 지정한 숫자로 지정합니다. 1과 60
사이의 값만 사용할 수 있습니다.

#### `contents.getFrameRate()`

*오프 스크린 렌더링* 이 활성화된 경우 현재 프레임 레이트를 반환합니다.

#### `contents.invalidate()`

*오프 스크린 렌더링* 이 활성화된 경우 프레임을 무효화 하고 `'paint'` 이벤트를
통해 새로 만듭니다.

### Instance Properties

#### `contents.id`

이 WebContents의 유일 ID.

#### `contents.session`

이 webContents에서 사용하는 [session](session.md) 객체를 반환합니다.

#### `contents.hostWebContents`

현재 `WebContents`를 소유하는 `WebContents`를 반환합니다.

#### `contents.devToolsWebContents`

이 `WebContents`에 대한 개발자 도구의 `WebContents`를 가져옵니다.

**참고:** 사용자가 절대로 이 객체를 저장해서는 안 됩니다. 개발자 도구가 닫혔을 때,
`null`이 반환될 수 있습니다.

#### `contents.debugger`

현재 `webContents`에 대한 디버거 인스턴스를 가져옵니다.

## Class: Debugger

> Chrome의 원격 디버깅 프로토콜에 대한 대체 접근자입니다.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

try {
  win.webContents.debugger.attach('1.1')
} catch (err) {
  console.log('Debugger attach failed : ', err)
}

win.webContents.debugger.on('detach', (event, reason) => {
  console.log('Debugger detached due to : ', reason)
})

win.webContents.debugger.on('message', (event, method, params) => {
  if (method === 'Network.requestWillBeSent') {
    if (params.request.url === 'https://www.github.com') {
      win.webContents.debugger.detach()
    }
  }
})

win.webContents.debugger.sendCommand('Network.enable')
```

### Instance Methods

#### `debugger.attach([protocolVersion])`

* `protocolVersion` String (optional) - 요청할 디버깅 프로토콜의 버전.

`webContents`에 디버거를 부착합니다.

#### `debugger.isAttached()`

디버거가 `webContents`에 부착되어 있는지 여부를 반환합니다.

#### `debugger.detach()`

`webContents`로부터 디버거를 분리시킵니다.

#### `debugger.sendCommand(method[, commandParams, callback])`

* `method` String - 메서드 이름, 반드시 원격 디버깅 프로토콜에 의해 정의된 메서드중
  하나가 됩니다.
* `commandParams` Object (optional) - 요청 인수를 표현한 JSON 객체.
* `callback` Function (optional) - 응답
  * `error` Object -  커맨드의 실패를 표시하는 에러 메시지.
  * `result` Object - 원격 디버깅 프로토콜에서 커맨드 설명의 'returns' 속성에 의해
    정의된 응답

지정한 커맨드를 디버깅 대상에게 전송합니다.

### Instance Events

#### Event: 'detach'

* `event` Event
* `reason` String - 디버거 분리 사유.

디버깅 세션이 종료될 때 발생하는 이벤트입니다. `webContents`가 닫히거나 개발자 도구가
부착된 `webContents`에 대해 호출될 때 발생합니다.

#### Event: 'message'

* `event` Event
* `method` String - 메서드 이름.
* `params` Object - 원격 디버깅 프로토콜의 'parameters' 속성에서 정의된 이벤트 인수

디버깅 타겟이 관련 이벤트를 발생시킬 때 마다 발생하는 이벤트입니다.

[rdp]: https://developer.chrome.com/devtools/docs/debugger-protocol
[`webContents.findInPage`]: web-contents.md#contentsfindinpagetext-options
