## Class: Cookies

> 查询并修改会话的 cookies.

Process: [Main](../glossary.md#main-process)

通过 `Session` 的属性 `cookies` 来访问 `Cookies` 类的实例.

例如:

```javascript
const {session} = require('electron')

// Query all cookies.
session.defaultSession.cookies.get({}, (error, cookies) => {
  console.log(error, cookies)
})

// Query all cookies associated with a specific url.
session.defaultSession.cookies.get({url: 'http://www.github.com'}, (error, cookies) => {
  console.log(error, cookies)
})

// Set a cookie with the given cookie data;
// may overwrite equivalent cookies if they exist.
const cookie = {url: 'http://www.github.com', name: 'dummy_name', value: 'dummy'}
session.defaultSession.cookies.set(cookie, (error) => {
  if (error) console.error(error)
})
```

### 实例事件
以下事件可在 `Cookies` 实例上获得

#### Event: 'changed'

* `event` Event
* `cookie` [Cookie](structures/cookie.md) - 被改变的 cookie
* `cause` String - 下面的一个值是改变的原因:
  * `explicit` - cookie 由用户的行为直接改变.
  * `overwrite` - cookie 由于插入操作被覆盖.
  * `expired` - cookie 由于过期被自动删除.
  * `evicted` - cookie 在垃圾回收的过程中被自动删除.
  * `expired-overwrite` - cookie已经覆盖过期日期.
* `removed` Boolean - cookie 被移除为 `true`,否则为`false` .

当 cookie 因为增加、编辑、移除或过期等原因被改变的时候发送.

### 实例方法

以下方法可在 `Cookies` 实例上获得:

#### `cookies.get(filter, callback)`

* `filter` Object
  * `url` String (optional) - 检索与` url` 关联的 cookies.为空时检索所有 url 的 cookie.
  * `name` String (optional) - 按照名称过滤 cookie.
  * `domain` String (optional) - 检索域名或子域名匹配的cookie
  * `path` String (optional) - 检索与路径匹配的cookie.
  * `secure` Boolean (optional) - 按照安全属性过滤 cookie.
  * `session` Boolean (optional) - 按照会话或持久性过滤 cookie.
* `callback` Function
  * `error` Error
  * `cookies` Cookies[]

发送请求来获取所有 cookie 匹配的`详细信息`,`callback(error, cookies)` 完成时会调用 `callback` 回调方法.

`cookies` 是一个 [`cookie`](structures/cookie.md) 对象的数组.

#### `cookies.set(details, callback)`

* `details` Object
  * `url` String - 关联 cookie 的URL.
  * `name` String (optional) - cookie 的名称。如果省略，默认为空.
  * `value` String (optional) - cookie 的值。如果省略，默认为空.
  * `domain` String (optional) - cookie 的域。如果省略，默认为空.
  * `path` String (optional) - cookie 的路径。如果省略，默认为空.
  * `secure` Boolean (optional) - Cookie 是否应标记为安全.默认为false.
  * `httpOnly` Boolean (optional) - cookie 是否应仅被标记为 HTTP.默认为false.
  * `expirationDate` Double (optional) - cookie 距离 UNIX 时间戳的过期时间，数值为秒。如果省略，则 cookie 成为会话cookie，并且不会在会话之间保留.
* `callback` Function
  * `error` Error

设置 cookie 的`详细信息`,`callback(error)` 完成时会调用 `callback` 回调方法.

#### `cookies.remove(url, name, callback)`

* `url` String - 与 cookie 关联的 URL.
* `name` String - 要删除的 cookie 的名称.
* `callback` Function

移除符合 `url` 和 `name` 的 cookie,`callback()` 完成时会调用 `callback` 回调方法.
