# Test Spec Library

This directory contains helpful, reusable modules for making our tests more consistent, ergonomic, clear, and efficient.

## Library Tour

* `async-iter`: [Async iterators (`AsyncIterator`)][AsyncIterator]
  * `firstN`: Limit how many elements an iterator emits.
  * `find`: Find a matching element in an iterator.
  * `arrayFromAsync`: Implementation of the pending [`Array.fromAsync`][Array.fromAsync] proposal.
  * `iterable`: Turn any `AsyncIterator` into an `AsyncIterableIterator`.
* `async-loop`: Async looping
  * `pollUntil`: Run an async function repeatedly until it returns a predicate-satisfying value (or just any truthy value, by default).
* `color`: Colors
  * `HexColors`: Common, named hex color strings.
  * `Color`: RGB color interface.
  * `colorFromHex`: Parse 6-digit hex color strings (e.g. `#1a2b3c`).
  * `areColorsSimilar`: Roughly check if two colors are similar enough to be considered the same color.
  * `expectColorsAreSimilar`, `expectColorsAreDissimilar`: Convenience functions that assert that two colors are similar or dissimilar, respectively. These functions display a helpful assertion error message when they fail.
* `events`: Events (`EventEmitter`, `node:events`)
  * `scopedOn`: Similar to the [`on` helper from `node:events`][node:events:on] but automatically cancelled after the provided closer resolves. Used to clean up event listeners proactively.
  * `emittedN`: Collect the first n emitted arguments from an event emitter.
  * `findEmit`: Resolves when an event emitter emits an event that satisfies a predicate function.
* `fixtures`: Fixtures
  * `fixturePath`: Standard way to build a path to something within the [`fixtures/`][fixtures-dir] directory.
  * `fixtureFileURL`: Standard way to build a `file://` URL to something within the [`fixtures/`][fixtures-dir] directory.
* `jsont`: JSON string interpolation & JS source code construction
  * `jsont`: A tagged-template function for safely interpolating JSON-compatible values into strings, particularly JS code snippets.
* `native-img`: Electron `NativeImage`
  * `Bitmap`: A class that provides ergonomic access to pixel colors from a `NativeImage`.
* `screen-capture`: Display and screen capture
  * `captureScreenImg`: Captures a `NativeImage` of a particular display.
  * `captureScreenBitmap`: Convenience wrapper fusing `captureScreenImg` with the `Bitmap` class in `native-img`.
* `spec-conditional`: Specifying preconditions for specs
  * `ifit`, `ifdescribe`: Provide a precondition for a spec (`it` or `describe`), otherwise skip it.

### Older Utilities

These utilities are older than the ones above, but are widely used in our test suite. They still prove useful in certain test suites. You may have to consult their source code to understand how to best use them.

* `spec-helpers`: Catch-all module for various helpers that don't fit elsewhere
  * `defer`: Adds a function to clean up some resource after tests finish running.
    * `runCleanupFunctions`: Eagerly run all deferred cleanup functions.
  * Remote control & remote context: Historically, some of our tests were written to run in the renderer process. These utilities are a sort of compatibility layer for those tests, allowing them to work without major rewrites. Consider writing new tests in a way that doesn't require these utilities.
    * `startRemoteControlApp`
    * `makeRemoteContext`
    * `getRemoteContext`: Reuse or create a remote context for the current test spec closure.
    * `useRemoteContext`: Creates a new remote context for the current test spec closure. This new context is cleaned up and removed at the end of the test scope.
    * `itremote`: Similar to the `it` test descriptor, but the provided closure is stringified and run in the renderer process.
  * `listen`: Accepts a server (HTTP or HTTPS) and returns a promise that resolves when the server has started listening for incoming connections. The resolved value includes information about the chosen port and a URL to the server.
* `video-helpers`: A copy of the [whammy](https://github.com/antimatter15/whammy) library. Used to generate WebM videos from a sequence of images.
* `window-helpers`: Various helpers for working with `BrowserWindow`
  * `closeWindow`: Close a window and wait for it to be destroyed.
  * `closeAllWindows`: Close all windows and wait for them to be destroyed.
* `get-files`
  * `getFiles`: Walks the paths within a directory and collects them into a list, optionally filtering by a predicate function.
* `pipe-transport`
  * `PipeTransport`: A small pipe transport for talking to Electron over CDP.

[AsyncIterator]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Iteration_protocols#the_async_iterator_and_async_iterable_protocols
[Array.fromAsync]: https://github.com/tc39/proposal-array-from-async
[node:events:on]: https://nodejs.org/api/events.html#eventsonemitter-eventname-options
[fixtures-dir]: ../fixtures/

## Recipes 🍱 🥗 🥘

Here are some common use cases for the various utilities in this library.

Each recipe will include a **main idea** that briefly references what utilities to use, followed by a full **code example**, and finally an in-depth **explanation** of how the recipe works.

### HTTP/HTTPS Server 🌐

**Main idea**: To do.

**Code example**:

```ts
// To do.
```

**Explanation**: To do.

### Fixture file paths 🧱

**Main idea**: To do.

**Code example**:

```ts
// To do.
```

**Explanation**: To do.
