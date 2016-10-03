# 协议

`protocol` 模块可以注册一个自定义协议，或者使用一个已经存在的协议.

例子，使用一个与 `file://` 功能相似的协议 :

```javascript
const electron = require('electron')
const app = electron.app
const path = require('path')

app.on('ready', function () {
  var protocol = electron.protocol
  protocol.registerFileProtocol('atom', function (request, callback) {
    var url = request.url.substr(7)
    callback({path: path.normalize(__dirname + '/' + url)})
  }, function (error) {
    if (error)
      console.error('Failed to register protocol')
  })
})
```

**注意:** 这个模块只有在 `app` 模块的 `ready` 事件触发之后才可使用.

## 方法

`protocol` 模块有如下方法:

### `protocol.registerStandardSchemes(schemes)`

* `schemes` Array - 将一个自定义的方案注册为标准的方案.

一个标准的 `scheme` 遵循 RFC 3986 的
[generic URI syntax](https://tools.ietf.org/html/rfc3986#section-3) 标准. 这包含了 `file:` 和 `filesystem:`.

### `protocol.registerServiceWorkerSchemes(schemes)`

* `schemes` Array - 将一个自定义的方案注册为处理 service workers.

### `protocol.registerFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

注册一个协议，用来发送响应文件.当通过这个协议来发起一个请求的时候，将使用 `handler(request, callback)` 来调用
`handler` .当 `scheme` 被成功注册或者完成(错误)时失败，将使用 `completion(null)` 调用 `completion`.

* `request` Object
  * `url` String
  * `referrer` String
  * `method` String
  * `uploadData` Array (可选)
* `callback` Function

`uploadData` 是一个 `data` 对象数组:

* `data` Object
  * `bytes` Buffer - 被发送的内容.
  * `file` String - 上传的文件路径.

为了处理请求，调用 `callback` 时需要使用文件路径或者一个带 `path` 参数的对象, 例如 `callback(filePath)` 或
`callback({path: filePath})`.

当不使用任何参数调用 `callback` 时，你可以指定一个数字或一个带有 `error` 参数的对象，来标识 `request` 失败.你可以使用的 error number 可以参考 
[net error list][net-error].

默认 `scheme` 会被注册为一个 `http:` 协议，它与遵循 "generic URI syntax" 规则的协议解析不同，例如 `file:` ，所以你或许应该调用 `protocol.registerStandardSchemes` 来创建一个标准的 scheme.

### `protocol.registerBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

注册一个 `scheme` 协议，用来发送响应 `Buffer` .

这个方法的用法类似 `registerFileProtocol`，除非使用一个 `Buffer` 对象，或一个有 `data`,
`mimeType`, 和 `charset` 属性的对象来调用 `callback` .

例子:

```javascript
protocol.registerBufferProtocol('atom', function (request, callback) {
  callback({mimeType: 'text/html', data: new Buffer('<h5>Response</h5>')})
}, function (error) {
  if (error)
    console.error('Failed to register protocol')
})
```

### `protocol.registerStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

注册一个 `scheme` 协议，用来发送响应 `String` .

这个方法的用法类似 `registerFileProtocol`，除非使用一个 `String` 对象，或一个有 `data`,
`mimeType`, 和 `charset` 属性的对象来调用 `callback` .

### `protocol.registerHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

注册一个 `scheme` 协议，用来发送 HTTP 请求作为响应.

这个方法的用法类似 `registerFileProtocol`，除非使用一个 `redirectRequest` 对象，或一个有 `url`, `method`,
`referrer`, `uploadData` 和 `session` 属性的对象来调用 `callback` .

* `redirectRequest` Object
  * `url` String
  * `method` String
  * `session` Object (可选)
  * `uploadData` Object (可选)

默认这个 HTTP 请求会使用当前 session .如果你想使用不同的session值，你应该设置 `session` 为 `null`.

POST 请求应当包含 `uploadData` 对象.

* `uploadData` object
  * `contentType` String - 内容的 MIME type.
  * `data` String - 被发送的内容.

### `protocol.unregisterProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (可选)

注销自定义协议 `scheme`.

### `protocol.isProtocolHandled(scheme, callback)`

* `scheme` String
* `callback` Function

将使用一个布尔值来调用 `callback` ，这个布尔值标识了是否已经存在 `scheme` 的句柄了.

### `protocol.interceptFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送响应文件.

### `protocol.interceptStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送响应 `String`.

### `protocol.interceptBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (可选)

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送响应 `Buffer`.

### `protocol.interceptHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
* `completion` Function (optional)

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送新的响应 HTTP 请求.
Intercepts `scheme` protocol and uses `handler` as the protocol's new handler
which sends a new HTTP request as a response.

### `protocol.uninterceptProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function
取消对 `scheme` 的拦截，使用它的原始句柄进行处理.

[net-error]: https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h