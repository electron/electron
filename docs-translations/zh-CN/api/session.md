# session

`session` 模块可以用来创建一个新的 `Session` 对象.

你也可以通过使用 [`webContents`](web-contents.md) 的属性 `session` 来使用一个已有页面的 `session` ，`webContents` 是[`BrowserWindow`](browser-window.md) 的属性.

```javascript
const BrowserWindow = require('electron').BrowserWindow;

var win = new BrowserWindow({ width: 800, height: 600 });
win.loadURL("http://github.com");

var ses = win.webContents.session;
```

## 方法

`session` 模块有如下方法:

### session.fromPartition(partition)

* `partition` String

从字符串 `partition` 返回一个新的 `Session` 实例.

如果 `partition` 以 `persist:` 开头，那么这个page将使用一个持久的 session，这个 session 将对应用的所有 page 可用.如果没前缀，这个 page 将使用一个历史 session.如果 `partition` 为空，那么将返回应用的默认 session .

## 属性

`session` 模块有如下属性:

### session.defaultSession

返回应用的默认 session 对象.

## Class: Session

可以在 `session` 模块中创建一个 `Session` 对象 :

```javascript
const session = require('electron').session;

var ses = session.fromPartition('persist:name');
```

### 实例事件

实例 `Session` 有以下事件:

### Event: 'will-download'

* `event` Event
* `item` [DownloadItem](download-item.md)
* `webContents` [WebContents](web-contents.md)

当 Electron 将要从 `webContents` 下载 `item` 时触发.

调用 `event.preventDefault()` 可以取消下载，并且在进程的下个 tick中，这个 `item` 也不可用.

```javascript
session.defaultSession.on('will-download', function(event, item, webContents) {
  event.preventDefault();
  require('request')(item.getURL(), function(data) {
    require('fs').writeFileSync('/somewhere', data);
  });
});
```

### 实例方法

实例 `Session` 有以下方法:

### `ses.cookies`

`cookies` 赋予你全力来查询和修改 cookies. 例如:

```javascript
// 查询所有 cookies.
session.defaultSession.cookies.get({}, function(error, cookies) {
  console.log(cookies);
});

// 查询与指定 url 相关的所有 cookies.
session.defaultSession.cookies.get({ url : "http://www.github.com" }, function(error, cookies) {
  console.log(cookies);
});

// 设置 cookie;
// may overwrite equivalent cookies if they exist.
var cookie = { url : "http://www.github.com", name : "dummy_name", value : "dummy" };
session.defaultSession.cookies.set(cookie, function(error) {
  if (error)
    console.error(error);
});
```

### `ses.cookies.get(filter, callback)`

* `filter` Object
  * `url` String (可选) - 与获取 cookies 相关的 
    `url`.不设置的话就是从所有 url 获取 cookies .
  * `name` String (可选) - 通过 name 过滤 cookies.
  * `domain` String (可选) - 获取对应域名或子域名的 cookies .
  * `path` String (可选) - 获取对应路径的 cookies .
  * `secure` Boolean (可选) - 通过安全性过滤 cookies.
  * `session` Boolean (可选) - 过滤掉 session 或 持久的 cookies.
* `callback` Function

发送一个请求，希望获得所有匹配 `details` 的 cookies, 
在完成的时候，将通过 `callback(error, cookies)` 调用 `callback`.

`cookies`是一个 `cookie` 对象.

* `cookie` Object
  *  `name` String - cookie 名.
  *  `value` String - cookie值.
  *  `domain` String - cookie域名.
  *  `hostOnly` String - 是否 cookie 是一个 host-only cookie.
  *  `path` String - cookie路径.
  *  `secure` Boolean - 是否是安全 cookie.
  *  `httpOnly` Boolean - 是否只是 HTTP cookie.
  *  `session` Boolean - cookie 是否是一个 session cookie 或一个带截至日期的持久
     cookie .
  *  `expirationDate` Double (可选) - cookie的截至日期，数值为UNIX纪元以来的秒数. 对session cookies 不提供.

### `ses.cookies.set(details, callback)`

* `details` Object
  * `url` String - 与获取 cookies 相关的 
    `url`.
  * `name` String - cookie 名. 忽略默认为空.
  * `value` String - cookie 值. 忽略默认为空.
  * `domain` String - cookie的域名. 忽略默认为空.
  * `path` String - cookie 的路径. 忽略默认为空.
  * `secure` Boolean - 是否已经进行了安全性标识. 默认为
    false.
  * `session` Boolean - 是否已经 HttpOnly 标识. 默认为 false.
  * `expirationDate` Double -	cookie的截至日期，数值为UNIX纪元以来的秒数. 如果忽略, cookie 变为 session cookie.
* `callback` Function

使用 `details` 设置 cookie, 完成时使用 `callback(error)` 掉哟个 `callback` .

### `ses.cookies.remove(url, name, callback)`

* `url` String - 与 cookies 相关的 
    `url`.
* `name` String - 需要删除的 cookie 名.
* `callback` Function

删除匹配 `url` 和 `name` 的 cookie, 完成时使用 `callback()`调用`callback`.

### `ses.getCacheSize(callback)`

* `callback` Function
  * `size` Integer - 单位 bytes 的缓存 size.

返回 session 的当前缓存 size .

### `ses.clearCache(callback)`

* `callback` Function - 操作完成时调用

清空 session 的 HTTP 缓存.

### `ses.clearStorageData([options, ]callback)`

* `options` Object (可选)
  * `origin` String - 应当遵循 `window.location.origin` 的格式
    `scheme://host:port`.
  * `storages` Array - 需要清理的 storages 类型, 可以包含 :
    `appcache`, `cookies`, `filesystem`, `indexdb`, `local storage`,
    `shadercache`, `websql`, `serviceworkers`
  * `quotas` Array - 需要清理的类型指标, 可以包含:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - 操作完成时调用.

清除 web storages 的数据.

### `ses.flushStorageData()`

将没有写入的 DOMStorage 写入磁盘.

### `ses.setProxy(config, callback)`

* `config` Object
  * `pacScript` String - 与 PAC 文件相关的 URL.
  * `proxyRules` String - 代理使用规则.
* `callback` Function - 操作完成时调用.

设置 proxy settings.

当 `pacScript` 和 `proxyRules` 一同提供时，将忽略 `proxyRules`，并且使用 `pacScript` 配置 .

`proxyRules` 需要遵循下面的规则:

```
proxyRules = schemeProxies[";"<schemeProxies>]
schemeProxies = [<urlScheme>"="]<proxyURIList>
urlScheme = "http" | "https" | "ftp" | "socks"
proxyURIList = <proxyURL>[","<proxyURIList>]
proxyURL = [<proxyScheme>"://"]<proxyHost>[":"<proxyPort>]
```

例子:

* `http=foopy:80;ftp=foopy2` - 为 `http://` URL 使用 HTTP 代理 `foopy:80` , 和为 `ftp://` URL 
  HTTP 代理 `foopy2:80` .
* `foopy:80` - 为所有 URL 使用 HTTP 代理 `foopy:80` .
* `foopy:80,bar,direct://` - 为所有 URL 使用 HTTP 代理 `foopy:80` , 如果 `foopy:80` 不可用，则切换使用  `bar`, 再往后就不使用代理了.
* `socks4://foopy` - 为所有 URL 使用 SOCKS v4 代理 `foopy:1080`.
* `http=foopy,socks5://bar.com` - 为所有 URL 使用 HTTP 代理 `foopy`, 如果 `foopy`不可用，则切换到 SOCKS5 代理 `bar.com`.
* `http=foopy,direct://` - 为所有http url 使用 HTTP 代理，如果 `foopy`不可用，则不使用代理.
* `http=foopy;socks=foopy2` -  为所有http url 使用 `foopy` 代理，为所有其他 url 使用 `socks4://foopy2` 代理.

### `ses.resolveProxy(url, callback)`

* `url` URL
* `callback` Function

解析 `url` 的代理信息.当请求完成的时候使用 `callback(proxy)` 调用 `callback`.

### `ses.setDownloadPath(path)`

* `path` String - 下载地址

设置下载保存地址，默认保存地址为各自 app 应用的 `Downloads`目录.

### `ses.enableNetworkEmulation(options)`

* `options` Object
  * `offline` Boolean - 是否模拟网络故障.
  * `latency` Double - 每毫秒的 RTT 
  * `downloadThroughput` Double - 每 Bps 的下载速率.
  * `uploadThroughput` Double - 每 Bps 的上载速率.

通过给定配置的 `session` 来模拟网络.

```javascript
// 模拟 GPRS 连接，使用的 50kbps 流量，500 毫秒的 rtt.
window.webContents.session.enableNetworkEmulation({
    latency: 500,
    downloadThroughput: 6400,
    uploadThroughput: 6400
});

// 模拟网络故障.
window.webContents.session.enableNetworkEmulation({offline: true});
```

### `ses.disableNetworkEmulation()`

停止所有已经使用 `session` 的活跃模拟网络.
重置为原始网络类型.

### `ses.setCertificateVerifyProc(proc)`

* `proc` Function

为 `session` 设置证书验证过程，当请求一个服务器的证书验证时，使用 `proc(hostname, certificate, callback)` 调用 `proc`.调用 `callback(true)` 来接收证书，调用  `callback(false)` 来拒绝验证证书.

调用了 `setCertificateVerifyProc(null)` ，则将会回复到默认证书验证过程.

```javascript
myWindow.webContents.session.setCertificateVerifyProc(function(hostname, cert, callback) {
  if (hostname == 'github.com')
    callback(true);
  else
    callback(false);
});
```

### `ses.setPermissionRequestHandler(handler)`

* `handler` Function
  * `webContents` Object - [WebContents](web-contents.md) 请求许可.
  * `permission`  String - 枚举了 'media', 'geolocation', 'notifications', 'midiSysex', 'pointerLock', 'fullscreen'.
  * `callback`  Function - 允许或禁止许可.

为对应 `session` 许可请求设置响应句柄.调用 `callback(true)` 接收许可，调用 `callback(false)` 禁止许可.

```javascript
session.fromPartition(partition).setPermissionRequestHandler(function(webContents, permission, callback) {
  if (webContents.getURL() === host) {
    if (permission == "notifications") {
      callback(false); // denied.
      return;
    }
  }

  callback(true);
});
```

### `ses.clearHostResolverCache([callback])`

* `callback` Function (可选) - 操作结束调用.

清除主机解析缓存.

### `ses.webRequest`

在其生命周期的不同阶段，`webRequest` API 设置允许拦截并修改请求内容.

每个 API 接收一可选的 `filter` 和 `listener`，当 API 事件发生的时候使用 `listener(details)` 调用 `listener`，`details` 是一个用来描述请求的对象.为 `listener` 使用 `null` 则会退定事件.

`filter` 是一个拥有 `urls` 属性的对象，这是一个 url 模式数组，这用来过滤掉不匹配指定 url 模式的请求.如果忽略 `filter` ，那么所有请求都将可以成功匹配.

所有事件的 `listener` 都有一个回调事件，当 `listener` 完成它的工作的时候，它将使用一个 `response` 对象来调用.

```javascript
// 将所有请求的代理都修改为下列 url.
var filter = {
  urls: ["https://*.github.com/*", "*://electron.github.io"]
};

session.defaultSession.webRequest.onBeforeSendHeaders(filter, function(details, callback) {
  details.requestHeaders['User-Agent'] = "MyAgent";
  callback({cancel: false, requestHeaders: details.requestHeaders});
});
```

### `ses.webRequest.onBeforeRequest([filter, ]listener)`

* `filter` Object
* `listener` Function

当一个请求即将开始的时候，使用 `listener(details, callback)` 调用 `listener`.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `uploadData` Array (可选)
* `callback` Function

`uploadData` 是一个 `data` 数组对象:

* `data` Object
  * `bytes` Buffer - 被发送的内容.
  * `file` String - 上载文件路径.

`callback` 必须使用一个 `response` 对象来调用:

* `response` Object
  * `cancel` Boolean (可选)
  * `redirectURL` String (可选) - 原始请求阻止发送或完成，而不是重定向.

### `ses.webRequest.onBeforeSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

一旦请求报文头可用了,在发送 HTTP 请求的之前，使用 `listener(details, callback)` 调用 `listener`.这也许会在服务器发起一个tcp 连接，但是在发送任何 http 数据之前发生.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object
* `callback` Function

必须使用一个 `response` 对象来调用 `callback` :

* `response` Object
  * `cancel` Boolean (可选)
  * `requestHeaders` Object (可选) - 如果提供了,将使用这些 headers 来创建请求.

### `ses.webRequest.onSendHeaders([filter, ]listener)`

* `filter` Object
* `listener` Function

在一个请求正在发送到服务器的时候，使用 `listener(details)` 来调用 `listener` ，之前 `onBeforeSendHeaders` 修改部分响应可用，同时取消监听.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `requestHeaders` Object

### `ses.webRequest.onHeadersReceived([filter,] listener)`

* `filter` Object
* `listener` Function

当 HTTP 请求报文头已经到达的时候，使用 `listener(details, callback)` 调用 `listener` .

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

必须使用一个 `response` 对象来调用 `callback` :

* `response` Object
  * `cancel` Boolean
  * `responseHeaders` Object (可选) - 如果提供, 服务器将假定使用这些头来响应.

### `ses.webRequest.onResponseStarted([filter, ]listener)`

* `filter` Object
* `listener` Function

当响应body的首字节到达的时候，使用 `listener(details)` 调用 `listener`.对 http 请求来说，这意味着状态线和响应头可用了.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `responseHeaders` Object
  * `fromCache` Boolean  - 标识响应是否来自磁盘
    cache.
  * `statusCode` Integer
  * `statusLine` String

### `ses.webRequest.onBeforeRedirect([filter, ]listener)`

* `filter` Object
* `listener` Function

当服务器的重定向初始化正要启动时，使用 `listener(details)` 调用 `listener`.

* `details` Object
  * `id` String
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `redirectURL` String
  * `statusCode` Integer
  * `ip` String (可选) - 请求的真实服务器ip 地址
  * `fromCache` Boolean
  * `responseHeaders` Object

### `ses.webRequest.onCompleted([filter, ]listener)`

* `filter` Object
* `listener` Function

当请求完成的时候，使用 `listener(details)` 调用 `listener`.

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

### `ses.webRequest.onErrorOccurred([filter, ]listener)`

* `filter` Object
* `listener` Function

当一个错误发生的时候，使用 `listener(details)` 调用 `listener`.

* `details` Object
  * `id` Integer
  * `url` String
  * `method` String
  * `resourceType` String
  * `timestamp` Double
  * `fromCache` Boolean
  * `error` String - 错误描述.