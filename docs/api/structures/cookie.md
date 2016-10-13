# Certificate Object

*  `name` String - The name of the cookie.
*  `value` String - The value of the cookie.
*  `domain` String - The domain of the cookie.
*  `hostOnly` String - Whether the cookie is a host-only cookie.
*  `path` String - The path of the cookie.
*  `secure` Boolean - Whether the cookie is marked as secure.
*  `httpOnly` Boolean - Whether the cookie is marked as HTTP only.
*  `session` Boolean - Whether the cookie is a session cookie or a persistent
   cookie with an expiration date.
*  `expirationDate` Double (optional) - The expiration date of the cookie as
   the number of seconds since the UNIX epoch. Not provided for session
   cookies.
