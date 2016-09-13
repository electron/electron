# `<webview>` 태그

> 외부 웹 콘텐츠를 고립된 프레임과 프로세스에서 표시합니다.

`guest` 콘텐츠(웹 페이지)를 Electron 앱 페이지에 삽입하기 위해 `webview` 태그를
사용할 수 있습니다. 게스트 콘텐츠는 `webview` 컨테이너에 담겨 대상 페이지에 삽입되고
해당 페이지에선 게스트 콘텐츠의 배치 및 렌더링 과정을 조작할 수 있습니다.

`iframe`과는 달리 `webview`는 애플리케이션과 분리된 프로세스에서 작동합니다.
이는 웹 페이지와 같은 권한을 가지지 않고 앱과 임베디드(게스트) 콘텐츠간의 모든
상호작용이 비동기로 작동한다는 것을 의미합니다. 따라서 임베디드 콘텐츠로부터
애플리케이션을 안전하게 유지할 수 있습니다.

보안상의 이유로, `webview`는 `nodeIntegration`이 활성화된 `BrowserWindow`에서만 사용할 수 있습니다.

## 예시

웹 페이지를 애플리케이션에 삽입하려면 `webview` 태그를 사용해 원하는 타겟 페이지에
추가하면 됩니다. (게스트 콘텐츠가 앱 페이지에 추가 됩니다) 간단한 예로 `webview`
태그의 `src` 속성에 페이지를 지정하고 css 스타일을 이용해서 컨테이너의 외관을 설정할
수 있습니다:

```html
<webview id="foo" src="https://www.github.com/" style="display:inline-flex; width:640px; height:480px"></webview>
```

게스트 콘텐츠를 조작하기 위해 자바스크립트로 `webview` 태그의 이벤트를 리스닝 하여
응답을 받을 수 있습니다. 다음 예시를 참고하세요: 첫번째 리스너는 페이지 로딩 시작시의
이벤트를 확인하고 두번째 리스너는 페이지의 로딩이 끝난시점을 확인합니다. 그리고
페이지를 로드하는 동안 "loading..." 메시지를 표시합니다.

```html
<script>
  onload = () => {
    const webview = document.getElementById('foo');
    const indicator = document.querySelector('.indicator');

    const loadstart = () => {
      indicator.innerText = 'loading...';
    };

    const loadstop = () => {
      indicator.innerText = '';
    };

    webview.addEventListener('did-start-loading', loadstart);
    webview.addEventListener('did-stop-loading', loadstop);
  };
</script>
```

## CSS 스타일링 참고

주의할 점은 `webview` 태그의 스타일은 전통적인 flexbox 레이아웃을 사용했을 때 자식
`object` 요소가 해당 `webview` 컨테이너의 전체 높이와 넓이를 확실히 채우도록
내부적으로 `display:flex;`를 사용합니다. (v0.36.11 부터) 따라서 인라인 레이아웃을
위해 `display:inline-flex;`를 쓰지 않는 한, 기본 `display:flex;` CSS 속성을
덮어쓰지 않도록 주의해야 합니다.

`webview`는 `hidden` 또는 `display: none;` 속성을 사용할 때 발생하는 문제를 한 가지
가지고 있습니다. 자식 `browserplugin` 객체 내에서 비정상적인 랜더링 동작을 발생시킬 수
있으며 웹 페이지가 로드되었을 때, `webview`가 숨겨지지 않았을 때, 반대로 그냥 바로
다시 보이게 됩니다. `webview`를 숨기는 방법으로 가장 권장되는 방법은 `width` &
`height`를 0으로 지정하는 CSS를 사용하는 것이며 `flex`를 통해 0px로 요소를 수축할 수
있도록 합니다.

```html
<style>
  webview {
    display:inline-flex;
    width:640px;
    height:480px;
  }
  webview.hide {
    flex: 0 1;
    width: 0px;
    height: 0px;
  }
</style>
```

## 태그 속성

`webview` 태그는 다음과 같은 속성을 가지고 있습니다:

### `src`

```html
<webview src="https://www.github.com/"></webview>
```

지정한 URL을 페이지 소스로 사용합니다. 이 속성을 지정할 경우 `webview`의 최상위
페이지가 됩니다.

`src`에 같은 페이지를 지정하면 페이지를 새로고침합니다.

`src` 속성은 `data:text/plain,Hello, world!` 같은 data URL도 사용할 수 있습니다.

### `autosize`

```html
<webview src="https://www.github.com/" autosize="on" minwidth="576" minheight="432"></webview>
```

"on" 으로 지정하면 `webview` 컨테이너는 `minwidth`, `minheight`, `maxwidth`,
`maxheight`에 맞춰서 자동으로 크기를 조절합니다. 이 속성들은 `autosize`가
활성화되어있지 않는 한 프레임에 영향을 주지 않습니다. `autosize`가 활성화 되어있으면
`webview` 컨테이너의 크기는 각각의 지정한 최대, 최소값에 따라 조절됩니다.

### `nodeintegration`

```html
<webview src="http://www.google.com/" nodeintegration></webview>
```

"on"으로 지정하면 `webview` 페이지 내에서 `require`와 `process 객체`같은 node.js
API를 사용할 수 있습니다. 이를 지정하면 내부에서 로우레벨 리소스에 접근할 수 있습니다.

**참고:** Node 통합 기능은 `webview`에서 부모 윈도우가 해당 옵션이 비활성화되어있는
경우 항상 비활성화됩니다.

### `plugins`

```html
<webview src="https://www.github.com/" plugins></webview>
```

"on"으로 지정하면 `webview` 내부에서 브라우저 플러그인을 사용할 수 있습니다.

### `preload`

```html
<webview src="https://www.github.com/" preload="./test.js"></webview>
```

페이지가 로드되기 전에 실행할 스크립트를 지정합니다. 스크립트 URL은 `file:` 또는
`asar:` 프로토콜 중 하나를 반드시 사용해야 합니다. 왜냐하면 페이지 내에서 `require`를
사용하여 스크립트를 로드하기 때문입니다.

페이지가 nodeintegration을 활성화 하지 않아도 지정한 스크립트는 정상적으로 작동합니다.
하지만 스크립트 내에서 사용할 수 있는 global 객체는 스크립트 작동이 끝나면 삭제됩니다.

### `httpreferrer`

```html
<webview src="https://www.github.com/" httpreferrer="http://cheng.guru"></webview>
```

페이지의 referrer URL을 설정합니다.

### `useragent`

```html
<webview src="https://www.github.com/" useragent="Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; AS; rv:11.0) like Gecko"></webview>
```

페이지의 `User-Agent`를 설정합니다. 페이지가 로드된 후엔 `setUserAgent` 메소드를
사용해서 변경할 수 있습니다.

### `disablewebsecurity`

```html
<webview src="https://www.github.com/" disablewebsecurity></webview>
```

"on"으로 지정하면 페이지의 웹 보안을 해제합니다.

### `partition`

```html
<webview src="https://github.com" partition="persist:github"></webview>
<webview src="http://electron.atom.io" partition="electron"></webview>
```

페이지에서 사용하는 세션을 설정합니다.
만약 `partition` 속성이 `persist:` 접두사를 시작하면 같은 `partition` 속성을 가진
앱 내 모든 페이지가 공유하는 영구 세션을 사용합니다. `persist:` 접두사가 없을 경우
페이지는 인 메모리 세션을 사용합니다. 동일한 `partition`을 지정하여 다중 페이지에서
동일한 세션을 공유할 수 있도록 할 수 있습니다. 만약 `partition`이 지정되지 않으면 앱의
기본 세션을 사용합니다.

이 값은 첫 탐색 이전에만 지정할 수 있습니다. 즉. 작동중인 렌더러 프로세스의 세션은
변경할 수 없습니다. 이후 이 값을 바꾸려고 시도하면 DOM 예외를 발생시킵니다.

### `allowpopups`

```html
<webview src="https://www.github.com/" allowpopups></webview>
```

"on"으로 지정하면 페이지에서 새로운 창을 열 수 있도록 허용합니다.

### `blinkfeatures`

```html
<webview src="https://www.github.com/" blinkfeatures="PreciseMemoryInfo, CSSVariables"></webview>
```

활성화할 blink 기능을 지정한 `,`로 구분된 문자열의 리스트입니다. 지원하는 기능
문자열의 전체 목록은 [RuntimeEnabledFeatures.in][blink-feature-string] 파일에서
찾을 수 있습니다.

### `disableblinkfeatures`

```html
<webview src="https://www.github.com/" disableblinkfeatures="PreciseMemoryInfo, CSSVariables"></webview>
```

비활성화할 blink 기능을 지정한 `,`로 구분된 문자열의 리스트입니다. 지원하는 기능
문자열의 전체 목록은 [RuntimeEnabledFeatures.in][blink-feature-string] 파일에서
찾을 수 있습니다.

## Methods

`webview` 태그는 다음과 같은 메서드를 가지고 있습니다:

**참고:** <webview> 태그 객체의 메서드는 페이지 로드가 끝난 뒤에만 사용할 수 있습니다.

**예시**

```javascript
webview.addEventListener('dom-ready', () => {
  webview.openDevTools();
});
```

### `<webview>.loadURL(url[, options])`

* `url` URL
* `options` Object (optional)
  * `httpReferrer` String - HTTP 레퍼러 url.
  * `userAgent` String - 요청을 시작한 유저 에이전트.
  * `extraHeaders` String - "\n"로 구분된 Extra 헤더들.

Webview에 웹 페이지 `url`을 로드합니다. `url`은 `http://`, `file://`과 같은
프로토콜 접두사를 가지고 있어야 합니다.

### `<webview>.getURL()`

페이지의 URL을 반환합니다.

### `<webview>.getTitle()`

페이지의 제목을 반환합니다.

### `<webview>.isLoading()`

페이지가 아직 리소스를 로딩하고 있는지 확인합니다. 불린 값을 반환합니다.

### `<webview>.isWaitingForResponse()`

페이지가 메인 리소스의 첫 응답을 기다리고 있는지 확인합니다. 불린 값을 반환합니다.

### `<webview>.stop()`

모든 탐색을 취소합니다.

### `<webview>.reload()`

페이지를 새로고침합니다.

### `<webview>.reloadIgnoringCache()`

캐시를 무시하고 페이지를 새로고침합니다.

### `<webview>.canGoBack()`

페이지 히스토리를 한 칸 뒤로 가기를 할 수 있는지 확인합니다. 불린 값을 반환합니다.

### `<webview>.canGoForward()`

페이지 히스토리를 한 칸 앞으로 가기를 할 수 있는지 확인합니다. 불린 값을 반환합니다.

### `<webview>.canGoToOffset(offset)`

* `offset` Integer

페이지 히스토리를 `offset` 만큼 이동할 수 있는지 확인합니다. 불린값을 반환합니다.

### `<webview>.clearHistory()`

탐색 히스토리를 비웁니다.

### `<webview>.goBack()`

페이지 뒤로 가기를 실행합니다.

### `<webview>.goForward()`

페이지 앞으로 가기를 실행합니다.

### `<webview>.goToIndex(index)`

* `index` Integer

페이지를 지정한 `index`로 이동합니다.

### `<webview>.goToOffset(offset)`

* `offset` Integer

페이지로부터 `offset` 만큼 이동합니다.

### `<webview>.isCrashed()`

렌더러 프로세스가 크래시 됬는지 확인합니다.

### `<webview>.setUserAgent(userAgent)`

* `userAgent` String

`User-Agent`를 지정합니다.

### `<webview>.getUserAgent()`

페이지의 `User-Agent 문자열`을 가져옵니다.

### `<webview>.insertCSS(css)`

* `css` String

페이지에 CSS를 삽입합니다.

### `<webview>.executeJavaScript(code[, userGesture, callback])`

* `code` String
* `userGesture` Boolean
* `callback` Function (optional) - 스크립트의 실행이 완료되면 호출됩니다.
  * `result`

페이지에서 자바스크립트 코드를 실행합니다.

만약 `userGesture`가 `true`로 설정되어 있으면 페이지에 유저 제스쳐 컨텍스트를 만듭니다.
이 옵션을 활성화 시키면 `requestFullScreen`와 같은 HTML API에서 유저의 승인을
무시하고 개발자가 API를 바로 사용할 수 있도록 허용합니다.

**역자주:** 기본적으로 브라우저에선 전체화면, 웹캠, 파일 열기등의 API를 사용하려면 유저의
승인(이벤트)이 필요합니다.

### `<webview>.openDevTools()`

페이지에 대한 개발자 도구를 엽니다.

### `<webview>.closeDevTools()`

페이지에 대한 개발자 도구를 닫습니다.

### `<webview>.isDevToolsOpened()`

페이지에 대한 개발자 도구가 열려있는지 확인합니다. 불린 값을 반환합니다.

### `<webview>.isDevToolsFocused()`

페이지의 개발자 도구에 포커스 되어있는지 여부를 반화합니다.

### `<webview>.inspectElement(x, y)`

* `x` Integer
* `y` Integer

(`x`, `y`) 위치에 있는 엘리먼트를 inspect합니다.

### `<webview>.inspectServiceWorker()`

Service worker에 대한 개발자 도구를 엽니다.

### `<webview>.undo()`

페이지에서 실행 취소 커맨드를 실행합니다.

### `<webview>.redo()`

페이지에서 다시 실행 커맨드를 실행합니다.

### `<webview>.cut()`

페이지에서 잘라내기 커맨드를 실행합니다.

### `<webview>.copy()`

페이지에서 복사 커맨드를 실행합니다.

### `<webview>.paste()`

페이지에서 붙여넣기 커맨드를 실행합니다.

### `<webview>.pasteAndMatchStyle()`

페이지에서 `pasteAndMatchStyle` 편집 커맨드를 실행합니다.

### `<webview>.delete()`

페이지에서 삭제 커맨드를 실행합니다.

### `<webview>.selectAll()`

페이지에서 전체 선택 커맨드를 실행합니다.

### `<webview>.unselect()`

페이지에서 `unselect` 커맨드를 실행합니다.

### `<webview>.replace(text)`

* `text` String

페이지에서 `replace` 커맨드를 실행합니다.

### `<webview>.replaceMisspelling(text)`

* `text` String

페이지에서 `replaceMisspelling` 커맨드를 실행합니다.

### `<webview>.insertText(text)`

* `text` String

포커스된 요소에 `text`를 삽입합니다.

### `webContents.findInPage(text[, options])`

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
[`found-in-page`](web-view-tag.md#event-found-in-page) 이벤트를 통해 취득할 수
있습니다.

### `webContents.stopFindInPage(action)`

* `action` String - [`<webview>.findInPage`](web-view-tag.md#webviewtagfindinpage)
  요청이 종료되었을 때 일어날 수 있는 작업을 지정합니다.
  * `clearSelection` - 선택을 취소합니다.
  * `keepSelection` - 선택을 일반 선택으로 변경합니다.
  * `activateSelection` - 포커스한 후 선택된 노드를 클릭합니다.

제공된 `action`에 대한 `webContents`의 모든 `findInPage` 요청을 중지합니다.

### `<webview>.print([options])`

`webview` 페이지를 인쇄합니다. `webContents.print([options])` 메서드와 같습니다.

### `<webview>.printToPDF(options, callback)`

`webview` 페이지를 PDF 형식으로 인쇄합니다.
`webContents.printToPDF(options, callback)` 메서드와 같습니다.

### `<webview>.capturePage([rect, ]callback)`

`webview`의 페이지의 스냅샷을 캡쳐합니다.
`webContents.printToPDF(options, callback)` 메서드와 같습니다.

### `<webview>.send(channel[, arg1][, arg2][, ...])`

* `channel` String
* `args` (optional)

`channel`을 통해 렌더러 프로세스로 비동기 메시지를 보냅니다. 또한 `args`를 지정하여
임의의 인수를 보낼 수도 있습니다. 렌더러 프로세스는 `ipcRenderer` 모듈의 `channel`
이벤트로 이 메시지를 받아 처리할 수 있습니다.

예시는 [webContents.send](web-contents.md#webcontentssendchannel-args)를 참고하세요.

### `<webview>.sendInputEvent(event)`

* `event` Object

페이지에 입력 `event`를 보냅니다.

`event` 객체에 대해 자세히 알아보려면 [webContents.sendInputEvent](web-contents.md#webcontentssendinputeventevent)를
참고하세요.

### `<webview>.showDefinitionForSelection()` _macOS_

페이지에서 선택된 단어에 대한 사전 검색 결과 팝업을 표시합니다.

### `<webview>.getWebContents()`

이 `webview`에 해당하는 [WebContents](web-contents.md)를 반환합니다.

## DOM 이벤트

`webview` 태그는 다음과 같은 DOM 이벤트를 가지고 있습니다:

### Event: 'load-commit'

Returns:

* `url` String
* `isMainFrame` Boolean

로드가 시작됬을 때 발생하는 이벤트입니다.
이 이벤트는 현재 문서내의 탐색뿐만 아니라 서브 프레임 문서 레벨의 로드도 포함됩니다.
하지만 비동기 리소스 로드는 포함되지 않습니다.

### Event: 'did-finish-load'

탐색이 끝나면 발생하는 이벤트입니다. 브라우저 탭의 스피너가 멈추고 `onload` 이벤트가
발생할 때를 생각하면 됩니다.

### Event: 'did-fail-load'

Returns:

* `errorCode` Integer
* `errorDescription` String
* `validatedURL` String
* `isMainFrame` Boolean

`did-finish-load`와 비슷합니다. 하지만 이 이벤트는 `window.stop()`과 같이 취소
함수가 호출되었거나 로드에 실패했을 때 발생하는 이벤트입니다.

### Event: 'did-frame-finish-load'

Returns:

* `isMainFrame` Boolean

프레임의 탐색이 끝나면 발생하는 이벤트입니다.

### Event: 'did-start-loading'

브라우저 탭의 스피너가 돌기 시작할 때 처럼 페이지의 로드가 시작될 때 발생하는
이벤트입니다.

### Event: 'did-stop-loading'

브라우저 탭의 스피너가 멈출 때 처럼 페이지의 로드가 끝나면 발생하는 이벤트입니다.

### Event: 'did-get-response-details'

Returns:

* `status` Boolean
* `newURL` String
* `originalURL` String
* `httpResponseCode` Integer
* `requestMethod` String
* `referrer` String
* `headers` Object
* `resourceType` String

요청한 리소스에 관해 자세한 내용을 알 수 있을 때 발생하는 이벤트입니다.
`status`는 리소스를 다운로드할 소켓 커낵션을 나타냅니다.

### Event: 'did-get-redirect-request'

Returns:

* `oldURL` String
* `newURL` String
* `isMainFrame` Boolean

리소스를 요청하고 받는 도중에 리다이렉트가 생기면 발생하는 이벤트입니다.

### Event: 'dom-ready'

프레임 문서의 로드가 끝나면 발생하는 이벤트입니다.

### Event: 'page-title-updated'

Returns:

* `title` String
* `explicitSet` Boolean

탐색하는 동안에 페이지의 제목이 설정되면 발생하는 이벤트입니다. `explicitSet`는 파일
URL에서 합성(synthesised)된 제목인 경우 false로 표시됩니다.

### Event: 'page-favicon-updated'

Returns:

* `favicons` Array - URL 배열

페이지가 favicon URL을 받았을 때 발생하는 이벤트입니다.

### Event: 'enter-html-full-screen'

페이지가 HTML API에 의해 전체 화면 모드에 돌입했을 때 발생하는 이벤트입니다.

### Event: 'leave-html-full-screen'

페이지의 전체 화면 모드가 해제됬을 때 발생하는 이벤트입니다.

### Event: 'console-message'

Returns:

* `level` Integer
* `message` String
* `line` Integer
* `sourceId` String

`console.log` API에 의해 로깅될 때 발생하는 이벤트입니다.

다음 예시는 모든 로그 메시지를 로그 레벨이나 다른 속성에 관련 없이 호스트 페이지의
콘솔에 다시 로깅하는 예시입니다.

```javascript
webview.addEventListener('console-message', (e) => {
  console.log('Guest page logged a message:', e.message);
});
```

### Event: 'found-in-page'

Returns:

* `result` Object
  * `requestId` Integer
  * `finalUpdate` Boolean - 더 많은 응답이 따르는 경우를 표시합니다.
  * `activeMatchOrdinal` Integer (optional) - 활성화 일치의 위치.
  * `matches` Integer (optional) - 일치하는 개수.
  * `selectionArea` Object (optional) - 첫 일치 부위의 좌표.

[`webContents.findInPage`](web-contents.md#webcontentsfindinpage) 요청의 결과를
사용할 수 있을 때 발생하는 이벤트입니다.

```javascript
webview.addEventListener('found-in-page', (e) => {
  if (e.result.finalUpdate)
    webview.stopFindInPage('keepSelection');
});

const requestId = webview.findInPage('test');
```

### Event: 'new-window'

Returns:

* `url` String
* `frameName` String
* `disposition` String - `default`, `foreground-tab`, `background-tab`,
  `new-window`, `other`를 사용할 수 있습니다.
* `options` Object - 새로운 `BrowserWindow`를 만들 때 사용되어야 하는 옵션.

페이지가 새로운 브라우저 창을 생성할 때 발생하는 이벤트입니다.

다음 예시 코드는 새 URL을 시스템의 기본 브라우저로 여는 코드입니다.

```javascript
const {shell} = require('electron');

webview.addEventListener('new-window', (e) => {
  const protocol = require('url').parse(e.url).protocol;
  if (protocol === 'http:' || protocol === 'https:') {
    shell.openExternal(e.url);
  }
});
```

### Event: 'will-navigate'

Returns:

* `url` String

사용자 또는 페이지가 새로운 페이지로 이동할 때 발생하는 이벤트입니다.
`window.location` 객체가 변경되거나 사용자가 페이지의 링크를 클릭했을 때 발생합니다.

이 이벤트는 `<webview>.loadURL`과 `<webview>.back` 같은 API를 이용한
프로그램적으로 시작된 탐색에 대해서는 발생하지 않습니다.

이 이벤트는 앵커 링크를 클릭하거나 `window.location.hash`의 값을 변경하는 등의 페이지
내 탐색시엔 발생하지 않습니다. 대신 `did-navigate-in-page` 이벤트를 사용해야 합니다.

`event.preventDefault()`를 호출하는 것은 __아무__ 효과도 내지 않습니다.

### Event: 'did-navigate'

Returns:

* `url` String

탐색이 완료되면 발생하는 이벤트입니다.

이 이벤트는 앵커 링크를 클릭하거나 `window.location.hash`의 값을 변경하는 등의 페이지
내 탐색시엔 발생하지 않습니다. 대신 `did-navigate-in-page` 이벤트를 사용해야 합니다.

### Event: 'did-navigate-in-page'

Returns:

* `url` String

페이지 내의 탐색이 완료되면 발생하는 이벤트입니다.

페이지 내의 탐색이 발생하면 페이지 URL이 변경되지만 페이지 밖으로의 탐색은 일어나지
않습니다. 예를 들어 앵커 링크를 클릭했을 때, 또는 DOM `hashchange` 이벤트가 발생했을
때로 볼 수 있습니다.

### Event: 'close'

페이지가 자체적으로 닫힐 때 발생하는 이벤트입니다.

다음 예시 코드는 페이지가 자체적으로 닫힐 때 `webview`를 `about:blank` 페이지로
이동시키는 예시입니다.

```javascript
webview.addEventListener('close', () => {
  webview.src = 'about:blank';
});
```

### Event: 'ipc-message'

Returns:

* `channel` String
* `args` Array

호스트 페이지에서 비동기 IPC 메시지를 보낼 때 발생하는 이벤트입니다.

`sendToHost` 메소드와 `ipc-message` 이벤트로 호스트 페이지와 쉽게 통신을 할 수
있습니다:

```javascript
// In embedder page.
webview.addEventListener('ipc-message', (event) => {
  console.log(event.channel);
  // Prints "pong"
});
webview.send('ping');
```

```javascript
// In guest page.
const {ipcRenderer} = require('electron');
ipcRenderer.on('ping', () => {
  ipcRenderer.sendToHost('pong');
});
```

### Event: 'crashed'

렌더러 프로세스가 크래시 되었을 때 발생하는 이벤트입니다.

### Event: 'gpu-crashed'

GPU 프로세스가 크래시 되었을 때 발생하는 이벤트입니다.

### Event: 'plugin-crashed'

Returns:

* `name` String
* `version` String

플러그인 프로세스가 크래시 되었을 때 발생하는 이벤트입니다.

### Event: 'destroyed'

WebContents가 파괴될 때 발생하는 이벤트입니다.

### Event: 'media-started-playing'

미디어가 재생되기 시작할 때 발생하는 이벤트입니다.

### Event: 'media-paused'

미디어가 중지되거나 재생이 완료되었을 때 발생하는 이벤트입니다.

### Event: 'did-change-theme-color'

Returns:

* `themeColor` String

페이지의 테마 색이 변경될 때 발생하는 이벤트입니다. 이 이벤트는 보통 meta 태그에
의해서 발생합니다:

```html
<meta name='theme-color' content='#ff0000'>
```

### Event: 'update-target-url'

Returns:

* `url` String

마우스나 키보드를 사용해 링크에 포커스할 때 발생하는 이벤트입니다.

### Event: 'devtools-opened'

개발자 도구가 열렸을 때 발생하는 이벤트입니다.

### Event: 'devtools-closed'

개발자 도구가 닫혔을 때 발생하는 이벤트입니다.

### Event: 'devtools-focused'

개발자 도구가 포커스되거나 열렸을 때 발생하는 이벤트입니다.

[blink-feature-string]: https://cs.chromium.org/chromium/src/third_party/WebKit/Source/platform/RuntimeEnabledFeatures.in
