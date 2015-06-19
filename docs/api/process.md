# Process object

The `process` object in Electron has the following differences from the one in
upstream node:

* `process.type` String - Process's type, can be `browser` (i.e. main process) or `renderer`.
* `process.versions['electron']` String - Version of Electron.
* `process.versions['chrome']` String - Version of Chromium.
* `process.resourcesPath` String - Path to JavaScript source code.

## process.hang

Causes the main thread of the current process hang.
