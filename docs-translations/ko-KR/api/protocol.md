# protocol

> 커스텀 프로토콜을 등록하거나 이미 존재하능 프로토콜의 요청의 동작을 변경합니다.

다음 예시는 `file://` 프로토콜과 비슷한 일을 하는 커스텀 프로토콜을 설정합니다:

```javascript
const {app, protocol} = require('electron')
const path = require('path')

app.on('ready', () => {
  protocol.registerFileProtocol('atom', (request, callback) => {
    const url = request.url.substr(7)
    callback({path: path.normalize(`${__dirname}/${url}`)})
  }, function (error) {
    if (error) console.error('프로토콜 등록 실패')
  })
})
```

**참고:** 모든 메서드는 따로 표기하지 않는 한 `app` 모듈의 `ready` 이벤트가 발생한
이후에만 사용할 수 있습니다.

## Methods

`protocol` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `protocol.registerStandardSchemes(schemes)`

* `schemes` Array - 표준 스킴으로 등록할 커스텀 스킴 리스트

표준 스킴의 형식은 RFC 3986 [일반 URI 문법](https://tools.ietf.org/html/rfc3986#section-3)
표준을 따릅니다. 예를 들어 `http`와 `https` 같은 표준 스킴과 `file`과 같은 표준이
아닌 스킴이 있습니다.

표준 스킴으로 등록하면, 상대, 절대 경로의 리소스를 올바르게 취할 수 있습니다. 다른
경우엔 스킴이 상대 경로 URL에 대한 분석 기능이 제외된 `file` 프로토콜과 같이
작동합니다.

예를 들어 다음과 같은 페이지에서 표준 스킴 등록 절차 없이 커스텀 프로토콜을 사용하여
이미지를 로드하려 했을 때, 표준 스킴으로 등록되지 않은 상대 경로 URL을 인식하지 못하고
로드에 실패하게 됩니다:

```html
<body>
  <img src='test.png'>
</body>
```

표준 스킴으로 등록하는 것은 [파일 시스템 API][file-system-api]를 통해 파일에 접근하는
것을 허용할 것입니다. 그렇지 않은 경우 렌더러는 해당 스킴에 대해 보안 에러를 발생 할
것입니다.

표준 스킴에는 기본적으로 저장 API (localStorage, sessionStorage, webSQL,
indexedDB, cookies) 가 비활성화 되어있습니다. 일반적으로 `http` 프로토콜을
대체하는 커스텀 프로토콜을 등록하고 싶다면, 표준 스킴으로 등록해야 합니다:

```javascript
const {app, protocol} = require('electron')

protocol.registerStandardSchemes(['atom'])
app.on('ready', () => {
  protocol.registerHttpProtocol('atom', '...')
})
```

**참고:** 이 메서드는 `app` 모듈의 `ready` 이벤트가 발생하기 이전에만 사용할 수
있습니다.

### `protocol.registerServiceWorkerSchemes(schemes)`

* `schemes` Array - 서비스 워커를 제어하기 위해 등록될 커스텀 스킴.

### `protocol.registerFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`에 파일을 응답으로 보내는 프로토콜을 등록합니다. `handler`는 `scheme`와 함께
`request`가 생성될 때 `handler(request, callback)` 형식으로 호출됩니다.
`completion` 콜백은 `scheme`가 성공적으로 등록되었을 때 `completion(null)` 형식으로
호출되고, 등록에 실패했을 땐 `completion(error)` 형식으로 에러 내용을 담아 호출됩니다.

* `request` Object
  * `url` String
  * `referrer` String
  * `method` String
  * `uploadData` Array (optional)
* `callback` Function

`uploadData` 는 `data` 객체의 배열입니다:

* `data` Object
  * `bytes` Buffer - 전송될 콘텐츠.
  * `file` String - 업로드될 파일의 경로.
  * `blobUUID` String - blob 데이터의 UUID. 데이터를 이용하기 위해
    [ses.getBlobData](session.md#sesgetblobdataidentifier-callback) 메소드를
    사용하세요.

`request`를 처리할 때 반드시 파일 경로 또는 `path` 속성을 포함하는 객체를 인수에
포함하여 `callback`을 호출해야 합니다. 예: `callback(filePath)` 또는
`callback({path: filePath})`.

만약 `callback`이 아무 인수도 없이 호출되거나 숫자나 `error` 프로퍼티를 가진 객체가
인수로 전달될 경우 `request`는 지정한 `error` 코드(숫자)를 출력합니다. 사용할 수 있는
에러 코드는 [네트워크 에러 목록][net-error]에서 확인할 수 있습니다.

기본적으로 `scheme`은 `http:`와 같이 처리됩니다. `file:`과 같이 "일반적인 URI 문법"
과는 다르게 인식되는 프로토콜은 `protocol.registerStandardSchemes`을 사용하여 표준
스킴으로 처리되도록 할 수 있습니다.

### `protocol.registerBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`Buffer`를 응답으로 전송하는 `scheme`의 프로토콜을 등록합니다.

사용법은 `callback`이 반드시 `Buffer` 객체 또는 `data`, `mimeType`, `charset`
속성을 포함하는 객체와 함께 호출되어야 한다는 점을 제외하면 `registerFileProtocol`과
사용법이 같습니다.

예시:

```javascript
const {protocol} = require('electron')

protocol.registerBufferProtocol('atom', (request, callback) => {
  callback({mimeType: 'text/html', data: new Buffer('<h5>Response</h5>')})
}, (error) => {
  if (error) console.error('Failed to register protocol')
})
```

### `protocol.registerStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`String`을 응답으로 전송할 `scheme`의 프로토콜을 등록합니다.

사용법은 `callback`이 반드시 `String` 또는 `data`, `mimeType`, `charset` 속성을
포함하는 객체와 함께 호출되어야 한다는 점을 제외하면 `registerFileProtocol`과
사용법이 같습니다.

### `protocol.registerHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

HTTP 요청을 응답으로 전송할 `scheme`의 프로토콜을 등록합니다.

사용법은 `callback`이 반드시 `url`, `method`, `referrer`, `uploadData`와
`session` 속성을 포함하는 `redirectRequest` 객체와 함께 호출되어야 한다는 점을
제외하면 `registerFileProtocol`과 사용법이 같습니다.

* `redirectRequest` Object
  * `url` String
  * `method` String
  * `session` Object (optional)
  * `uploadData` Object (optional)

기본적으로 HTTP 요청은 현재 세션을 재사용합니다. 만약 서로 다른 세션에 요청을 보내고
싶으면 `session`을 `null`로 지정해야 합니다.

POST 요청에는 반드시 `uploadData` 객체가 제공되어야 합니다.

* `uploadData` object
  * `contentType` String - 콘텐츠의 MIME 타입.
  * `data` String - 전송할 콘텐츠.

### `protocol.unregisterProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (optional)

`scheme`의 커스텀 프로토콜 등록을 해제합니다.

### `protocol.isProtocolHandled(scheme, callback)`

* `scheme` String
* `callback` Function

`scheme`에 동작(handler)이 등록되어 있는지 여부를 확인합니다. `callback`으로
결과(boolean)가 반환됩니다.

### `protocol.interceptFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme` 프로토콜을 가로채고 `handler`를 파일 전송에 대한 새로운 동작으로 사용합니다.

### `protocol.interceptStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme` 프로토콜을 가로채고 `handler`를 문자열 전송에 대한 새로운 동작으로 사용합니다.

### `protocol.interceptBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme` 프로토콜을 가로채고 `handler`를 `Buffer` 전송에 대한 새로운 동작으로
사용합니다.

### `protocol.interceptHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme` 프로토콜을 가로채고 `handler`를 HTTP 프로토콜의 요청에 대한 새로운 동작으로
사용합니다.

### `protocol.uninterceptProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (optional)

가로챈 `scheme`를 삭제하고 기본 핸들러로 복구합니다.

[net-error]: https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h
[file-system-api]: https://developer.mozilla.org/en-US/docs/Web/API/LocalFileSystem
