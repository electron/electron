# session

`session` 객체는 [`BrowserWindow`](browser-window.md)의 [`webContents`](web-contents.md)의 프로퍼티입니다.
다음과 같이 `BrowserWindow` 인스턴스에서 접근할 수 있습니다:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadUrl("http://github.com");

var session = win.webContents.session
```

## Events

### Event: 'will-download'

* `event` Event
* `item` [DownloadItem](download-item.md)
* `webContents` [WebContents](web-contents.md)

Electron의 `webContents`에서 `item`을 다운로드할 때 발생하는 이벤트입니다.

`event.preventDefault()` 메서드를 호출하면 다운로드를 취소합니다.

```javascript
session.on('will-download', function(event, item, webContents) {
  event.preventDefault();
  require('request')(item.getUrl(), function(data) {
    require('fs').writeFileSync('/somewhere', data);
  });
});
```

## Methods

`session` 객체는 다음과 같은 메서드와 속성을 가지고 있습니다:

### `session.cookies`

`cookies` 속성은 쿠키를 조작하는 방법을 제공합니다. 예를 들어 다음과 같이 할 수 있습니다:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600 });

win.loadUrl('https://github.com');

win.webContents.on('did-finish-load', function() {
  // 모든 쿠키를 가져옵니다.
  win.webContents.session.cookies.get({}, function(error, cookies) {
    if (error) throw error;
    console.log(cookies);
  });

  // Url에 관련된 쿠키를 모두 가져옵니다.
  win.webContents.session.cookies.get({ url : "http://www.github.com" },
      function(error, cookies) {
        if (error) throw error;
        console.log(cookies);
  });

  // 지정한 쿠키 데이터를 설정합니다.
  // 동일한 쿠키가 있으면 해당 쿠키를 덮어씁니다.
  win.webContents.session.cookies.set(
    { url : "http://www.github.com", name : "dummy_name", value : "dummy"},
    function(error, cookies) {
      if (error) throw error;
      console.log(cookies);
  });
});
```

### `session.cookies.get(details, callback)`

`details` Object, properties:

* `url` String - `url`에 관련된 쿠키를 가져옵니다. 이 속성을 비워두면 모든 url의 쿠키를 가져옵니다.
* `name` String - 이름을 기준으로 쿠키를 필터링합니다.
* `domain` String - `domain`과 일치하는 도메인과 서브 도메인에 대한 쿠키를 가져옵니다.
* `path` String - `path`와 일치하는 경로에 대한 쿠키를 가져옵니다.
* `secure` Boolean - 보안 속성을 기준으로 쿠키를 필터링합니다.
* `session` Boolean - 세션 또는 영구 쿠키를 필터링합니다.

* `callback` Function - function(error, cookies)
* `error` Error
* `cookies` Array - `cookie` 객체의 배열, 속성은 다음과 같습니다:
  *  `name` String - 쿠키의 이름.
  *  `value` String - 쿠키의 값.
  *  `domain` String - 쿠키의 도메인.
  *  `host_only` String - 쿠키가 호스트 전용인가에 대한 여부.
  *  `path` String - 쿠키의 경로.
  *  `secure` Boolean - 쿠키가 안전한 것으로 표시되는지에 대한 여부. (일반적으로 HTTPS)
  *  `http_only` Boolean - 쿠키가 HttpOnly로 표시되는지에 대한 여부.
  *  `session` Boolean - 쿠키가 세션 쿠키 또는 만료일이 있는 영구 쿠키인지에 대한 여부.
  *  `expirationDate` Double - (Option) UNIX 시간으로 표시되는 쿠키의 만료일에 대한 초 단위 시간. 세션 쿠키는 지원되지 않음.

### `session.cookies.set(details, callback)`

`details` Object, properties:

* `url` String - `url`에 관련된 쿠키를 가져옵니다. 
* `name` String - 쿠키의 이름입니다. 기본적으로 비워두면 생략됩니다.
* `value` String - 쿠키의 값입니다. 기본적으로 비워두면 생략됩니다.
* `domain` String - 쿠키의 도메인입니다. 기본적으로 비워두면 생략됩니다.
* `path` String - 쿠키의 경로입니다. 기본적으로 비워두면 생략됩니다.
* `secure` Boolean - 쿠키가 안전한 것으로 표시되는지에 대한 여부입니다. 기본값은 false입니다.
* `session` Boolean - 쿠키가 HttpOnly로 표시되는지에 대한 여부입니다. 기본값은 false입니다.
* `expirationDate` Double -	UNIX 시간으로 표시되는 쿠키의 만료일에 대한 초 단위 시간입니다. 생략하면 쿠키는 세션 쿠키가 됩니다.

* `callback` Function - function(error)
  * `error` Error

### `session.cookies.remove(details, callback)`

* `details` Object, proprties:
  * `url` String - 쿠키와 관련된 URL입니다.
  * `name` String - 지울 쿠키의 이름입니다.
* `callback` Function - function(error)
  * `error` Error

### `session.clearCache(callback)`

* `callback` Function - 작업이 완료되면 호출됩니다.

세션의 HTTP 캐시를 비웁니다.

### `session.clearStorageData([options, ]callback)`

* `options` Object (optional), proprties:
  * `origin` String - `scheme://host:port`와 같은 `window.location.origin` 규칙을 따르는 origin 문자열.
  * `storages` Array - 비우려는 스토리지의 종류, 다음과 같은 타입을 포함할 수 있습니다:
    `appcache`, `cookies`, `filesystem`, `indexdb`, `local storage`,
    `shadercache`, `websql`, `serviceworkers`
  * `quotas` Array - 비우려는 할당의 종류, 다음과 같은 타입을 포함할 수 있습니다:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - 작업이 완료되면 호출됩니다.

웹 스토리지의 데이터를 비웁니다.

### `session.setProxy(config, callback)`

* `config` String
* `callback` Function - 작업이 완료되면 호출됩니다.

세션에 사용할 프록시 `config`를 분석하고 프록시를 적용합니다.

세션에 사용할 프록시는 `config`가 PAC 주소일 경우 그대로 적용하고, 다른 형식일 경우 다음 규칙에 따라 적용합니다.

```
config = scheme-proxies[";"<scheme-proxies>]
scheme-proxies = [<url-scheme>"="]<proxy-uri-list>
url-scheme = "http" | "https" | "ftp" | "socks"
proxy-uri-list = <proxy-uri>[","<proxy-uri-list>]
proxy-uri = [<proxy-scheme>"://"]<proxy-host>[":"<proxy-port>]

  예시:
       "http=foopy:80;ftp=foopy2"  -- use HTTP proxy "foopy:80" for http://
                                      URLs, and HTTP proxy "foopy2:80" for
                                      ftp:// URLs.
       "foopy:80"                  -- use HTTP proxy "foopy:80" for all URLs.
       "foopy:80,bar,direct://"    -- use HTTP proxy "foopy:80" for all URLs,
                                      failing over to "bar" if "foopy:80" is
                                      unavailable, and after that using no
                                      proxy.
       "socks4://foopy"            -- use SOCKS v4 proxy "foopy:1080" for all
                                      URLs.
       "http=foopy,socks5://bar.com -- use HTTP proxy "foopy" for http URLs,
                                      and fail over to the SOCKS5 proxy
                                      "bar.com" if "foopy" is unavailable.
       "http=foopy,direct://       -- use HTTP proxy "foopy" for http URLs,
                                      and use no proxy if "foopy" is
                                      unavailable.
       "http=foopy;socks=foopy2   --  use HTTP proxy "foopy" for http URLs,
                                      and use socks4://foopy2 for all other
                                      URLs.
```

### `session.setDownloadPath(path)`

* `path` String - 다운로드 위치

다운로드 저장 위치를 지정합니다. 기본 다운로드 위치는 각 어플리케이션 데이터 디렉터리의 `Downloads` 폴더입니다.

### `session.enableNetworkEmulation(options)`

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

### `session.disableNetworkEmulation`

활성화된 `session`의 에뮬레이션을 비활성화합니다. 기본 네트워크 설정으로 돌아갑니다.
