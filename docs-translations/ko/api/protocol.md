# protocol

`protocol` 모듈은 이미 있는 프로토콜의 동작을 가로채거나 새로운 프로토콜을 만들 수 있는 기능을 제공합니다.

다음 예제는 `file://` 프로토콜과 비슷한 일을 하는 커스텀 프로토콜을 설정합니다:

```javascript
var app = require('app');
var path = require('path');

app.on('ready', function() {
    var protocol = require('protocol');
    protocol.registerFileProtocol('atom', function(request, callback) {
      var url = request.url.substr(7);
      callback({path: path.normalize(__dirname + '/' + url)});
    }, function (error) {
      if (error)
        console.error('Failed to register protocol')
    });
});
```

**참고:** 이 모듈은 `app` 모듈의 `ready` 이벤트가 발생한 이후에만 사용할 수 있습니다.

## Methods

`protocol` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### `protocol.registerStandardSchemes(schemes)`

* `schemes` Array - 표준 스킴으로 등록할 커스텀 스킴 리스트

표준 `scheme`의 형식은 RFC 3986 [일반 URI 구문](https://tools.ietf.org/html/rfc3986#section-3) 표준을 따릅니다.
이 형식은 `file:`과 `filesystem:`을 포함합니다.

### `protocol.registerFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`에 파일을 응답으로 보내는 프로토콜을 등록합니다.
`handler`는 `scheme`와 함께 `request`가 생성될 때 `handler(request, callback)` 형식으로 호출됩니다.
`completion` 콜백은 `scheme`가 성공적으로 등록되었을 때 `completion(null)` 형식으로 호출되고
등록에 실패했을 땐 `completion(error)` 형식으로 에러 내용을 담아 호출됩니다.

`request`를 처리할 때 반드시 파일 경로 또는 `path` 속성을 포함하는 객체를 인자에 포함하여 `callback`을 호출해야 합니다.
예: `callback(filePath)` 또는 `callback({path: filePath})`.

만약 `callback`이 아무 인자도 없이 호출되거나 숫자나 `error` 프로퍼티를 가진 객체가 인자로 전달될 경우
`request`는 지정한 `error` 코드(숫자)를 출력합니다.
사용할 수 있는 에러 코드는 [네트워크 에러 목록](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h)에서 확인할 수 있습니다.

기본적으로 `scheme`은 `http:`와 같이 처리됩니다. `file:`과 같이 "일반적인 URI 문법"과는 다르게 인식되는 프로토콜은
`protocol.registerStandardSchemes`을 사용하여 표준 스킴으로 처리되도록 할 수 있습니다.

### `protocol.registerBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`에 `Buffer`를 응답으로 보내는 프로토콜을 등록합니다.
반드시 `Buffer` 또는 `data`, `mimeType`, `chart` 속성을 포함한 객체 중 하나를 인자에 포함하여 `callback`을 호출해야 합니다.

예제:

```javascript
protocol.registerBufferProtocol('atom', function(request, callback) {
  callback({mimeType: 'text/html', data: new Buffer('<h5>Response</h5>')});
}, function (error) {
  if (error)
    console.error('Failed to register protocol')
});
```

### `protocol.registerStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`에 `문자열`을 응답으로 보내는 프로토콜을 등록합니다.
반드시 `문자열` 또는 `data`, `mimeType`, `chart` 속성을 포함한 객체 중 하나를 인자에 포함하여 `callback`을 호출해야 합니다.

### `protocol.registerHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme`에 HTTP 요청을 응답으로 보내는 프로토콜을 등록합니다.
반드시 `url`, `method`, `referer`, `session` 속성을 포함하는 객체를 인자에 포함하여 `callback`을 호출해야 합니다.

기본적으로 HTTP 요청은 현재 세션을 재사용합니다. 만약 서로 다른 세션에 요청을 보내고 싶으면 `session`을 `null`로 지정해야 합니다.

### `protocol.unregisterProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (optional)

`scheme`의 커스텀 프로토콜 등록을 해제합니다.

### `protocol.isProtocolHandled(scheme, callback)`

* `scheme` String
* `callback` Function

`scheme`에 동작(handler)이 등록되어 있는지 여부를 확인합니다. `callback`으로 결과(boolean)가 반환됩니다.

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

`scheme` 프로토콜을 가로채고 `handler`를 `Buffer` 전송에 대한 새로운 동작으로 사용합니다.

### `protocol.interceptHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

`scheme` 프로토콜을 가로채고 `handler`를 HTTP 프로토콜의 요청에 대한 새로운 동작으로 사용합니다.

### `protocol.uninterceptProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (optional)

가로챈 `scheme`를 삭제하고 기본 핸들러로 복구합니다.
