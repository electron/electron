# `<webview>` 태그

`guest` 컨텐츠(웹 페이지)를 Electron 앱 페이지에 삽입하기 위해 `webview` 태그를 사용할 수 있습니다.
게스트 컨텐츠는 `webview` 컨테이너에 담겨 대상 페이지에 삽입되고 해당 페이지에선 게스트 컨텐츠의 배치 및 렌더링 과정을 조작할 수 있습니다.

`iframe`과 `webview`의 차이는 어플리케이션과 프로세스가 분리되어 돌아간다는 점입니다.
그것은 모든 권한이 웹 페이지와 같지 않고 모든 앱과 임베디드(게스트) 컨텐츠간의 상호작용이 비동기로 작동한다는 것을 의미합니다.
이에 따라 임베디드 컨텐츠로부터 어플리케이션을 안전하게 유지할 수 있습니다.

## 예제

웹 페이지를 어플리케이션에 삽입하려면 `webview` 태그를 사용해 원하는 타겟 페이지에 추가하면 됩니다. (게스트 컨텐츠가 앱 페이지에 추가 됩니다)
간단한 예로 `webview` 태그의 `src` 속성에 페이지를 지정하고 css 스타일을 이용해서 컨테이너의 외관을 설정할 수 있습니다:

```html
<webview id="foo" src="https://www.github.com/" style="display:inline-block; width:640px; height:480px"></webview>
```

게스트 컨텐츠를 조작하기 위해 자바스크립트로 `webview` 태그의 이벤트를 리스닝 하여 응답을 받을 수 있습니다.
다음 예제를 참고하세요: 첫번째 리스너는 페이지 로딩 시작시의 이벤트를 확인하고 두번째 리스너는 페이지의 로딩이 끝난시점을 확인합니다.
그리고 페이지를 로드하는 동안 "loading..." 메시지를 표시합니다.

```html
<script>
  onload = function() {
    var webview = document.getElementById("foo");
    var indicator = document.querySelector(".indicator");

    var loadstart = function() {
      indicator.innerText = "loading...";
    }
    var loadstop = function() {
      indicator.innerText = "";
    }
    webview.addEventListener("did-start-loading", loadstart);
    webview.addEventListener("did-stop-loading", loadstop);
  }
</script>
```

## 태그 속성

### src

```html
<webview src="https://www.github.com/"></webview>
```

지정한 URL을 페이지 소스로 사용합니다. 이 속성을 지정할 경우 `webview`의 최상위 페이지가 됩니다.

`src`에 같은 페이지를 지정하면 페이지를 새로고침합니다.

`src` 속성은 `data:text/plain,Hello, world!` 같은 data URL도 사용할 수 있습니다.

### autosize

```html
<webview src="https://www.github.com/" autosize="on" minwidth="576" minheight="432"></webview>
```

"on" 으로 지정하면 `webview` 컨테이너는 `minwidth`, `minheight`, `maxwidth`, `maxheight`에 맞춰서 자동으로 크기를 조절합니다.
이 조건은 `autosize`가 활성화되어있지 않는 한 따로 영향을 주지 않습니다.
`autosize`가 활성화 되어있으면 `webview` 컨테이너의 크기는 각각의 지정한 최대, 최소값에 따라 조절됩니다.

### nodeintegration

```html
<webview src="http://www.google.com/" nodeintegration></webview>
```

"on"으로 지정하면 `webview` 페이지 내에서 `require`와 `process 객체`같은 node.js API를 사용할 수 있습니다.
이를 지정하면 내부에서 로우레벨 리소스에 접근할 수 있습니다.

### plugins

```html
<webview src="https://www.github.com/" plugins></webview>
```

"on"으로 지정하면 `webview` 내부에서 브라우저 플러그인을 사용할 수 있습니다.

### preload

```html
<webview src="https://www.github.com/" preload="./test.js"></webview>
```

게스트 페이지가 로드되기 전에 실행할 스크립트를 지정합니다.
스크립트 URL은 `file:` 또는 `asar:` 프로토콜 중 하나를 반드시 사용해야 합니다.
왜냐하면 `require`를 사용해 게스트 페이지 내에서 스크립트를 로드하기 때문입니다.

게스트 페이지가 nodeintegration을 활성화 하지 않았어도 지정된 스크립트는 정상적으로 돌아갑니다.
하지만 스크립트 내에서 사용할 수 있는 global 객체는 스크립트 작동이 끝나면 삭제됩니다.

### httpreferrer

```html
<webview src="https://www.github.com/" httpreferrer="http://cheng.guru"></webview>
```

게스트 페이지의 referrer URL을 설정합니다.

### useragent

```html
<webview src="https://www.github.com/" useragent="Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko"></webview>
```

게스트 페이지의 `User-Agent`를 설정합니다. 페이지가 로드된 후엔 `setUserAgent` 메소드를 사용해서 변경할 수 있습니다.

### disablewebsecurity

```html
<webview src="https://www.github.com/" disablewebsecurity></webview>
```

"on"으로 지정하면 게스트 페이지의 웹 보안을 해제합니다.

## API

webview 메서드는 페이지 로드가 끝난 뒤에만 사용할 수 있습니다.

**예제**
```javascript
webview.addEventListener("dom-ready", function() {
  webview.openDevTools();
});
```

### `<webview>`.getUrl()

게스트 페이지의 URL을 반환합니다.

### `<webview>`.getTitle()

게스트 페이지의 제목을 반환합니다.

### `<webview>`.isLoading()

페이지가 아직 리소스를 로딩하고 있는지 확인합니다.

### `<webview>`.isWaitingForResponse()

게스트 페이지가 메인 리소스의 첫 응답을 기다리고 있는지 확인합니다.

### `<webview>`.stop()

모든 탐색을 취소합니다.

### `<webview>`.reload()

페이지를 새로고침합니다.

### `<webview>`.reloadIgnoringCache()

캐시를 무시하고 페이지를 새로고침합니다.

### `<webview>`.canGoBack()

페이지 히스토리를 한 칸 뒤로 가기를 할 수 있는지 확인합니다.

### `<webview>`.canGoForward()

페이지 히스토리를 한 칸 앞으로 가기를 할 수 있는지 확인합니다.

### `<webview>`.canGoToOffset(offset)

* `offset` Integer

페이지 히스토리를 `offset` 만큼 이동할 수 있는지 확인합니다.

### `<webview>`.clearHistory()

탐색 히스토리를 비웁니다.

### `<webview>`.goBack()

페이지 뒤로 가기를 실행합니다.

### `<webview>`.goForward()

페이지 앞으로 가기를 실행합니다.

### `<webview>`.goToIndex(index)

* `index` Integer

페이지를 지정한 `index`로 이동합니다.

### `<webview>`.goToOffset(offset)

* `offset` Integer

현재 페이지로 부터 `offset` 만큼 이동합니다.

### `<webview>`.isCrashed()

랜더러 프로세스가 크래시 됬는지 확인합니다.

### `<webview>`.setUserAgent(userAgent)

* `userAgent` String

`User-Agent`를 지정합니다.

### `<webview>`.getUserAgent()

현재 페이지의 `User-Agent` 문자열을 가져옵니다.

### `<webview>`.insertCSS(css)

* `css` String

게스트 페이지에 CSS를 삽입합니다.

### `<webview>`.executeJavaScript(code[, userGesture])

* `code` String
* `userGesture` Boolean

게스트 페이지에서 자바스크립트 `code`를 실행합니다.

`userGesture`가 `true`로 설정되어 있으면 `requestFullScreen` HTML API 같이
유저의 승인이 필요한 API를 유저의 승인을 무시하고 개발자가 API를 직접 사용할 수 있습니다.

역주: 기본적으로 브라우저에선 전체화면, 웹캠, 파일 열기등의 API를 사용하려면 유저의 승인(이벤트)이 필요합니다.

### `<webview>`.openDevTools()

게스트 페이지에 대한 개발자 툴을 엽니다.

### `<webview>`.closeDevTools()

게스트 페이지에 대한 개발자 툴을 닫습니다.

### `<webview>`.isDevToolsOpened()

게스트 페이지에 대한 개발자 툴이 열려있는지 확인합니다.

### `<webview>`.inspectElement(x, y)

* `x` Integer
* `y` Integer

(`x`, `y`) 위치에 있는 엘리먼트를 inspect합니다.

### `<webview>`.inspectServiceWorker()

Service worker에 대한 개발자 툴을 엽니다.

### `<webview>`.undo()

페이지에서 실행 취소 커맨드를 실행합니다.

### `<webview>`.redo()

페이지에서 다시 실행 커맨드를 실행합니다.

### `<webview>`.cut()

페이지에서 잘라내기 커맨드를 실행합니다.

### `<webview>`.copy()

페이지에서 복사 커맨드를 실행합니다.

### `<webview>`.paste()

페이지에서 붙여넣기 커맨드를 실행합니다.

### `<webview>`.pasteAndMatchStyle()

페이지에서 `pasteAndMatchStyle` 편집 커맨드를 실행합니다.

### `<webview>`.delete()

페이지에서 삭제 커맨드를 실행합니다.

### `<webview>`.selectAll()

페이지에서 전체 선택 커맨드를 실행합니다.

### `<webview>`.unselect()

페이지에서 `unselect` 커맨드를 실행합니다.

### `<webview>`.replace(text)

* `text` String

페이지에서 `replace` 커맨드를 실행합니다.

### `<webview>`.replaceMisspelling(text)

* `text` String

페이지에서 `replaceMisspelling` 커맨드를 실행합니다.

### `<webview>`.print([options])

Webview 페이지를 인쇄합니다. `webContents.print([options])` 메서드와 같습니다.

### `<webview>`.printToPDF(options, callback)

Webview 페이지를 PDF 형식으로 인쇄합니다. `webContents.printToPDF(options, callback)` 메서드와 같습니다.

### `<webview>`.send(channel[, args...])

* `channel` String

`channel`을 통해 게스트 페이지에 `args...` 비동기 메시지를 보냅니다.
게스트 페이지에선 `ipc` 모듈의 `channel` 이벤트를 사용하면 이 메시지를 받을 수 있습니다.

예제는 [WebContents.send](browser-window-ko.md#webcontentssendchannel-args)를 참고하세요.

## DOM 이벤트

### load-commit

* `url` String
* `isMainFrame` Boolean

Fired when a load has committed. This includes navigation within the current
document as well as subframe document-level loads, but does not include
asynchronous resource loads.

### did-finish-load

탐색이 끝나면 발생하는 이벤트입니다. 브라우저 탭의 스피너가 멈추고 `onload` 이벤트가 발생될 때를 생각하면 됩니다.

### did-fail-load

* `errorCode` Integer
* `errorDescription` String

`did-finish-load`와 비슷합니다. 하지만 이 이벤트는 `window.stop()`과 같은 무언가로 인해 로드에 실패했을 때 발생하는 이벤트입니다.

### did-frame-finish-load

* `isMainFrame` Boolean

프레임의 탐색이 끝나면 발생하는 이벤트입니다.

### did-start-loading

브라우저 탭의 스피너가 돌기 시작할 때 처럼 페이지의 로드가 시작될 때 발생하는 이벤트입니다.

### did-stop-loading

브라우저 탭의 스피너가 멈출 때 처럼 페이지의 로드가 끝나면 발생하는 이벤트입니다.

### did-get-response-details

* `status` Boolean
* `newUrl` String
* `originalUrl` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object

요청한 리소스에 관해 자세한 내용을 알 수 있을 때 발생하는 이벤트입니다.
`status`는 리소스를 다운로드할 소켓 커낵션을 나타냅니다.

### did-get-redirect-request

* `oldUrl` String
* `newUrl` String
* `isMainFrame` Boolean

리소스를 요청하고 받는 도중에 리다이렉트가 생기면 발생하는 이벤트입니다.

### dom-ready

현재 프레임 문서의 로드가 끝나면 발생하는 이벤트입니다.

### page-title-set

* `title` String
* `explicitSet` Boolean

탐색하는 동안에 페이지의 제목이 설정되면 발생하는 이벤트입니다. `explicitSet`는 파일 URL에서 종합(synthesised)된 제목인 경우 false로 표시됩니다.

### page-favicon-updated

* `favicons` Array - Array of Urls

페이지가 favicon URL을 받았을 때 발생하는 이벤트입니다.

### enter-html-full-screen

페이지가 HTML API에 의해 전체 화면 모드에 돌입했을 때 발생하는 이벤트입니다.

### leave-html-full-screen

페이지의 전체 화면 모드가 해제됬을 때 발생하는 이벤트입니다.

### console-message

* `level` Integer
* `message` String
* `line` Integer
* `sourceId` String

`console.log` API에 의해 로깅될 때 발생하는 이벤트입니다.

다음 예제는 모든 로그 메시지를 로그 레벨이나 다른 속성에 관련 없이 호스트 페이지의 콘솔에 다시 로깅하는 예제입니다.

```javascript
webview.addEventListener('console-message', function(e) {
  console.log('Guest page logged a message:', e.message);
});
```

### new-window

* `url` String
* `frameName` String
* `disposition` String - Can be `default`, `foreground-tab`, `background-tab`,
  `new-window` and `other`

게스트 페이지가 새로운 브라우저 창을 생성할 때 발생하는 이벤트입니다.

다음 예제 코드는 새 URL을 시스템의 기본 브라우저로 여는 코드입니다.

```javascript
webview.addEventListener('new-window', function(e) {
  require('shell').openExternal(e.url);
});
```

### close

게스트 페이지가 자체적으로 닫힐 때 발생하는 이벤트입니다.

다음 예제 코드는 게스트 페이지가 자체적으로 닫힐 때 `webview`를 `about:blank` 페이지로 이동시키는 예제입니다.

```javascript
webview.addEventListener('close', function() {
  webview.src = 'about:blank';
});
```

### ipc-message

* `channel` String
* `args` Array

호스트 페이지에서 비동기 IPC 메시지를 보낼 때 발생하는 이벤트입니다.

`sendToHost` 메소드와 `ipc-message` 이벤트로 호스트 페이지와 쉽게 통신을 할 수 있습니다:

```javascript
// In embedder page.
webview.addEventListener('ipc-message', function(event) {
  console.log(event.channel);
  // Prints "pong"
});
webview.send('ping');
```

```javascript
// In guest page.
var ipc = require('ipc');
ipc.on('ping', function() {
  ipc.sendToHost('pong');
});
```

### crashed

랜더러 프로세스가 크래시 되었을 때 발생하는 이벤트입니다.

### gpu-crashed

GPU 프로세스가 크래시 되었을 때 발생하는 이벤트입니다.

### plugin-crashed

* `name` String
* `version` String

플러그인 프로세스가 크래시 되었을 때 발생하는 이벤트입니다.

### destroyed

WebContents가 파괴될 때 발생하는 이벤트입니다.
