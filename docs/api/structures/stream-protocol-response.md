# StreamProtocolResponse Object

* `statusCode` Number (optional) - The HTTP response code.
* `headers` Record<String, String | String[]> (optional) - An object containing the response headers.
* `data` ReadableStream | null - A Node.js readable stream representing the response body.
