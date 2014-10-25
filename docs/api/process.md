# Process object

The `process` object in atom-shell has following differences between the one in
upstream node:

* `process.type` String - Process's type, can be `browser` or `renderer`.
* `process.versions['atom-shell']` String - Version of atom-shell.
* `process.versions['chrome']` String - Version of Chromium.
* `process.resourcesPath` String - Path to JavaScript source code.
