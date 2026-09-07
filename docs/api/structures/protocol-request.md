# ProtocolRequest Object

* `url` string
* `referrer` string
* `initiatorOrigin` string (optional) - The origin that issued the request (for example
  `https://example.com`, or `null` for an opaque origin). Absent for requests
  the browser started itself. Unlike `referrer`, this is not controlled by the
  requesting page.
* `method` string
* `uploadData` [UploadData[]](upload-data.md) (optional)
* `headers` Record\<string, string\>
