# protocol

`protocol` 모듈은 여러 프로토콜의 요청과 응답을 커스터마이즈 할 수 있도록 이미 있는 프로토콜을 변경하거나 새로운 프로토콜을 만드는 방법을 제공합니다.

다음 예제는 `file://` 프로토콜과 같은 일을 하는 커스텀 프로토콜을 설정합니다:

```javascript
var app = require('app');
var path = require('path');

app.on('ready', function() {
    var protocol = require('protocol');
    protocol.registerProtocol('atom', function(request) {
      var url = request.url.substr(7)
      return new protocol.RequestFileJob(path.normalize(__dirname + '/' + url));
    }, function (error, scheme) {
      if (!error)
        console.log(scheme, ' registered successfully')
    });
});
```

**알림:** 이 모듈은 app의 `ready` 이벤트가 발생한 이후에만 사용할 수 있습니다.

## protocol.registerProtocol(scheme, handler, callback)

* `scheme` String
* `handler` Function
* `callback` Function 

지정한 `scheme`을 기반으로 커스텀 프로토콜을 등록합니다. `handler`는 등록한 `scheme` 프로토콜에 요청이 들어올 경우 `request` 인자와 함께 `handler(request)` 형식으로 호출됩니다.

`handler` 함수에선 요청에 대한 해당 프로토콜의 작업 결과를 응답(반환) 해야 합니다.

기본적으로 스킴은 `http:`와 비슷합니다. `file:`과 같이 "표준 URI 구문"을 다르게 해석되게 하려면
`protocol.registerStandardSchemes` 메서드를 이용해서 사용자 정의 스킴을 표준 스킴으로 만들 수 있습니다.

## protocol.unregisterProtocol(scheme, callback)

* `scheme` String
* `callback` Function

지정한 `scheme` 프로토콜을 등록 해제합니다.

## protocol.registerStandardSchemes(value)

* `value` Array

지정한 `value` 배열을 사용하여 미리 지정된 표준 스킴으로 등록합니다.

표준 스킴은 RFC 3986 [표준 URI 구문](https://tools.ietf.org/html/rfc3986#section-3)에 해당합니다.
이 표준은 `file:`과 `filesystem:`을 포함합니다.

## protocol.isHandledProtocol(scheme, callback)

* `scheme` String
* `callback` Function

해당 `scheme`에 처리자(handler)가 등록되었는지 확인합니다.
지정한 `callback`에 결과가 boolean 값으로 반환됩니다.

## protocol.interceptProtocol(scheme, handler, callback)

* `scheme` String
* `handler` Function
* `callback` Function

지정한 `scheme`의 작업을 `handler`로 변경합니다.
`handler`에서 `null` 또는 `undefined`를 반환 할 경우 해당 프로토콜의 기본 동작(응답)으로 대체 됩니다.

## protocol.uninterceptProtocol(scheme, callback)

* `scheme` String
* `callback` Function

변경된 프로토콜의 작업을 해제합니다.

## Class: protocol.RequestFileJob(path)

* `path` String

`path` 경로를 기반으로 파일을 반환하는 request 작업을 생성합니다. 그리고 해당 파일에 상응하는 mime type을 지정합니다.

## Class: protocol.RequestStringJob(options)

* `options` Object
  * `mimeType` String - 기본값: `text/plain`
  * `charset` String - 기본값: `UTF-8`
  * `data` String

문자열을 반환하는 request 작업을 생성합니다.

## Class: protocol.RequestBufferJob(options)

* `options` Object
  * `mimeType` String - 기본값: `application/octet-stream`
  * `encoding` String - 기본값: `UTF-8`
  * `data` Buffer

버퍼를 반환하는 request 작업을 생성합니다.

## Class: protocol.RequestHttpJob(options)

* `options` Object
  * `session` [Session](browser-window.md#class-session) - 기본적으로 이 옵션은 어플리케이션의 기본 세션입니다.
    `null`로 설정하면 요청을 위한 새로운 세션을 만듭니다.
  * `url` String
  * `method` String - 기본값: `GET`
  * `referrer` String

`url`의 요청 결과를 그대로 반환하는 request 작업을 생성합니다.

## Class: protocol.RequestErrorJob(code)

* `code` Integer

콘솔에 특정한 네트워크 에러 메시지를 설정하는 request 작업을 생성합니다.
기본 메시지는 `net::ERR_NOT_IMPLEMENTED`입니다. 사용할 수 있는 코드의 범위는 다음과 같습니다.

* 범위:
  * 0- 99 System related errors
  * 100-199 Connection related errors
  * 200-299 Certificate errors
  * 300-399 HTTP errors
  * 400-499 Cache errors
  * 500-599 ?
  * 600-699 FTP errors
  * 700-799 Certificate manager errors
  * 800-899 DNS resolver errors

에러 코드와 메시지에 대해 자세하게 알아보려면 [네트워크 에러 리스트](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h)를 참고하기 바랍니다.
