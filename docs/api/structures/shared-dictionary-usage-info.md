# SharedDictionaryUsageInfo Object

* `frameOrigin` string - The origin of the frame where the request originates. It’s specific to the individual frame making the request and is defined by its scheme, host, and port. In practice, will look like a URL.
* `topFrameSite` string - The site of the top-level browsing context (the main frame or tab that contains the request). It’s less granular than `frameOrigin` and focuses on the broader "site" scope. In practice, will look like a URL.
* `totalSizeBytes` number - The amount of bytes stored for this shared dictionary information object in Chromium's internal storage (usually Sqlite).
