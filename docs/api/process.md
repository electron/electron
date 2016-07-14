# process

> Extensions to process object.

The `process` object is extended in Electron with following APIs:

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

### `process.type`

Current process's type, can be `"browser"` (i.e. main process) or `"renderer"`.

### `process.versions.electron`

Electron's version string.

### `process.versions.chrome`

Chrome's version string.

### `process.resourcesPath`

Path to the resources directory.

### `process.mas`

For Mac App Store build, this property is `true`, for other builds it is
`undefined`.

### `process.windowsStore`

If the app is running as a Windows Store app (appx), this property is `true`,
for otherwise it is `undefined`.

### `process.defaultApp`

When app is started by being passed as parameter to the default app, this
property is `true` in the main process, otherwise it is `undefined`.

## Methods

The `process` object has the following method:

### `process.crash()`

Causes the main thread of the current process crash.

### `process.hang()`

Causes the main thread of the current process hang.

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

Sets the file descriptor soft limit to `maxDescriptors` or the OS hard
limit, whichever is lower for the current process.

### `process.getProcessMemoryInfo()`

Returns an object giving memory usage statistics about the current process. Note
that all statistics are reported in Kilobytes.

* `workingSetSize` Integer - The amount of memory currently pinned to actual physical
  RAM.
* `peakWorkingSetSize` Integer - The maximum amount of memory that has ever been pinned
  to actual physical RAM.
* `privateBytes` Integer - The amount of memory not shared by other processes, such as
  JS heap or HTML content.
* `sharedBytes` Integer - The amount of memory shared between processes, typically
  memory consumed by the Electron code itself

### `process.getSystemMemoryInfo()`

Returns an object giving memory usage statistics about the entire system. Note
that all statistics are reported in Kilobytes.

* `total` Integer - The total amount of physical memory in Kilobytes available to the
  system.
* `free` Integer - The total amount of memory not being used by applications or disk
  cache.
* `swapTotal` Integer - The total amount of swap memory in Kilobytes available to the
  system.  _Windows_ _Linux_
* `swapFree` Integer - The free amount of swap memory in Kilobytes available to the
  system.  _Windows_ _Linux_
