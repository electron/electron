# process

The `process` object in Electron has the following differences from the one in
upstream node:

* `process.type` String - Process's type, can be `browser` (i.e. main process)
  or `renderer`.
* `process.versions['electron']` String - Version of Electron.
* `process.versions['chrome']` String - Version of Chromium.
* `process.resourcesPath` String - Path to JavaScript source code.

# Methods

The `process` object has the following method:

### `process.hang`

Causes the main thread of the current process hang.

## process.setFdLimit(maxDescriptors) _OS X_ _Linux_

* `maxDescriptors` Integer

Sets the file descriptor soft limit to `maxDescriptors` or the OS hard
limit, whichever is lower for the current process.
