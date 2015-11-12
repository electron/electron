# session

The `session` object is a property of [`webContents`](web-contents.md) which is
a property of [`BrowserWindow`](browser-window.md). You can access it through an
instance of `BrowserWindow`. For example:

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

Fired when Electron is about to download `item` in `webContents`.

Calling `event.preventDefault()` will cancel the download.

```javascript
session.on('will-download', function(event, item, webContents) {
  event.preventDefault();
  require('request')(item.getUrl(), function(data) {
    require('fs').writeFileSync('/somewhere', data);
  });
});
```

## Methods

The `session` object has the following methods:

### `session.cookies`

The `cookies` gives you ability to query and modify cookies. For example:

```javascript
var BrowserWindow = require('browser-window');

var win = new BrowserWindow({ width: 800, height: 600 });

win.loadUrl('https://github.com');

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

### `session.cookies.get(details, callback)`

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

### `session.cookies.set(details, callback)`

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

### `session.cookies.remove(details, callback)`

* `details` Object, proprties:
  * `url` String - The URL associated with the cookie
  * `name` String - The name of cookie to remove
* `callback` Function - function(error)
  * `error` Error

### `session.clearCache(callback)`

* `callback` Function - Called when operation is done

Clears the session’s HTTP cache.

### `session.clearStorageData([options, ]callback)`

* `options` Object (optional), proprties:
  * `origin` String - Should follow `window.location.origin`’s representation
    `scheme://host:port`.
  * `storages` Array - The types of storages to clear, can contain:
    `appcache`, `cookies`, `filesystem`, `indexdb`, `local storage`,
    `shadercache`, `websql`, `serviceworkers`
  * `quotas` Array - The types of quotas to clear, can contain:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - Called when operation is done.

Clears the data of web storages.

### `session.setProxy(config, callback)`

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

### `session.setDownloadPath(path)`

* `path` String - The download location

Sets download saving directory. By default, the download directory will be the
`Downloads` under the respective app folder.

### `session.enableNetworkEmulation(options)`

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

### `session.disableNetworkEmulation`

Disables any network emulation already active for the `session`. Resets to
the original network configuration.
