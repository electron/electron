# SharedDictionaryInfo Object

* `match` string - The matching path pattern for the dictionary which was declared in 'use-as-dictionary' response header's `match` option.
* `matchDestinations` string[] - An array of matching destinations for the dictionary which was declared in 'use-as-dictionary' response header's `match-dest` option.
* `id` string - The Id for the dictionary which was declared in 'use-as-dictionary' response header's `id` option.
* `dictionaryUrl` string - URL of the dictionary.
* `lastFetchTime` Date - The time of when the dictionary was received from the network layer.
* `responseTime` Date - The time of when the dictionary was received from the server. For cached responses, this time could be "far" in the past.
* `expirationDuration` number - The expiration time for the dictionary which was declared in 'use-as-dictionary' response header's `expires` option in seconds.
* `lastUsedTime` Date - The time when the dictionary was last used.
* `size` number - The amount of bytes stored for this shared dictionary information object in Chromium's internal storage (usually Sqlite).
* `hash` string - The sha256 hash of the dictionary binary.
