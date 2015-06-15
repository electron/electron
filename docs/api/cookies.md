# cookies

The `cookies` module gives you ability to query and modify cookies, an example
is:

```javascipt
var cookies = require('cookies');

// Query all cookies.
cookies.get({}, function(error, cookies) {
  if (error) throw error;
  console.log(cookies);
});

// Query all cookies that are associated with a specific url.
cookies.get({ url : "http://www.github.com" }, function(error, cookies) {
  if (error) throw error;
  console.log(cookies);
});

// Set a cookie with the given cookie data; may overwrite equivalent cookies if they exist.
cookies.set({ url : "http://www.github.com", name : "dummy_name", value : "dummy"},
    function(error, cookies) {
        if (error) throw error;
        console.log(cookies);
});
```

## cookies.get(details, callback)

* `details` Object
  * `url` String - Retrieves cookies which are associated with `url`. Empty imples retrieving cookies of all urls.
  * `name` String - Filters cookies by name
  * `domain` String - Retrieves cookies whose domains match or are subdomains of `domains`
  * `path` String - Retrieves cookies whose path matches `path`
  * `secure` Boolean - Filters cookies by their Secure property
  * `session` Boolean - Filters out session or persistent cookies.
* `callback` Function - function(error, cookies)
  * `error` Error
  * `cookies` Array - array of cookie objects.

## cookies.set(details, callback)

* `details` Object
  * `url` String - Retrieves cookies which are associated with `url`
  * `name` String - The name of the cookie. Empty by default if omitted.
  * `value` String - The value of the cookie. Empty by default if omitted.
  * `domain` String - The domain of the cookie. Empty by default if omitted.
  * `path` String - The path of the cookie. Empty by default if omitted.
  * `secure` Boolean - Whether the cookie should be marked as Secure. Defaults to false.
  * `session` Boolean - Whether the cookie should be marked as HttpOnly. Defaults to false.
  * `expirationDate` Double -	The expiration date of the cookie as the number of seconds since the UNIX epoch. If omitted, the cookie becomes a session cookie.

* `callback` Function - function(error)
  * `error` Error

## cookies.remove(details, callback)

* `details` Object
  * `url` String - The URL associated with the cookie
  * `name` String - The name of cookie to remove
* `callback` Function - function(error)
  * `error` Error
