# Extension Object

* `id` string
* `dynamicId` string - A per-load identifier Chromium generates for the extension, used in place of `id` in the `chrome-extension://` URLs of web accessible resources declared with `use_dynamic_url`. It changes every time the extension is loaded.
* `manifest` any - Copy of the [extension's manifest data](https://developer.chrome.com/extensions/manifest).
* `name` string
* `path` string - The extension's file path.
* `version` string
* `url` string - The extension's `chrome-extension://` URL.
