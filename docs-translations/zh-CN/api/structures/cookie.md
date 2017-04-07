# Cookie Object

* `name` String - cookie 的名称.
* `value` String - cookie 的值.
* `domain` String (optional) - cookie 的域名.
* `hostOnly` Boolean (optional) - cookie 的类型是否为 host-only.
* `path` String (optional) - cookie 的路径.
* `secure` Boolean (optional) - cookie 是否标记为安全.
* `httpOnly` Boolean (optional) - cookie 是否只标记为 HTTP.
* `session` Boolean (optional) - cookie 是否是一个 session cookie, 还是一个带有过期时间的持续 cookie.
* `expirationDate` Double (optional) -  cookie 距离 UNIX 时间戳的过期时间，数值为秒。不需要提供给 session
  cookies.
