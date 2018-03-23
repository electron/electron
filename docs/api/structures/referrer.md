# Referrer Object

* `url` String - HTTP Referrer URL.
* `policy` String - Referrer Policy, as specified in the [Referrer-Policy
  header][1].

`policy` can be one of `default`, `unsafe-url`, `no-referrer-when-downgrade`,
`no-referrer`, `origin`, `strict-origin-when-cross-origin`, `same-origin`,
`strict-origin`, or `no-referrer`. See the [Referrer-Policy spec][1] for more
details on the meaning of these values.

[1]: https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Referrer-Policy
