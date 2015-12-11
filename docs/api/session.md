# session

The `session` module can be used to create new `Session` objects.

You can also access the `session` of existing pages by using the `session`
property of [`webContents`](web-contents.md) which is a property of
[`BrowserWindow`](browser-window.md).

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadURL("http://github.com");

var ses = win.webContents.session;
```

## Methods

The `session` module has the following methods:

### session.fromPartition(partition)

* `partition` String

Returns a new `Session` instance from `partition` string.

If `partition` starts with `persist:`, the page will use a persistent session
available to all pages in the app with the same `partition`. if there is no
`persist:` prefix, the page will use an in-memory session. If the `partition` is
empty then default session of the app will be returned.

## Properties

The `session` module has the following properties:

### session.defaultSession

Returns the default session object of the app.

## Class: Session

You can create a `Session` object in the `session` module:

```javascript
const session = require('electron').session;

var ses = session.fromPartition('persist:name');
```

### Instance Events

The following events are available on instances of `Session`:

#### Event: 'will-download'

* `event` Event
* `item` [DownloadItem](download-item.md)
* `webContents` [WebContents](web-contents.md)

Emitted when Electron is about to download `item` in `webContents`.

Calling `event.preventDefault()` will cancel the download.

```javascript
session.on('will-download', function(event, item, webContents) {
  event.preventDefault();
  require('request')(item.getURL(), function(data) {
    require('fs').writeFileSync('/somewhere', data);
  });
});
```

### Instance Methods

The following methods are available on instances of `Session`:

#### `ses.cookies`

The `cookies` gives you ability to query and modify cookies. For example:

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600 });

win.loadURL('https://github.com');

win.webContents.on('did-finish-load', function() {
  // Query all cookies.
  win.webContents.session.cookies.get({}, function(error, cookies) {
    if (error) throw error;
    console.log(cookies);
  });

  // Query all cookies associated with a specific url.
  win.webContents.session.cookies.get({ url : "http://www.github.com" },
      function(error, cookies) {
        if (error) throw error;
        console.log(cookies);
  });

  // Set a cookie with the given cookie data;
  // may overwrite equivalent cookies if they exist.
  win.webContents.session.cookies.set(
    { url : "http://www.github.com", name : "dummy_name", value : "dummy"},
    function(error, cookies) {
      if (error) throw error;
      console.log(cookies);
  });
});
```

#### `ses.cookies.get(details, callback)`

`details` Object, properties:

* `url` String - Retrieves cookies which are associated with `url`.
  Empty implies retrieving cookies of all urls.
* `name` String - Filters cookies by name
* `domain` String - Retrieves cookies whose domains match or are subdomains of
  `domains`
* `path` String - Retrieves cookies whose path matches `path`
* `secure` Boolean - Filters cookies by their Secure property
* `session` Boolean - Filters out session or persistent cookies.
* `callback` Function - function(error, cookies)
* `error` Error
* `cookies` Array - array of `cookie` objects, properties:
  *  `name` String - The name of the cookie.
  *  `value` String - The value of the cookie.
  *  `domain` String - The domain of the cookie.
  *  `host_only` String - Whether the cookie is a host-only cookie.
  *  `path` String - The path of the cookie.
  *  `secure` Boolean - Whether the cookie is marked as Secure (typically HTTPS).
  *  `http_only` Boolean - Whether the cookie is marked as HttpOnly.
  *  `session` Boolean - Whether the cookie is a session cookie or a persistent
     cookie with an expiration date.
  *  `expirationDate` Double - (Option) The expiration date of the cookie as
     the number of seconds since the UNIX epoch. Not provided for session
     cookies.

#### `ses.cookies.set(details, callback)`

`details` Object, properties:

* `url` String - Retrieves cookies which are associated with `url`
* `name` String - The name of the cookie. Empty by default if omitted.
* `value` String - The value of the cookie. Empty by default if omitted.
* `domain` String - The domain of the cookie. Empty by default if omitted.
* `path` String - The path of the cookie. Empty by default if omitted.
* `secure` Boolean - Whether the cookie should be marked as Secure. Defaults to
  false.
* `session` Boolean - Whether the cookie should be marked as HttpOnly. Defaults
  to false.
* `expirationDate` Double -	The expiration date of the cookie as the number of
  seconds since the UNIX epoch. If omitted, the cookie becomes a session cookie.

* `callback` Function - function(error)
  * `error` Error

#### `ses.cookies.remove(details, callback)`

* `details` Object
  * `url` String - The URL associated with the cookie
  * `name` String - The name of cookie to remove
* `callback` Function - function(error)
  * `error` Error

#### `ses.clearCache(callback)`

* `callback` Function - Called when operation is done

Clears the session’s HTTP cache.

#### `ses.clearStorageData([options, ]callback)`

* `options` Object (optional)
  * `origin` String - Should follow `window.location.origin`’s representation
    `scheme://host:port`.
  * `storages` Array - The types of storages to clear, can contain:
    `appcache`, `cookies`, `filesystem`, `indexdb`, `local storage`,
    `shadercache`, `websql`, `serviceworkers`
  * `quotas` Array - The types of quotas to clear, can contain:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - Called when operation is done.

Clears the data of web storages.

#### `ses.setProxy(config, callback)`

* `config` String
* `callback` Function - Called when operation is done.

If `config` is a PAC url, it is used directly otherwise
`config` is parsed based on the following rules indicating which
proxies to use for the session.

```
config = scheme-proxies[";"<scheme-proxies>]
scheme-proxies = [<url-scheme>"="]<proxy-uri-list>
url-scheme = "http" | "https" | "ftp" | "socks"
proxy-uri-list = <proxy-uri>[","<proxy-uri-list>]
proxy-uri = [<proxy-scheme>"://"]<proxy-host>[":"<proxy-port>]

  For example:
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

### `ses.resolveProxy(url, callback)`

* `url` URL
* `callback` Function

Resolves the proxy information for `url`. The `callback` will be called with
`callback(proxy)` when the request is performed.

#### `ses.setDownloadPath(path)`

* `path` String - The download location

Sets download saving directory. By default, the download directory will be the
`Downloads` under the respective app folder.

#### `ses.enableNetworkEmulation(options)`

* `options` Object
  * `offline` Boolean - Whether to emulate network outage.
  * `latency` Double - RTT in ms
  * `downloadThroughput` Double - Download rate in Bps
  * `uploadThroughput` Double - Upload rate in Bps

Emulates network with the given configuration for the `session`.

```javascript
// To emulate a GPRS connection with 50kbps throughput and 500 ms latency.
window.webContents.session.enableNetworkEmulation({
    latency: 500,
    downloadThroughput: 6400,
    uploadThroughput: 6400
});

// To emulate a network outage.
window.webContents.session.enableNetworkEmulation({offline: true});
```

#### `ses.disableNetworkEmulation()`

Disables any network emulation already active for the `session`. Resets to
the original network configuration.

#### `ses.setCertificateVerifyProc(proc)`

* `proc` Function

Sets the certificate verify proc for `session`, the `proc` will be called with
`proc(hostname, certificate, callback)` whenever a server certificate
verification is requested. Calling `callback(true)` accepts the certificate,
calling `callback(false)` rejects it.

Calling `setCertificateVerifyProc(null)` will revert back to default certificate
verify proc.

```javascript
myWindow.webContents.session.setCertificateVerifyProc(function(hostname, cert, callback) {
  if (hostname == 'github.com')
    callback(true);
  else
    callback(false);
});
```

#### `ses.webRequest`

The `webRequest` API set allows to intercept and modify contents of a request at
various stages of its lifetime.

Each API accepts an optional `filter` and a `listener`, the `listener` will be
called with `listener(details)` when the API's event has happened, the `details`
is an object that describes the request. Passing `null` as `listener` will
unsubscribe from the event.

The `filter` is an object that has an `urls` property, which is an Array of URL
patterns that will be used to filter out the requests that do not match the URL
patterns. If the `filter` is omitted then all requests will be matched.

For certain events the `listener` is required to return an object that describes
how to handle the request, users can specify the `cancel` property to `true` to
cancel the request.

```javascript
// Modify the user agent for all requests to the following urls.
var filter = {
  urls: ["https://*.github.com/*", "*://electron.github.io"]
};

session.defaultSession.webRequest.onBeforeSendHeaders(filter, function(details) {
  details.requestHeaders['User-Agent'] = "MyAgent";
  return {cancel: false, requestHeaders: details.requestHeaders};
});
```

#### `ses.webRequest.onBeforeRequest([filter, ]listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` when a request is about
to occur.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double

The `listener` has to return an `response` object:

* `response` Object
  * `cancel` Boolean
  * `redirectURL` String __optional__ - The original request is prevented from
    being sent or completed, and is instead redirected to the given URL.

#### `ses.webRequest.onBeforeSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` before sending an HTTP
request, once the request headers are available. This may occur after a TCP
connection is made to the server, but before any http data is sent.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object

The `listener` has to return an `response` object:

* `response` Object
  * `cancel` Boolean
  * `requestHeaders` Object __optional__ - When provided, request will be made
    with these headers.

#### `ses.webRequest.onSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` just before a request is
going to be sent to the server, modifications of previous `onBeforeSendHeaders`
response are visible by the time this listener is fired.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object

#### `ses.webRequest.onHeadersReceived([filter,] listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` when HTTP response
headers of a request have been received.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `statusLine` String
  * `statusCode` Integer
  * `responseHeaders` Object

The `listener` has to return an `response` object:

* `response` Object
  * `cancel` Boolean
  * `responseHeaders` Object __optional__ - When provided, the server is assumed
    to have responded with these headers.

#### `ses.webRequest.onResponseStarted([filter, ]listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` when first byte of the
response body is received. For HTTP requests, this means that the status line
and response headers are available.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `responseHeaders` Object
  * `fromCache` Boolean  - Indicates whether the response was fetched from disk
    cache.
  * `statusCode` Integer
  * `statusLine` String

#### `ses.webRequest.onBeforeRedirect([filter, ]listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` when a server initiated
redirect is about to occur.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `redirectURL` String
  * `statusCode` Integer
  * `ip` String __optional__ - The server IP address that the request was
    actually sent to.
  * `fromCache` Boolean
  * `responseHeaders` Object

#### `ses.webRequest.onCompleted([filter, ]listener)`

* `filter` Object
* `listener` Function

The `listener` will be called with `listener(details)` when a request is
completed.

* `details` Object
  * `id` String - Request ID.
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

The `listener` will be called with `listener(details)` when an error occurs.

* `details` Object
  * `id` String - Request ID.
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `fromCache` Boolean
  * `error` String - The error description.
