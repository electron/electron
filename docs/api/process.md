# process

> Get information about the running application process.

The `process` object in Electron has the following differences from the one in
upstream node:

* `process.type` String - Process's type, can be `browser` (i.e. main process)
  or `renderer`.
* `process.versions.electron` String - Version of Electron.
* `process.versions.chrome` String - Version of Chromium.
* `process.resourcesPath` String - Path to JavaScript source code.
* `process.mas` Boolean - For Mac App Store build, this value is `true`, for
  other builds it is `undefined`.
* `process.windowsStore` Boolean - If the app is running as a Windows Store app
  (appx), this value is `true`, for other builds it is `undefined`.
* `process.defaultApp` Boolean - When app is started by being passed as
  parameter to the default app, this value is `true` in the main process,
  otherwise it is `undefined`.
  
## Events

### Event: 'loaded'

Emitted when Electron has loaded its internal initialization script and is
beginning to load the web page or the main script.

It can be used by the preload script to add removed Node global symbols back to
the global scope when node integration is turned off:

```javascript
// preload.js
const _setImmediate = setImmediate;
const _clearImmediate = clearImmediate;
process.once('loaded', () => {
  global.setImmediate = _setImmediate;
  global.clearImmediate = _clearImmediate;
});
```

## Properties

### `process.noAsar`

Setting this to `true` can disable the support for `asar` archives in Node's
built-in modules.

## Methods

The `process` object has the following method:

### `process.crash()`

Causes the main thread of the current process crash.

### `process.hang()`

Causes the main thread of the current process hang.

### `process.setFdLimit(maxDescriptors)` _OS X_ _Linux_

* `maxDescriptors` Integer

Sets the file descriptor soft limit to `maxDescriptors` or the OS hard
limit, whichever is lower for the current process.

### getProcessMemoryInfo()

Return an object giving memory usage statistics about the current process. Note
that all statistics are reported in Kilobytes.

* `workingSetSize` - The amount of memory currently pinned to actual physical
  RAM
* `peakWorkingSetSize` - The maximum amount of memory that has ever been pinned
  to actual physical RAM
* `privateBytes` - The amount of memory not shared by other processes, such as
  JS heap or HTML content.
* `sharedBytes` - The amount of memory shared between processes, typically
  memory consumed by the Electron code itself

### getSystemMemoryInfo()

Return an object giving memory usage statistics about the entire system. Note
that all statistics are reported in Kilobytes.

* `total` - The total amount of physical memory in Kilobytes available to the
  system
* `free` - The total amount of memory not being used by applications or disk
  cache

On Windows / Linux:

* `swapTotal` - The total amount of swap memory in Kilobytes available to the
  system
* `swapFree` - The free amount of swap memory in Kilobytes available to the
  system
