## Class: Cookies

> Query and modify a session's cookies.

Process: [Main](../tutorial/quick-start.md#main-process)

Instances of the `Cookies` class are accessed by using `cookies` property of
a `Session`.

For example:

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

### Instance Events

The following events are available on instances of `Cookies`:

#### Event: 'changed'

* `event` Event
* `cookie` [Cookie](structures/cookie.md) - The cookie that was changed
* `cause` String - The cause of the change with one of the following values:
  * `explicit` - The cookie was changed directly by a consumer's action.
  * `overwrite` - The cookie was automatically removed due to an insert
    operation that overwrote it.
  * `expired` - The cookie was automatically removed as it expired.
  * `evicted` - The cookie was automatically evicted during garbage collection.
  * `expired-overwrite` - The cookie was overwritten with an already-expired
    expiration date.
* `removed` Boolean - `true` if the cookie was removed, `false` otherwise.

Emitted when a cookie is changed because it was added, edited, removed, or
expired.

### Instance Methods

The following methods are available on instances of `Cookies`:

#### `cookies.get(filter, callback)`

* `filter` Object
  * `url` String (optional) - Retrieves cookies which are associated with
    `url`. Empty implies retrieving cookies of all urls.
  * `name` String (optional) - Filters cookies by name.
  * `domain` String (optional) - Retrieves cookies whose domains match or are
    subdomains of `domains`
  * `path` String (optional) - Retrieves cookies whose path matches `path`.
  * `secure` Boolean (optional) - Filters cookies by their Secure property.
  * `session` Boolean (optional) - Filters out session or persistent cookies.
* `callback` Function
  * `error` Error
  * `cookies` Cookies[]

Sends a request to get all cookies matching `details`, `callback` will be called
with `callback(error, cookies)` on complete.

`cookies` is an Array of [`cookie`](structures/cookie.md) objects.

#### `cookies.set(details, callback)`

* `details` Object
  * `url` String - The url to associate the cookie with.
  * `name` String - (optional) The name of the cookie. Empty by default if omitted.
  * `value` String - (optional) The value of the cookie. Empty by default if omitted.
  * `domain` String - (optional) The domain of the cookie. Empty by default if omitted.
  * `path` String - (optional) The path of the cookie. Empty by default if omitted.
  * `secure` Boolean - (optional) Whether the cookie should be marked as Secure. Defaults to
    false.
  * `httpOnly` Boolean - (optional) Whether the cookie should be marked as HTTP only.
    Defaults to false.
  * `expirationDate` Double -	(optional) The expiration date of the cookie as the number of
    seconds since the UNIX epoch. If omitted then the cookie becomes a session
    cookie and will not be retained between sessions.
* `callback` Function
  * `error` Error

Sets a cookie with `details`, `callback` will be called with `callback(error)`
on complete.

#### `cookies.remove(url, name, callback)`

* `url` String - The URL associated with the cookie.
* `name` String - The name of cookie to remove.
* `callback` Function

Removes the cookies matching `url` and `name`, `callback` will called with
`callback()` on complete.
