# session

`session` 모듈은 새로운 `Session` 객체를 만드는데 사용할 수 있습니다.

또한 존재하는 [`BrowserWindow`](browser-window.md)의
[`webContents`](web-contents.md)에서 `session` 속성으로 접근할 수도 있습니다.

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadURL("http://github.com");

var ses = win.webContents.session;
```

## Methods

`session` 모듈은 다음과 같은 메서드를 가지고 있습니다:

### session.fromPartition(partition)

* `partition` String

`partition` 문자열로 부터 새로운 `Session` 인스턴스를 만들어 반환합니다.

`partition`이 `persist:`로 시작하면 페이지는 지속성 세션을 사용하며 다른 모든 앱 내의
페이지에서 같은 `partition`을 사용할 수 있습니다. 만약 `persist:` 접두어로 시작하지
않으면 페이지는 인-메모리 세션을 사용합니다. `partition`을 지정하지 않으면 어플리케이션의
기본 세션이 반환됩니다.

## Properties

`session` 모듈은 다음과 같은 속성을 가지고 있습니다:

### session.defaultSession

어플리케이션의 기본 세션 객체를 반환합니다.

## Class: Session

`session` 모듈을 사용하여 `Session` 객체를 생성할 수 있습니다:

```javascript
const session = require('electron').session;

var ses = session.fromPartition('persist:name');
 ```

### Instance Events

`Session` 객체는 다음과 같은 이벤트를 가지고 있습니다:

#### Event: 'will-download'

* `event` Event
* `item` [DownloadItem](download-item.md)
* `webContents` [WebContents](web-contents.md)

Electron의 `webContents`에서 `item`을 다운로드할 때 발생하는 이벤트입니다.

`event.preventDefault()` 메서드를 호출하면 다운로드를 취소하고, 프로세스의 다음
틱부터 `item`을 사용할 수 없게 됩니다.

```javascript
session.defaultSession.on('will-download', function(event, item, webContents) {
  event.preventDefault();
  require('request')(item.getURL(), function(data) {
    require('fs').writeFileSync('/somewhere', data);
  });
});
```

### Instance Methods

`Session` 객체는 다음과 같은 메서드와 속성을 가지고 있습니다:

#### `ses.cookies`

`cookies` 속성은 쿠키를 조작하는 방법을 제공합니다. 예를 들어 다음과 같이 할 수
있습니다:

```javascript
// 모든 쿠키를 요청합니다.
session.defaultSession.cookies.get({}, function(error, cookies) {
  console.log(cookies);
});

// url에 관련된 쿠키를 모두 가져옵니다.
session.defaultSession.cookies.get({ url : "http://www.github.com" }, function(error, cookies) {
  console.log(cookies);
});

// 지정한 쿠키 데이터를 설정합니다.
// 동일한 쿠키가 있으면 해당 쿠키를 덮어씁니다.
var cookie = { url : "http://www.github.com", name : "dummy_name", value : "dummy" };
session.defaultSession.cookies.set(cookie, function(error) {
  if (error)
    console.error(error);
});
```

#### `ses.cookies.get(filter, callback)`

* `filter` Object
  * `url` String (optional) - `url`에 해당하는 쿠키를 취득합니다. 이 속성을
  생략하면 모든 url에서 찾습니다.
  * `name` String (optional) - 쿠키의 이름입니다.
  * `domain` String (optional) - 도메인 또는 서브 도메인에 일치하는 쿠키를
  취득합니다.
  * `path` String (optional) - `path`에 일치하는 쿠키를 취득합니다.
  * `secure` Boolean (optional) - 보안 속성에 따라 쿠키를 필터링합니다.
  * `session` Boolean (optional) - 세션 또는 지속성 쿠키를 필터링합니다.
* `callback` Function

`details` 객체에서 묘사한 모든 쿠키를 요청합니다. 모든 작업이 끝나면 `callback`이
`callback(error, cookies)` 형태로 호출됩니다.

`cookies`는 `cookie` 객체의 배열입니다.

* `cookie` Object
  *  `name` String - 쿠키의 이름.
  *  `value` String - 쿠키의 값.
  *  `domain` String - 쿠키의 도메인.
  *  `hostOnly` String - 쿠키가 호스트 전용인가에 대한 여부.
  *  `path` String - 쿠키의 경로.
  *  `secure` Boolean - 쿠키가 안전한 것으로 표시되는지에 대한 여부.
  *  `httpOnly` Boolean - 쿠키가 HTTP로만 표시되는지에 대한 여부.
  *  `session` Boolean - 쿠키가 세션 쿠키 또는 만료일이 있는 영구 쿠키인지에 대한
    여부.
  *  `expirationDate` Double - (Option) UNIX 시간으로 표시되는 쿠키의 만료일에
    대한 초 단위 시간. 세션 쿠키는 지원되지 않음.

#### `ses.cookies.set(details, callback)`

* `details` Object
  * `url` String - `url`에 관련된 쿠키를 가져옵니다.
  * `name` String - 쿠키의 이름입니다. 기본적으로 비워두면 생략됩니다.
  * `value` String - 쿠키의 값입니다. 기본적으로 비워두면 생략됩니다.
  * `domain` String - 쿠키의 도메인입니다. 기본적으로 비워두면 생략됩니다.
  * `path` String - 쿠키의 경로입니다. 기본적으로 비워두면 생략됩니다.
  * `secure` Boolean - 쿠키가 안전한 것으로 표시되는지에 대한 여부입니다. 기본값은
    false입니다.
  * `session` Boolean - 쿠키가 HttpOnly로 표시되는지에 대한 여부입니다. 기본값은
    false입니다.
  * `expirationDate` Double (optional) -	UNIX 시간으로 표시되는 쿠키의 만료일에
    대한 초 단위 시간입니다. 세션 쿠키에 제공되지 않습니다.
* `callback` Function

`details` 객체에 따라 쿠키를 설정합니다. 작업이 완료되면 `callback`이
`callback(error)` 형태로 호출됩니다.

#### `ses.cookies.remove(url, name, callback)`

* `url` String - 쿠키와 관련된 URL입니다.
* `name` String - 지울 쿠키의 이름입니다.
* `callback` Function

`url`과 `name`에 일치하는 쿠키를 삭제합니다. 작업이 완료되면 `callback`이
`callback()` 형식으로 호출됩니다.

#### `ses.getCacheSize(callback)`

* `callback` Function
  * `size` Integer - 바이트로 표현된 캐시 크기

세션의 현재 캐시 크기를 반환합니다.

#### `ses.clearCache(callback)`

* `callback` Function - 작업이 완료되면 호출됩니다.

세션의 HTTP 캐시를 비웁니다.

#### `ses.clearStorageData([options, ]callback)`

* `options` Object (optional), proprties:
  * `origin` String - `scheme://host:port`와 같은 `window.location.origin` 규칙을
    따르는 origin 문자열.
  * `storages` Array - 비우려는 스토리지의 종류, 다음과 같은 타입을 포함할 수 있습니다:
    `appcache`, `cookies`, `filesystem`, `indexdb`, `local storage`,
    `shadercache`, `websql`, `serviceworkers`
  * `quotas` Array - 비우려는 할당의 종류, 다음과 같은 타입을 포함할 수 있습니다:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - 작업이 완료되면 호출됩니다.

웹 스토리지의 데이터를 비웁니다.

#### `ses.flushStorageData()`

디스크에 사용되지 않은 DOMStorage 데이터를 모두 덮어씌웁니다.

#### `ses.setProxy(config, callback)`

* `config` Object
  * `pacScript` String - PAC 파일과 관련된 URL입니다.
  * `proxyRules` String - 사용할 프록시의 규칙을 나타냅니다.
* `callback` Function - 작업이 완료되면 호출됩니다.

프록시 설정을 적용합니다.

`pacScript`와 `proxyRules`이 같이 제공되면 `proxyRules` 옵션은 무시되며 `pacScript`
컨픽만 적용됩니다.

`proxyRules`는 다음과 같은 규칙을 따릅니다:

```
proxyRules = schemeProxies[";"<schemeProxies>]
schemeProxies = [<urlScheme>"="]<proxyURIList>
urlScheme = "http" | "https" | "ftp" | "socks"
proxyURIList = <proxyURL>[","<proxyURIList>]
proxyURL = [<proxyScheme>"://"]<proxyHost>[":"<proxyPort>]
```

예시:

* `http=foopy:80;ftp=foopy2` - http:// URL에 `foopy:80` HTTP 프록시를 사용합니다.
  `foopy2:80` 는 ftp:// URL에 사용됩니다.
* `foopy:80` - 모든 URL에 `foopy:80` 프록시를 사용합니다.
* `foopy:80,bar,direct://` - 모든 URL에 `foopy:80` HTTP 프록시를 사용합니다.
  문제가 발생하여 `foopy:80`를 사용할 수 없는 경우 `bar`를 대신 사용하여 장애를
  복구하며 그 다음 문제가 생긴 경우 프록시를 사용하지 않습니다.
* `socks4://foopy` - 모든 URL에 `foopy:1000` SOCKS v4 프록시를 사용합니다.
* `http=foopy,socks5://bar.com` - http:// URL에 `foopy` HTTP 프록시를 사용합니다.
  문제가 발생하여 `foopy`를 사용할 수 없는 경우 SOCKS5 `bar.com` 프록시를 대신
  사용합니다.
*  `http=foopy,direct://` - http:// URL에 `foopy` HTTP 프록시를 사용합니다. 그리고
  문제가 발생하여 `foopy`를 사용할 수 없는 경우 프록시를 사용하지 않습니다.
* `http=foopy;socks=foopy2` - http:// URL에 `foopy` HTTP 프록시를 사용합니다.
  그리고 `socks4://foopy2` 프록시를 다른 모든 URL에 사용합니다.

### `app.resolveProxy(url, callback)`

* `url` URL
* `callback` Function

`url`의 프록시 정보를 해석합니다. `callback`은 요청이 수행되었을 때
`callback(proxy)` 형태로 호출됩니다.

#### `ses.setDownloadPath(path)`

* `path` String - 다운로드 위치

다운로드 저장 위치를 지정합니다. 기본 다운로드 위치는 각 어플리케이션 데이터 디렉터리의
`Downloads` 폴더입니다.

#### `ses.enableNetworkEmulation(options)`

* `options` Object
  * `offline` Boolean - 네트워크의 오프라인 상태 여부
  * `latency` Double - 밀리세컨드 단위의 RTT
  * `downloadThroughput` Double - Bps 단위의 다운로드 주기
  * `uploadThroughput` Double - Bps 단위의 업로드 주기

제공된 설정으로 `session`의 네트워크를 에뮬레이트합니다.

```javascript
// 50kbps의 처리량과 함께 500ms의 레이턴시로 GPRS 연결을 에뮬레이트합니다.
window.webContents.session.enableNetworkEmulation({
    latency: 500,
    downloadThroughput: 6400,
    uploadThroughput: 6400
});

// 네트워크가 끊긴 상태를 에뮬레이트합니다.
window.webContents.session.enableNetworkEmulation({offline: true});
```

#### `ses.disableNetworkEmulation()`

활성화된 `session`의 에뮬레이션을 비활성화합니다. 기본 네트워크 설정으로 돌아갑니다.

#### `ses.setCertificateVerifyProc(proc)`

 * `proc` Function

`session`에 인증서의 유효성을 확인하는 프로세스(proc)를 등록합니다. `proc`은 서버
인증서 유효성 검증 요청이 들어왔을 때 언제나 `proc(hostname, certificate, callback)`
형식으로 호출됩니다. `callback(true)`을 호출하면 인증을 승인하고 `callback(false)`를
호출하면 인증을 거부합니다.

`setCertificateVerifyProc(null)`을 호출하면 기본 검증 프로세스로 되돌립니다.

```javascript
myWindow.webContents.session.setCertificateVerifyProc(function(hostname, cert, callback) {
 if (hostname == 'github.com')
   callback(true);
 else
   callback(false);
});
```
#### `ses.setPermissionRequestHandler(handler)`

* `handler` Function
  * `webContents` Object - [WebContents](web-contents.md) 권한을 요청.
  * `permission`  String - 'media', 'geolocation', 'notifications',
    'midiSysex', 'pointerLock', 'fullscreen', 'openExternal'의 나열.
  * `callback`  Function - 권한 허용 및 거부.

`session`의 권한 요청에 응답을 하는데 사용하는 핸들러를 설정합니다.
`callback(true)`를 호출하면 권한 제공을 허용하고 `callback(false)`를
호출하면 권한 제공을 거부합니다.

```javascript
session.fromPartition(partition).setPermissionRequestHandler(function(webContents, permission, callback) {
  if (webContents.getURL() === host) {
    if (permission == "notifications") {
      callback(false); // 거부됨.
      return;
    }
  }

  callback(true);
});
```

#### `ses.clearHostResolverCache([callback])`

* `callback` Function (optional) - 작업이 완료되면 호출됩니다.

호스트 리소버(resolver) 캐시를 지웁니다.

#### `ses.webRequest`

`webRequest` API는 생명주기의 다양한 단계에 맞춰 요청 컨텐츠를 가로채거나 변경할 수
있도록 합니다.

각 API는 `filter`와 `listener`를 선택적으로 받을 수 있습니다. `listener`는 API의
이벤트가 발생했을 때 `listener(details)` 형태로 호출되며 `defails`는 요청을 묘사하는
객체입니다. `listener`에 `null`을 전달하면 이벤트 수신을 중지합니다.

`filter`는 `urls` 속성을 가진 객체입니다. 이 속성은 URL 규칙의 배열이며 URL 규칙에
일치하지 않는 요청을 모두 거르는데 사용됩니다. 만약 `filter`가 생략되면 모든 요청을
여과 없이 통과시킵니다.

어떤 `listener`의 이벤트들은 `callback`을 같이 전달하는데, 이벤트 처리시
`listener`의 작업을 완료한 후 `response` 객체를 포함하여 호출해야 합니다.

```javascript
// 다음 url에 대한 User Agent를 조작합니다.
var filter = {
  urls: ["https://*.github.com/*", "*://electron.github.io"]
};

session.defaultSession.webRequest.onBeforeSendHeaders(filter, function(details, callback) {
  details.requestHeaders['User-Agent'] = "MyAgent";
  callback({cancel: false, requestHeaders: details.requestHeaders});
});
```

#### `ses.webRequest.onBeforeRequest([filter, ]listener)`

* `filter` Object
* `listener` Function

요청이 발생하면 `listener`가 `listener(details, callback)` 형태로 호출됩니다.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `uploadData` Array (optional)
* `callback` Function

`uploadData`는 `data` 객체의 배열입니다:

* `data` Object
  * `bytes` Buffer - 전송될 컨텐츠.
  * `file` String - 업로드될 파일의 경로.

`callback`은 `response` 객체와 함께 호출되어야 합니다:

* `response` Object
  * `cancel` Boolean (optional)
  * `redirectURL` String (optional) - 원래 요청은 전송과 완료가 방지되지만 이
    속성을 지정하면 해당 URL로 리다이렉트됩니다.

#### `ses.webRequest.onBeforeSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

HTTP 요청을 보내기 전 요청 헤더를 사용할 수 있을 때 `listener`가
`listener(details, callback)` 형태로 호출됩니다. 이 이벤트는 서버와의 TCP 연결이
완료된 후에 발생할 수도 있지만 http 데이터가 전송되기 전에 발생합니다.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object
* `callback` Function

`callback`은 `response` 객체와 함께 호출되어야 합니다:

* `response` Object
  * `cancel` Boolean (optional)
  * `requestHeaders` Object (optional) - 이 속성이 제공되면, 요청은 이 헤더로
    만들어 집니다.

#### `ses.webRequest.onSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

서버에 요청이 전송되기 바로 전에 `listener`가 `listener(details)` 형태로 호출됩니다.
이전 `onBeforeSendHeaders`의 response와 다른점은 리스너가 호출되는 시간으로 볼 수
있습니다.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object

#### `ses.webRequest.onHeadersReceived([filter, ]listener)`

* `filter` Object
* `listener` Function

요청의 HTTP 응답 헤더를 받았을 때 `listener`가 `listener(details, callback)` 형태로
호출됩니다.

* `details` Object
  * `id` String
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `statusLine` String
  * `statusCode` Integer
  * `responseHeaders` Object
* `callback` Function

`callback`은 `response` 객체와 함께 호출되어야 합니다:

* `response` Object
  * `cancel` Boolean
  * `responseHeaders` Object (optional) - 이 속성이 제공되면 서버는 이 헤더와
    함께 응답합니다.
  * `statusLine` String (optional) - `responseHeaders`를 덮어쓸 땐, 헤더의 상태를
    변경하기 위해 반드시 지정되어야 합니다. 그렇지 않은 경우, 기존의 응답 헤더의 상태가
    사용됩니다.

#### `ses.webRequest.onResponseStarted([filter, ]listener)`

* `filter` Object
* `listener` Function

요청 본문의 첫 번째 바이트를 받았을 때 `listener`가 `listener(details)` 형태로
호출됩니다. 이는 HTTP 요청에서 상태 줄과 요청 헤더가 사용 가능한 상태를 의미합니다.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `responseHeaders` Object
  * `fromCache` Boolean  - 응답을 디스크 캐시에서 가져올지에 대한 여부.
  * `statusCode` Integer
  * `statusLine` String

#### `ses.webRequest.onBeforeRedirect([filter, ]listener)`

* `filter` Object
* `listener` Function

서버에서 시작된 리다이렉트가 발생했을 때 `listener`가 `listener(details)` 형태로
호출됩니다.

* `details` Object
  * `id` String
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `redirectURL` String
  * `statusCode` Integer
  * `ip` String (optional) - 요청이 실질적으로 전송될 서버 아이피 주소.
  * `fromCache` Boolean
  * `responseHeaders` Object

#### `ses.webRequest.onCompleted([filter, ]listener)`

* `filter` Object
* `listener` Function

요청이 완료되면 `listener`가 `listener(details)` 형태로 호출됩니다.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `responseHeaders` Object
  * `fromCache` Boolean
  * `statusCode` Integer
  * `statusLine` String

#### `ses.webRequest.onErrorOccurred([filter, ]listener)`

* `filter` Object
* `listener` Function

에러가 발생하면 `listener`가 `listener(details)` 형태로 호출됩니다.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `fromCache` Boolean
  * `error` String - 에러 설명.
