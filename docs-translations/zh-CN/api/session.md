# session

> 管理浏览器会话，Cookie，缓存，代理设置等。

进程： [Main](../glossary.md#main-process)

`session` 模块可以用来创建一个新的 `Session` 对象。

你也可以通过使用 [`webContents`](web-contents.md) 的属性 `session` 来使用一个已有页面的 `session` ，`webContents` 是[`BrowserWindow`](browser-window.md) 的属性。

```javascript
const {BrowserWindow} = require('electron')

let win = new BrowserWindow({width: 800, height: 600})
win.loadURL('http://github.com')

const ses = win.webContents.session
console.log(ses.getUserAgent())
```

## 方法

`session` 模块有如下方法：

### `session.fromPartition(partition[, options])`

* `partition` String
* `options` Object
  * `cache` Boolean - 是否启用缓存。

从字符串 `partition` 返回一个新的 `Session` 实例。

返回 `Session` - 一个来自 `partition` 字符串的会话实例。当存在时
`Session` 与同一个 `partition`，它会被返回；否则一个新
`Session` 实例将使用 `options` 创建。

如果 `partition` 以 `persist:` 开头，那么这个 page 将使用一个持久的 session，这个 session 将对应用的所有 page 可用。如果没前缀，这个 page 将使用一个历史 session。如果 `partition` 为空，那么将返回应用的默认 session。

要用 `options` 创建一个 `Session`，你必须确保 `Session` 与
`partition` 从来没有被使用过。没有办法改变现有 `Session` 对象的 `options'。

## 属性

`session` 模块有如下属性：

### session.defaultSession

返回应用的默认 `Session` 对象。

## Class: Session

> 获取和设置会话的属性。

进程: [Main](../glossary.md#main-process)

可以在 `session` 模块中创建一个 `Session` 对象：

```javascript
const {session} = require('electron')
const ses = session.fromPartition('persist:name')
console.log(ses.getUserAgent())
```

### 实例事件

实例 `Session` 有以下事件:

#### Event: 'will-download'

* `event` Event
* `item` [DownloadItem](download-item.md)
* `webContents` [WebContents](web-contents.md)

当 Electron 将要从 `webContents` 下载 `item` 时触发.

调用 `event.preventDefault()` 可以取消下载，并且在进程的下个 tick 中，这个 `item` 也不可用。

```javascript
const {session} = require('electron')
session.defaultSession.on('will-download', (event, item, webContents) => {
  event.preventDefault()
  require('request')(item.getURL(), (data) => {
    require('fs').writeFileSync('/somewhere', data)
  })
})
```

### 实例方法

实例 `Session` 有以下方法：


#### `ses.getCacheSize(callback)`

* `callback` Function
  * `size` Integer - 单位 bytes 的缓存 size.

返回 session 的当前缓存大小 .

#### `ses.clearCache(callback)`

* `callback` Function - 操作完成时调用

清空 session 的 HTTP 缓存.

#### `ses.clearStorageData([options, ]callback)`

* `options` Object (可选)
  * `origin` String - 应当遵循 `window.location.origin` 的格式
    `scheme://host:port`.
  * `storages` String[] - 需要清理的 storages 类型, 可以包含 :
    `appcache`, `cookies`, `filesystem`, `indexdb`, `localstorage`,
    `shadercache`, `websql`, `serviceworkers`
  * `quotas` String[] - 需要清理的类型指标, 可以包含:
    `temporary`, `persistent`, `syncable`.
* `callback` Function - 操作完成时调用.

清除 web storages 的数据.

#### `ses.flushStorageData()`

将没有写入的 DOMStorage 写入磁盘.

#### `ses.setProxy(config, callback)`

* `config` Object
  * `pacScript` String - 与 PAC 文件相关的 URL.
  * `proxyRules` String - 代理使用规则.
  * `proxyBypassRules` String - 标明哪些 URL 可以绕过代理设置的规则。
* `callback` Function - 操作完成时调用.

设置 proxy.

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


The `proxyBypassRules` is a comma separated list of rules described below:

* `[ URL_SCHEME "://" ] HOSTNAME_PATTERN [ ":" <port> ]`

   Match all hostnames that match the pattern HOSTNAME_PATTERN.

   Examples:
     "foobar.com", "*foobar.com", "*.foobar.com", "*foobar.com:99",
     "https://x.*.y.com:99"

 * `"." HOSTNAME_SUFFIX_PATTERN [ ":" PORT ]`

   Match a particular domain suffix.

   Examples:
     ".google.com", ".com", "http://.google.com"

* `[ SCHEME "://" ] IP_LITERAL [ ":" PORT ]`

   Match URLs which are IP address literals.

   Examples:
     "127.0.1", "[0:0::1]", "[::1]", "http://[::1]:99"

*  `IP_LITERAL "/" PREFIX_LENGHT_IN_BITS`

   Match any URL that is to an IP literal that falls between the
   given range. IP range is specified using CIDR notation.

   Examples:
     "192.168.1.1/16", "fefe:13::abc/33".

*  `<local>`

   Match local addresses. The meaning of `<local>` is whether the
   host matches one of: "127.0.0.1", "::1", "localhost".
   
### `ses.resolveProxy(url, callback)`

* `url` URL
* `callback` Function
  * `proxy` String

解析 `url` 的代理信息.当请求完成的时候使用 `callback(proxy)` 调用 `callback`.

#### `ses.setDownloadPath(path)`

* `path` String - 下载地址

设置下载保存地址，默认保存地址为各自 app 应用的 `Downloads`目录.

#### `ses.enableNetworkEmulation(options)`

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
})

// 模拟网络故障.
window.webContents.session.enableNetworkEmulation({offline: true})
```

#### `ses.disableNetworkEmulation()`

停止所有已经使用 `session` 的活跃模拟网络.
重置为原始网络类型.

#### `ses.setCertificateVerifyProc(proc)`


* `proc` Function
  * `request` Object
    * `hostname` String
    * `certificate` [Certificate](structures/certificate.md)
    * `error` String - 从 chromium 获得验证结果.
  * `callback` Function
    * `verificationResult` Integer - 证书的错误码请看[这里](https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h).除了证书错误代码外，还可以使用以下特殊代码.
      * `0` - Indicates success and disables Certificate Transperancy verification.
      * `-2` - 表明失败.
      * `-3` - 使用 chromium 的验证结果.

为 `session` 设置证书验证过程，当请求一个服务器的证书验证时，使用 `proc(request, callback)` 来调用 `proc`.调用 `callback(0)` 来接收证书，调用  `callback(-2)` 来拒绝验证证书.

调用了 `setCertificateVerifyProc(null)` ，则将会回复到默认证书验证过程.

```javascript
const {BrowserWindow} = require('electron')
let win = new BrowserWindow()

win.webContents.session.setCertificateVerifyProc((request, callback) => {
  const {hostname} = request
  if (hostname === 'github.com') {
    callback(0)
  } else {
    callback(-2)
  }
})
```

#### `ses.setPermissionRequestHandler(handler)`

* `handler` Function
  * `webContents` Object - [WebContents](web-contents.md) 请求许可.
  * `permission`  String - 枚举了 'media', 'geolocation', 'notifications', 'midiSysex','pointerLock', 'fullscreen', 'openExternal.
  * `callback`  Function 
    * `permissionGranted` Boolean - 允许或禁止许可.

为对应 `session` 许可请求设置响应句柄.调用 `callback(true)` 接收许可，调用 `callback(false)` 禁止许可.

```javascript
const {session} = require('electron')
session.fromPartition('some-partition').setPermissionRequestHandler((webContents, permission, callback) => {
  if (webContents.getURL() === 'some-host' && permission === 'notifications') {
    return callback(false) // denied.
  }

  callback(true)
})
```

#### `ses.clearHostResolverCache([callback])`

* `callback` Function (可选) - 操作结束调用.

清除主机解析缓存.

#### `ses.allowNTLMCredentialsForDomains(domains)`

* `domains` String - 启用集成身份验证服务器列表，使用逗号隔开.

动态设置是否总是给 HTTP NTLM 或 Negotiate authentication 发送证书
```javascript
const {session} = require('electron')
// consider any url ending with `example.com`, `foobar.com`, `baz`
// for integrated authentication.
session.defaultSession.allowNTLMCredentialsForDomains('*example.com, *foobar.com, *baz')

// consider all urls for integrated authentication.
session.defaultSession.allowNTLMCredentialsForDomains('*')
```

#### `ses.setUserAgent(userAgent[, acceptLanguages])`

* `userAgent` String
* `acceptLanguages` String (optional)

覆盖当前会话中的 `userAgent` 和 `acceptLanguages`.

`acceptLanguages` 必须是用逗号分隔的语言代码列表，例如`"en-US,fr,de,ko,zh-CN,ja"`.

这不影响现有的 `WebContents`, 每个 `WebContents` 可以使用 `webContents.setUserAgent` 去覆盖会话范围内的用户代理.

#### `ses.getUserAgent()`

Returns `String` - 当前会话的用户代理.

#### `ses.getBlobData(identifier, callback)`

* `identifier` String - Valid UUID.
* `callback` Function
  * `result` Buffer - Blob data.

Returns `Blob` - The blob data associated with the `identifier`.

#### `ses.createInterruptedDownload(options)`

* `options` Object
  * `path` String - Absolute path of the download.
  * `urlChain` String[] - Complete URL chain for the download.
  * `mimeType` String (optional)
  * `offset` Integer - Start range for the download.
  * `length` Integer - Total length of the download.
  * `lastModified` String - Last-Modified header value.
  * `eTag` String - ETag header value.
  * `startTime` Double (optional) - Time when download was started in
    number of seconds since UNIX epoch.

Allows resuming `cancelled` or `interrupted` downloads from previous `Session`.
The API will generate a [DownloadItem](download-item.md) that can be accessed with the [will-download](#event-will-download)
event. The [DownloadItem](download-item.md) will not have any `WebContents` associated with it and
the initial state will be `interrupted`. The download will start only when the
`resume` API is called on the [DownloadItem](download-item.md).

#### `ses.clearAuthCache(options[, callback])`

* `options` ([RemovePassword](structures/remove-password.md) | [RemoveClientCertificate](structures/remove-client-certificate.md))
* `callback` Function (optional) - Called when operation is done

Clears the session’s HTTP authentication cache.

### 实例属性

下列为 `Session` 实例中存在的属性:

#### `ses.cookies`

当前会话的 [Cookies](cookies.md) 对象.

#### `ses.webRequest`

当前会话的 [WebRequest](web-request.md) 对象.

#### `ses.protocol`

当前会话的 [Protocol](protocol.md) 对象.

```javascript
const {app, session} = require('electron')
const path = require('path')

app.on('ready', function () {
  const protocol = session.fromPartition('some-partition').protocol
  protocol.registerFileProtocol('atom', function (request, callback) {
    var url = request.url.substr(7)
    callback({path: path.normalize(`${__dirname}/${url}`)})
  }, function (error) {
    if (error) console.error('Failed to register protocol')
  })
})
```
