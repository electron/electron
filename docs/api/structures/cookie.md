# Cookie Object

* `name` String - The name of the cookie.
* `value` String - The value of the cookie.
* `domain` String - (optional) The domain of the cookie.
* `hostOnly` Boolean - (optional) Whether the cookie is a host-only cookie.
* `path` String - (optional) The path of the cookie.
* `secure` Boolean - (optional) Whether the cookie is marked as secure.
* `httpOnly` Boolean - (optional) Whether the cookie is marked as HTTP only.
* `session` Boolean - (optional) Whether the cookie is a session cookie or a persistent
  cookie with an expiration date.
* `expirationDate` Double - (optional) The expiration date of the cookie as
  the number of seconds since the UNIX epoch. Not provided for session
  cookies.
