# 协议

> 注册一个自定义协议，或者使用一个已经存在的协议。

进程： [Main](../glossary.md#main-process)

例如使用一个与 `file://` 功能相似的协议：

```javascript
const {app, protocol} = require('electron')
const path = require('path')

app.on('ready', () => {
  protocol.registerFileProtocol('atom', (request, callback) => {
    const url = request.url.substr(7)
    callback({path: path.normalize(`${__dirname}/${url}`)})
  }, (error) => {
    if (error) console.error('Failed to register protocol')
  })
})
```

**注意：** 这个模块只有在 `app` 模块的 `ready` 事件触发之后才可使用。

## 方法

`protocol` 模块有如下方法：

### `protocol.registerStandardSchemes(schemes[, options])`

* `schemes` String[] - 将一个自定义的方案注册为标准的方案。
* `options` Object (可选)
  * `secure` Boolean (可选) - `true` 将方案注册为安全。
    默认值 `false`。

一个标准的 `scheme` 遵循 RFC 3986 的
[generic URI syntax](https://tools.ietf.org/html/rfc3986#section-3) 标准。例如 `http` 和
`https` 是标准方案，而 `file` 不是。

注册一个 `scheme` 作为标准，将允许相对和绝对的资源
在服务时正确解析。 否则该方案将表现得像
`file` 协议，但无法解析相对 URLs。

例如，当你加载以下页面与自定义协议无法
注册为标准 `scheme`，图像将不会加载，因为
非标准方案无法识别相对 URLs：

```html
<body>
  <img src='test.png'>
</body>
```

注册方案作为标准将允许通过访问文件
[FileSystem API][file-system-api]。 否则渲染器将抛出一个安全性
错误。

默认情况下 web storage apis（localStorage，sessionStorage，webSQL，indexedDB，cookies）
对于非标准方案禁用。所以一般来说如果你想注册一个
自定义协议替换 `http` 协议，您必须将其注册为标准方案：

```javascript
const {app, protocol} = require('electron')

protocol.registerStandardSchemes(['atom'])
app.on('ready', () => {
  protocol.registerHttpProtocol('atom', '...')
})
```

**注意：** 这个方法只有在 `app` 模块的 `ready` 事件触发之后才可使用。

### `protocol.registerServiceWorkerSchemes(schemes)`

* `schemes` String[] - 将一个自定义的方案注册为处理 service workers。

### `protocol.registerFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `filePath` String (可选)
* `completion` Function (可选)
  * `error` Error

注册一个协议，用来发送响应文件。当通过这个协议来发起一个请求的时候，将使用 `handler(request，callback)` 来调用
`handler`。当 `scheme` 被成功注册或者完成(错误)时失败，将使用 `completion(null)` 调用 `completion`。

为了处理请求，调用 `callback` 时需要使用文件路径或者一个带 `path` 参数的对象, 例如 `callback(filePath)` 或
`callback({path: filePath})`。

当不使用任何参数调用 `callback` 时，你可以指定一个数字或一个带有 `error` 参数的对象，来标识 `request` 失败。你可以使用的 error number 可以参考
[net error list][net-error]。

默认 `scheme` 会被注册为一个 `http:` 协议，它与遵循 "generic URI syntax" 规则的协议解析不同，例如 `file:`，所以你或许应该调用 `protocol.registerStandardSchemes` 来创建一个标准的 scheme。

### `protocol.registerBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `buffer` (Buffer | [MimeTypedBuffer](structures/mime-typed-buffer.md)) (optional)
* `completion` Function (可选)
  * `error` Error

注册一个 `scheme` 协议，用来发送响应 `Buffer`。

这个方法的用法类似 `registerFileProtocol`，除非使用一个 `Buffer` 对象，或一个有 `data`、
`mimeType` 和 `charset` 属性的对象来调用 `callback`。

例子:

```javascript
const {protocol} = require('electron')

protocol.registerBufferProtocol('atom', (request, callback) => {
  callback({mimeType: 'text/html', data: new Buffer('<h5>Response</h5>')})
}, (error) => {
  if (error) console.error('Failed to register protocol')
})
```

### `protocol.registerStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `data` String (可选)
* `completion` Function (可选)
  * `error` Error

注册一个 `scheme` 协议，用来发送响应 `String`。

这个方法的用法类似 `registerFileProtocol`，除非使用一个 `String` 对象，或一个有 `data`、
`mimeType` 和 `charset` 属性的对象来调用 `callback`。

### `protocol.registerHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `redirectRequest` Object
      * `url` String
      * `method` String
      * `session` Object (可选)
      * `uploadData` Object (可选)
        * `contentType` String - MIME type of the content.
        * `data` String - Content to be sent.
* `completion` Function (可选)
  * `error` Error

注册一个 `scheme` 协议，用来发送 HTTP 请求作为响应.

这个方法的用法类似 `registerFileProtocol`，除非使用一个 `redirectRequest` 对象，或一个有 `url`、 `method`、
`referrer`、 `uploadData` 和 `session` 属性的对象来调用 `callback`。

默认这个 HTTP 请求会使用当前 session。如果你想使用不同的session值，你应该设置 `session` 为 `null`。

POST 请求应当包含 `uploadData` 对象。

### `protocol.unregisterProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (可选)
  * `error` Error

注销自定义协议 `scheme`。

### `protocol.isProtocolHandled(scheme, callback)`

* `scheme` String
* `callback` Function
  * `error` Error

将使用一个布尔值来调用 `callback` ，这个布尔值标识了是否已经存在 `scheme` 的句柄了。

### `protocol.interceptFileProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `filePath` String
* `completion` Function (可选)
  * `error` Error

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送响应文件。

### `protocol.interceptStringProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `data` String (可选)
* `completion` Function (可选)
  * `error` Error

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送响应 `String`。

### `protocol.interceptBufferProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `buffer` Buffer (可选)
* `completion` Function (可选)
  * `error` Error

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送响应 `Buffer`。

### `protocol.interceptHttpProtocol(scheme, handler[, completion])`

* `scheme` String
* `handler` Function
  * `request` Object
    * `url` String
    * `referrer` String
    * `method` String
    * `uploadData` [UploadData[]](structures/upload-data.md)
  * `callback` Function
    * `redirectRequest` Object
      * `url` String
      * `method` String
      * `session` Object (可选)
      * `uploadData` Object (可选)
        * `contentType` String - MIME type of the content.
        * `data` String - Content to be sent.
* `completion` Function (可选)
  * `error` Error

拦截 `scheme` 协议并且使用 `handler` 作为协议的新的句柄来发送新的响应 HTTP 请求。

### `protocol.uninterceptProtocol(scheme[, completion])`

* `scheme` String
* `completion` Function (可选)
  * `error` Error

取消对 `scheme` 的拦截，使用它的原始句柄进行处理。

[net-error]: https://code.google.com/p/chromium/codesearch#chromium/src/net/base/net_error_list.h
[file-system-api]: https://developer.mozilla.org/en-US/docs/Web/API/LocalFileSystem
