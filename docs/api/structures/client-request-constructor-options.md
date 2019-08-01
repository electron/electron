# ClientRequestConstructorOptions Object

* `method` String (optional) - The HTTP request method. Defaults to the GET
method.
* `url` String (optional) - The request URL. Must be provided in the absolute
form with the protocol scheme specified as http or https.
* `session` Object (optional) - The [`Session`](session.md) instance with
which the request is associated.
* `partition` String (optional) - The name of the [`partition`](session.md)
  with which the request is associated. Defaults to the empty string. The
  `session` option prevails on `partition`. Thus if a `session` is explicitly
  specified, `partition` is ignored.
* `host` String (optional) - The server host provided as a concatenation of
  the hostname and the port number 'hostname:port'.
* `hostname` String (optional) - The server host name.
* `port` Integer (optional) - The server's listening port number.
* `path` String (optional) - The path part of the request URL.
* `protocol` String (optional) - The protocol scheme in the form `scheme:`.
  * `http:`
  * `https:` - The default value.
* `redirect` String (optional) - The redirect mode for this request.
  * `follow` - The default value.
  * `error` - Redirection will be aborted.
  * `manual` - Defer redirection until [`request.followRedirect`](#requestfollowredirect)
    is invoked. Listen for the [`redirect`](#event-redirect) event in
    this mode to get more details about the redirect request.

Properties such as `protocol`, `host`, `hostname`, `port` and `path` strictly
follow the Node.js model as described in the [URL](https://nodejs.org/api/url.html)
module.

For instance, we could have created the same request to 'github.com' as follows:

```JavaScript
const request = net.request({
  method: 'GET',
  protocol: 'https:',
  hostname: 'github.com',
  port: 443,
  path: '/'
})
```
