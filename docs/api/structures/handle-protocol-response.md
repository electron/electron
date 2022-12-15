# HandleProtocolResponse Object

* `error` Integer (optional) - When assigned, the request will fail with the
  error defined by `error`. For the available error numbers you can use, see
  the [net error list][net-error].
* `statusCode` number (optional) - The HTTP response code. If not specified,
  the response will be sent with status code 200.
* `headers` Record<string, string | string[]> (optional) - The response headers.
* `body` string | Buffer | ReadableStream | null (optional) - the body of the
  response. If null or undefined, the body will be empty.

[net-error]: https://source.chromium.org/chromium/chromium/src/+/main:net/base/net_error_list.h
