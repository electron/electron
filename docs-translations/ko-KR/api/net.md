# net

> Chromium 의 네이티브 네트워크 라이브러리를 사용한 HTTP/HTTPS 요청 실행.

`net` 모듈은 HTTP(S) 요청을 실행하기 위한 클라이언트 측 API 입니다. 이것은
Node.js 의 [HTTP](https://nodejs.org/api/http.html) 와
[HTTPS](https://nodejs.org/api/https.html) 모듈과 유사합니다. 하지만 Node.js
구현 대신 Chromium 의 네이티브 네트워크 라이브러리를 사용하는 것은, 웹 프록시에
대한 더 나은 지원을 제공합니다.

다음은 왜 네이티브 Node.js 모듈대신 `net` 모듈 사용을 고려해야하는지에 대한
대략적인 이유입니다:

* WPAD 프로토콜과 프록시 PAC 환경설정 파일 지원을 통한 시스템 프록시 구성 자동
  관리.
* HTTPS 요청의 자동 터널링.
* 기본, digest, NTLM, Kerberos, 협상 인증 스킴을 사용한 프록시 인증 지원.
* 트래픽 모니터링 프록시 지원: 접근 제어와 모니터링에 사용되는 Fiddler 같은
  프록시.

`net` 모듈 API 는 Node.js API 와 최대한 가까워 보이도록 설계되었습니다. 클래스,
메소드, 속성, 이벤트를 포함하는 API 컴포넌트는 일반적으로 Node.js 에서 사용되는
것과 유사합니다.

예를 들면, 다음 예제는 `net` API 가 사용될 수 있는 방법을 빠르게 보여줍니다:

```javascript
const {app} = require('electron')
app.on('ready', () => {
  const {net} = require('electron')
  const request = net.request('https://github.com')
  request.on('response', (response) => {
    console.log(`STATUS: ${response.statusCode}`)
    console.log(`HEADERS: ${JSON.stringify(response.headers)}`)
    response.on('data', (chunk) => {
      console.log(`BODY: ${chunk}`)
    })
    response.on('end', () => {
      console.log('No more data in response.')
    })
  })
  request.end()
})
```

그런데, 일반적으로 Node.js 의 [HTTP](https://nodejs.org/api/http.html)/
[HTTPS](https://nodejs.org/api/https.html) 모듈을 사용하는 방법과 거의
동일합니다.

`net` API 는 애플리케이션이 `ready` 이벤트를 발생한 이후부터 사용할 수 있습니다.
`ready` 이벤트 전에 모듈을 사용하려고 하면 오류가 발생할 것 입니다.

## Methods

`net` 모듈은 다음의 메소드를 가지고 있습니다:

### `net.request(options)`

* `options` (Object | String) - `ClientRequest` 생성자 옵션.

Returns `ClientRequest`

주어진 `options` 을 사용해 `ClientRequest` 인스턴스를 생성합니다.
`ClientRequest` 생성자에 직접 전달됩니다. `net.request` 메소드는 `options`
객체의 명시된 프로토콜 스킴에 따라 보안 및 비보안 HTTP 요청을 실행하는데
사용됩니다.

## Class: ClientRequest

> HTTP/HTTPS 요청을 합니다.

`ClientRequest` 는
[Writable Stream](https://nodejs.org/api/stream.html#stream_writable_streams)
인터페이스를 구현하였고 따라서
[EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter)
입니다.

### `new ClientRequest(options)`

* `options` (Object | String) - `options` 이 문자열이면, 요청 URL 로 해석됩니다.
  객체라면, 다음 속성을 통해 HTTP 요청을 완전히 명시한 것으로 예상됩니다:
  * `method` String (optional) - TheHTTP 요청 방법. 기본값은 GET 방법입니다.
  * `url` String (optional) - 요청 URL. HTTP 또는 HTTPS 로 지정된 프로토콜
    스킴과 함께 절대적인 형식으로 제공되어야 합니다.
  * `session` Object (optional) - 요청과 연관된 [`Session`](session.md)인스턴스.
  * `partition` String (optional) - 요청과 연관된 [`partition`](session.md) 의
    이름. 기본값은 빈 문자열입니다. `session` 옵션은 `partition` 에 우선합니다.
    그러므로 `session` 이 명시적으로 지정된 경우, `partition` 은 무시됩니다.
  * `protocol` String (optional) - 'scheme:' 형식의 프로토콜 스킴. 현재 지원되는
    값은 'http:' 또는 'https:' 입니다. 기본값은 'http:' 입니다.
  * `host` String (optional) - 호스트명과 포트번호의 연결로 제공되는 서버 호스트
    '호스트명:포트'
  * `hostname` String (optional) - 서버 호스트명.
  * `port` Integer (optional) - 서버의 리스닝 포트 번호.
  * `path` String (optional) - 요청 URL 의 경로 부분.

`protocol`, `host`, `hostname`, `port`, `path` 같은 `options` 속성은
[URL](https://nodejs.org/api/url.html) 모듈에 설명 된대로 Node.js 모델을 엄격히
따릅니다.

예를 들어, 다음과 같이 'github.com' 에 대한 같은 요청을 만들 수 있습니다:

```JavaScript
const request = net.request({
  method: 'GET',
  protocol: 'https:',
  hostname: 'github.com',
  port: 443,
  path: '/'
})
```

### Instance Events

#### Event: 'response'

Returns:

* `response` IncomingMessage - HTTP 응답 메시지를 나타내는 객체.

#### Event: 'login'

Returns:

* `authInfo` Object
  * `isProxy` Boolean
  * `scheme` String
  * `host` String
  * `port` Integer
  * `realm` String
* `callback` Function

인증 프록시가 사용자 자격 증명을 요구하는 경우 발생됩니다.

`callback` 함수는 사용자 자격 증명으로 다시 호출될 것으로 예상됩니다:

* `username` String
* `password` String

```JavaScript
request.on('login', (authInfo, callback) => {
  callback('username', 'password')
})
```

빈 자격증명을 제공하면 요청이 취소되고 응답 객체에서 인증 오류가 보고될 것
입니다:

```JavaScript
request.on('response', (response) => {
  console.log(`STATUS: ${response.statusCode}`);
  response.on('error', (error) => {
    console.log(`ERROR: ${JSON.stringify(error)}`)
  })
})
request.on('login', (authInfo, callback) => {
  callback()
})
```

#### Event: 'finish'

`request` 객체에 `request` 데이터의 마지막 덩어리가 기록된 후 발생됩니다.

#### Event: 'abort'

`request` 가 취소된 경우 발생됩니다. `request` 가 이미 닫힌 경우 `abort`
이벤트는 발생되지 않습니다.

#### Event: 'error'

Returns:

* `error` Error - 실패에 대한 몇가지 정보를 제공하는 오류 객체입니다.

`net` 모듈이 네트워크 요청 실행을 실패한 경우 발생됩니다. 일반적으로 `request`
객체가 `error` 이벤트를 발생할 때, `close` 이벤트가 연속적으로 발생하고 응답
객체는 제공되지 않습니다.

#### Event: 'close'

HTTP 요청-응답 처리의 마지막 이벤트로써 발생됩니다. `close` 이벤트는 `request`
또는 `reponse` 객체에서 더이상 이벤트가 발생되지 않을 것임을 나타냅니다.

### Instance Properties

#### `request.chunkedEncoding`

HTTP 덩어리 전송 인코딩을 사용할지 여부를 명시하는 불 값입니다. 기본값은 `false`
입니다. 속성은 읽고 쓸 수 있습니다. 그러나 HTTP 헤더가 아직 네트워크 선에
쓰여지지 않은 첫 쓰기 작업 전에만 쓸 수 있습니다. 첫 쓰기 이후에
`chunkedEncoding` 을 쓰려고 하면 오류가 발생합니다.

Electron 프로세스 메모리에서 내부적으로 버퍼링하는 대신 작은 덩어리로 데이터로써
큰 요청 본문을 전송하려면 덩어리 인코딩을 사용하는 것이 좋습니다.

### Instance Methods

#### `request.setHeader(name, value)`

* `name` String - 추가 HTTP 헤더 이름.
* `value` String - 추가 HTTP 헤더 값.

추가 HTTP 헤더를 추가합니다. 헤더 이름은 소문자화 되지 않고 실행됩니다. 첫 쓰기
전까지만 호출할 수 있습니다. 첫 쓰기 후 이 메소드를 호출하면 오류가 발생합니다.

#### `request.getHeader(name)`

* `name` String - 추가 헤더 이름을 명시합니다.

Returns String - 이전에 설정된 추가 헤더 이름에 대한 값.

#### `request.removeHeader(name)`

* `name` String - 추가 헤더 이름을 명시합니다.

이전에 설정된 추가 헤더 이름을 삭제합니다. 이 메소드는 첫 쓰기 전까지만 호출할
수 있습니다. 첫 쓰기 후 호출하면 오류가 발생합니다.

#### `request.write(chunk[, encoding][, callback])`

* `chunk` (String | Buffer) - 요청 본문 데이터의 덩어리. 문자열이라면, 지정된
  인코딩을 사용한 버퍼로 변환됩니다.
* `encoding` String (optional) - 버퍼 객체로 문자열을 변환하는데 사용됩니다.
  기본값은 'utf-8' 입니다.
* `callback` Function (optional) - 쓰기 작업이 끝난 이후 호출됩니다.

`callback` 은 근본적으로 Node.js API 와의 유사성을 유지하는 목적으로 도입된 더미
함수입니다. 그것은 `chunk` 내용이 Chromium 네트워크 계층에 전달 된 후 다음
틱에서 비동기적으로 호출됩니다. Node.js 구현과 달리, `callback` 호출 이전에
`chunk` 내용이 네트워크 선에 내보내진 것을 보장하지 않습니다.

요청 본문에 데이터의 덩어리를 추가합니다. 첫 쓰기 작업은 네트워크 선에 요청
헤더를 쓸 것 입니다. 첫 쓰기 작업 이후, 사용자 정의 헤더 추가 또는 삭제는
허용되지 않습니다.

#### `request.end([chunk][, encoding][, callback])`

* `chunk` (String | Buffer) (optional)
* `encoding` String (optional)
* `callback` Function (optional)

요청 데이터의 마지막 덩어리를 보냅니다. 이후 쓰기 또는 종료 명령은 허용되지
않습니다. `finish` 이벤트는 `end` 명령 이후에 발생합니다.

#### `request.abort()`

현재 진행중인 HTTP 처리를 취소합니다. 요청이 이미 `close` 이벤트를 발생했다면,
취소 명령은 적용되지 않습니다. 그렇지 않으면 진행중인 이벤트는 `abort` 와
`close` 이벤트를 발생시킵니다. 또한, 현재 진행중인 응답 객체가 있다면, `aborted`
이벤트를 발생할 것 입니다.

## Class: IncomingMessage

> HTTP/HTTPS 요청에 대한 응답을 처리합니다.

`IncomingMessage` 는
[Readable Stream](https://nodejs.org/api/stream.html#stream_readable_streams)
인터페이스를 구현하였고 따라서
[EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter)
입니다.

### Instance Events

#### Event: 'data'

Returns:

* `chunk` Buffer - 응답 본문 데이터의 덩어리.

`data` 이벤트는 응용 코드에 응답 데이터를 전송하는 통상적인 방법입니다.

#### Event: 'end'

응답 본문이 종료되었음을 나타냅니다.

#### Event: 'aborted'

요청이 계속 진행중인 HTTP 처리 도중 취소되었을 때 발생합니다.

#### Event: 'error'

Returns:

`error` Error - 일반적으로 실패 원인을 식별하는 오류 문자열을 가지고 있습니다.

스트리밍 응답 데이터 이벤트를 처리하는 동안 오류가 발생했을 때 발생됩니다.
예를 들어, 응답이 여전히 스트리밍되는 동안 서버가 닫히는 경우, 응답 객체에서
`error` 이벤트가 발생할 것이며 그 후에 요청 객체에서 `close` 이벤트가 발생할 것
입니다.

### Instance Properties

`IncomingMessage` 인스턴스는 다음의 읽을 수 있는 속성을 가지고 있습니다:

#### `response.statusCode`

HTTP 응답 상태 코드를 나타내는 정수.

#### `response.statusMessage`

HTTP 상태 메시지를 나타내는 문자열.

#### `response.headers`

응답 HTTP 헤더를 나타내는 객체. `headers` 객체는 다음 형식입니다:

* 모든 헤더 이름은 소문자입니다.
* 각 헤더 이름은 헤더 객체에 배열-값 속성을 생성합니다.
* 각 헤더 값은 헤더 이름과 연관된 배열에 푸시됩니다.

#### `response.httpVersion`

HTTP 프로토콜 버전 번호를 나타내는 문자열. 일반적으로 '1.0' 또는 '1.1' 입니다.
또한 `httpVersionMajor` 와 `httpVersionMinor` 는 각각 HTTP 메이저와 마이너 버전
번호를 반환하는 두개의 정수값 형태로 읽을 수 있는 속성입니다.
