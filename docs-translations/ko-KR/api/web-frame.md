# webFrame

> 현재 웹 페이지의 렌더링 상태를 커스터마이즈합니다.

다음 예시는 현재 페이지를 200% 줌 합니다:

```javascript
const {webFrame} = require('electron');

webFrame.setZoomFactor(2);
```

## Methods

`webFrame` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `webFrame.setZoomFactor(factor)`

* `factor` Number - Zoom 값

지정한 값으로 페이지를 줌 합니다. 줌 값은 퍼센트를 100으로 나눈 값입니다.
(예시: 300% = 3.0)

### `webFrame.getZoomFactor()`

Returns `Number` - 현재 줌 값.

### `webFrame.setZoomLevel(level)`

* `level` Number - Zoom level

지정한 레벨로 줌 레벨을 변경합니다. 0은 "기본 크기" 입니다. 그리고 각각 레벨 값을
올리거나 내릴 때마다 20%씩 커지거나 작아지고 기본 크기의 50%부터 300%까지 조절 제한이
있습니다.

### `webFrame.getZoomLevel()`

Returns `Number` - 현재 줌 레벨.

### `webFrame.setZoomLevelLimits(minimumLevel, maximumLevel)`

* `minimumLevel` Number
* `maximumLevel` Number

줌 레벨의 최대, 최소치를 지정합니다.

### `webFrame.setSpellCheckProvider(language, autoCorrectWord, provider)`

* `language` String
* `autoCorrectWord` Boolean
* `provider` Object

Input field나 text area에 철자 검사(spell checking) 제공자를 설정합니다.

`provider`는 반드시 전달된 단어의 철자가 맞았는지 검사하는 `spellCheck` 메소드를
가지고 있어야 합니다.

[node-spellchecker][spellchecker]를 철자 검사 제공자로 사용하는 예시입니다:

```javascript
const {webFrame} = require('electron');
webFrame.setSpellCheckProvider('en-US', true, {
  spellCheck (text) {
    return !(require('spellchecker').isMisspelled(text));
  }
});
```

### `webFrame.registerURLSchemeAsSecure(scheme)`

* `scheme` String

`scheme`을 보안 스킴으로 등록합니다.

보안 스킴은 혼합된 콘텐츠 경고를 발생시키지 않습니다. 예를 들어 `https` 와 `data`는
네트워크 공격자로부터 손상될 가능성이 없기 때문에 보안 스킴이라고 할 수 있습니다.

### `webFrame.registerURLSchemeAsBypassingCSP(scheme)`

* `scheme` String

현재 페이지 콘텐츠의 보안 정책에 상관없이 `scheme`로부터 리소스가 로드됩니다.

### `webFrame.registerURLSchemeAsPrivileged(scheme)`

 * `scheme` String

`scheme`를 보안된 스킴으로 등록합니다. 리소스에 대해 보안 정책을 우회하며,
ServiceWorker의 등록과 fetch API를 사용할 수 있도록 지원합니다.

### `webFrame.insertText(text)`

* `text` String

포커스된 요소에 `text`를 삽입합니다.

### `webFrame.executeJavaScript(code[, userGesture])`

* `code` String
* `userGesture` Boolean (optional) - 기본값은 `false` 입니다.

페이지에서 `code`를 실행합니다.

브라우저 윈도우에서 어떤 `requestFullScreen` 같은 HTML API는 사용자의 승인이
필요합니다. `userGesture`를 `true`로 설정하면 이러한 제약을 제거할 수 있습니다.

### `webFrame.getResourceUsage()`

Returns `Object`:
* `images` Object
  * `count` Integer
  * `size` Integer
  * `liveSize` Integer
  * `decodedSize` Integer
  * `purgedSize` Integer
  * `purgeableSize` Integer
* `cssStyleSheets` Object
  * `count` Integer
  * `size` Integer
  * `liveSize` Integer
  * `decodedSize` Integer
  * `purgedSize` Integer
  * `purgeableSize` Integer
* `xslStyleSheets` Object
  * `count` Integer
  * `size` Integer
  * `liveSize` Integer
  * `decodedSize` Integer
  * `purgedSize` Integer
  * `purgeableSize` Integer
* `fonts` Object
  * `count` Integer
  * `size` Integer
  * `liveSize` Integer
  * `decodedSize` Integer
  * `purgedSize` Integer
  * `purgeableSize` Integer
* `other` Object
  * `count` Integer
  * `size` Integer
  * `liveSize` Integer
  * `decodedSize` Integer
  * `purgedSize` Integer
  * `purgeableSize` Integer

Blink의 내부 메모리 캐시 사용 정보를 담고있는 객체를 반환합니다.

```javascript
const {webFrame} = require('electron');
console.log(webFrame.getResourceUsage());
```

다음이 출력됩니다:

```javascript
{
  images: {
    count: 22,
    size: 2549,
    liveSize: 2542,
    decodedSize: 478,
    purgedSize: 0,
    purgeableSize: 0
  },
  cssStyleSheets: { /* same with "images" */ },
  xslStyleSheets: { /* same with "images" */ },
  fonts: { /* same with "images" */ },
  other: { /* same with "images" */ },
}
```

### `webFrame.clearCache()`

(이전 페이지의 이미지 등) 사용하지 않는 메모리 해제를 시도합니다.

참고로 맹목적으로 이 메서드를 호출하는 것은 이 빈 캐시를 다시 채워야하기 때문에
Electron을 느리게 만듭니다. 따라서 이 메서드는 페이지가 예상했던 것 보다 실질적으로 더
적은 메모리를 사용하게 만드는 애플리케이션 이벤트가 발생했을 때만 호출해야 합니다.
(i.e. 아주 무거운 페이지에서 거의 빈 페이지로 이동한 후 계속 유지할 경우)

[spellchecker]: https://github.com/atom/node-spellchecker
