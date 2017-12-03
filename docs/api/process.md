# process

> Extensions to process object.

Process: [Main](../glossary.md#main-process), [Renderer](../glossary.md#renderer-process)

Electron's `process` object is extended from the
[Node.js `process` object](https://nodejs.org/api/process.html).
It adds the following events, properties, and methods:

## Events

### Event: 'loaded'

Emitted when Electron has loaded its internal initialization script and is
beginning to load the web page or the main script.

It can be used by the preload script to add removed Node global symbols back to
the global scope when node integration is turned off:

```javascript
// preload.js
const _setImmediate = setImmediate
const _clearImmediate = clearImmediate
process.once('loaded', () => {
  global.setImmediate = _setImmediate
  global.clearImmediate = _clearImmediate
})
```

## Properties

### `process.defaultApp`

A `Boolean`. When app is started by being passed as parameter to the default app, this
property is `true` in the main process, otherwise it is `undefined`.

### `process.mas`

A `Boolean`. For Mac App Store build, this property is `true`, for other builds it is
`undefined`.

### `process.noAsar`

A `Boolean` that controls ASAR support inside your application. Setting this to `true`
will disable the support for `asar` archives in Node's built-in modules.

### `process.noDeprecation`

A `Boolean` that controls whether or not deprecation warnings are printed to `stderr`.
Setting this to `true` will silence deprecation warnings. This property is used
instead of the `--no-deprecation` command line flag.

### `process.resourcesPath`

A `String` representing the path to the resources directory.

### `process.throwDeprecation`

A `Boolean` that controls whether or not deprecation warnings will be thrown as
exceptions. Setting this to `true` will throw errors for deprecations. This
property is used instead of the `--throw-deprecation` command line flag.

### `process.traceDeprecation`

A `Boolean` that controls whether or not deprecations printed to `stderr` include
 their stack trace. Setting this to `true` will print stack traces for deprecations.
 This property is instead of the `--trace-deprecation` command line flag.

### `process.traceProcessWarnings`
A `Boolean` that controls whether or not process warnings printed to `stderr` include
 their stack trace. Setting this to `true` will print stack traces for process warnings
 (including deprecations). This property is instead of the `--trace-warnings` command
 line flag.

### `process.type`

A `String` representing the current process's type, can be `"browser"` (i.e. main process) or `"renderer"`.

### `process.versions.chrome`

A `String` representing Chrome's version string.

### `process.versions.electron`

A `String` representing Electron's version string.

### `process.windowsStore`

A `Boolean`. If the app is running as a Windows Store app (appx), this property is `true`,
for otherwise it is `undefined`.

## Methods

The `process` object has the following methods:

### `process.crash()`

Causes the main thread of the current process crash.

### `process.getCPUUsage()`

Returns [`CPUUsage`](structures/cpu-usage.md)

### `process.getIOCounters()` _Windows_ _Linux_

Returns [`IOCounters`](structures/io-counters.md)

### `process.getProcessMemoryInfo()`

Returns `Object`:

* `workingSetSize` Integer - The amount of memory currently pinned to actual physical
  RAM.
* `peakWorkingSetSize` Integer - The maximum amount of memory that has ever been pinned
  to actual physical RAM.
* `privateBytes` Integer - The amount of memory not shared by other processes, such as
  JS heap or HTML content.
* `sharedBytes` Integer - The amount of memory shared between processes, typically
  memory consumed by the Electron code itself.

Returns an object giving memory usage statistics about the current process. Note
that all statistics are reported in Kilobytes.

### `process.getSystemMemoryInfo()`

Returns `Object`:

* `total` Integer - The total amount of physical memory in Kilobytes available to the
  system.
* `free` Integer - The total amount of memory not being used by applications or disk
  cache.
* `swapTotal` Integer _Windows_ _Linux_ - The total amount of swap memory in Kilobytes available to the
  system.
* `swapFree` Integer _Windows_ _Linux_ - The free amount of swap memory in Kilobytes available to the
  system.

Returns an object giving memory usage statistics about the entire system. Note
that all statistics are reported in Kilobytes.

### `process.hang()`

Causes the main thread of the current process hang.

### `process.setFdLimit(maxDescriptors)` _macOS_ _Linux_

* `maxDescriptors` Integer

Sets the file descriptor soft limit to `maxDescriptors` or the OS hard
limit, whichever is lower for the current process.
