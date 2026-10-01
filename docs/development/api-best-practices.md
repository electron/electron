# Electron API Design Guidelines

The following are a set of guidelines for building Electron APIs. This document is maintained by the [API Working Group](https://github.com/electron/governance/tree/main/wg-api).

## Questions to ask for every API change

These questions are intended to prompt reflection and bring up things that the API author may not have considered when designing the API.

### Does this API change alter any existing behavior? In what way?

API changes which alter existing behavior can cause apps to break unexpectedly when they upgrade to a newer version of Electron. Even seemingly minor behavior changes can often have unintended consequences. If possible, changes to Electron’s APIs should not alter behavior of existing code.

If the behavior must change to support the feature, the change should be listed in [the breaking changes document](https://github.com/electron/electron/blob/main/docs/breaking-changes.md). Additionally, consider whether the change can be introduced in a way which permits a deprecation cycle, for instance introducing the new API under a new name and deprecating the old name while keeping the behavior unchanged for apps using the API under the old name.

> [!NOTE]
> Breaking changes should be that which affects JavaScript runtime usage. If a change affects type definitions which breaks static compilation of a TypeScript app, that is not enough to be considered a breaking change.

### How will this API be extended in the future?

What additional changes can you imagine being made to this API in the future? Are there any features that are not in the first version of a change you’re making that you would like to include in the future?

Take some time to anticipate how the API might evolve, and design the API in such a way that reasonably-anticipated changes can be made in future without breaking backwards compatibility.

One key technique for future-proofing APIs is to pass options to method as an object, rather than as positional arguments. This is both more readable and allows flexibility to accommodate future changes. In particular, removing arguments from a method which takes its options positionally is not possible without introducing backwards incompatible changes.

```typescript
// Bad:

function whatever(a: string, b?: boolean) {
  /* ... */
}

// Good:

function whatever(opts: { a: string; b?: boolean }) {
  /* ... */
}
```

See https://w3ctag.github.io/design-principles/#prefer-dict-to-bool for more details.

### How will isolated components (e.g. third-party libraries) interact with this API?

When designing an API, consider that an app might have custom code and third-party libraries that both interact with that API. Can the API be designed so that multiple callers don't interfere with each other?

If an API accepts configuring options, should it provide the ability to append to rather than replace those options?
Can third-party libraries use the API without knowing what app-specific code is also using the API?

See the [style guide for designing APIs when dealing with arrays.](#provide-createreadupdatedelete-options-when-dealing-with-arrays)

### What underlying Chromium or OS features does this API rely on?

If the API you’re changing relies on underlying features provided by Chromium or by the operating system, how stable are those underlying features? How might those underlying features change in the future?

Can we design this API to insulate users from such changes? i.e, if the underlying API changes, can we keep the API we expose to users unchanged—such that they can upgrade Electron without having to change their code?

### Can this API be implemented on all the platforms that Electron supports?

If this API interacts with the underlying platform (Windows, macOS or Linux), consider whether the API makes sense on all platforms. It’s OK for an API to be platform-specific, but if the underlying feature exists on more than one platform, the API should be designed in such a way that it can behave uniformly on all supported platforms if possible. If you haven’t already implemented the API on other platforms, research how the API could be implemented on other platforms and take care to ensure that the API design works for all of them.

### Can this API be implemented as a native Node addon?

It’s tempting to add functionality that you need to Electron, in order to avoid having to deal with compiling native modules. But not every feature belongs in the core of Electron. Consider whether the functionality you’re trying to achieve might be better off as a native Node addon.

### Should this API be asynchronous?

If the proposed API is synchronous, consider whether it would make sense for it to do any asynchronous work in the future. If so, should the API be asynchronous instead?

It’s very difficult to change an API from being synchronous to being asynchronous—so consider from the outset whether it would make sense to define the API as asynchronous (i.e. returning a Promise), even if it doesn’t yet perform any asynchronous work.

However: don’t make an API asynchronous unless there’s a good reason to. Asynchronous APIs are harder to use. See https://w3ctag.github.io/design-principles/#synchronous.

## Style guide

This guide is intended to steer Electron’s APIs towards consistency and ease of use. Often, there are many possible roughly-equivalent ways of implementing an API. This style guide is designed to help you choose between them.

### Prefer functions to classes

Electron’s API surface should be mostly “flat”, that is, composed of functions which do not reference `this`.

Classes should be used only when there is a persistent underlying resource that must be managed, such as a socket, a window, or some other handle.

### Prefer promises to callbacks

If your code is asynchronous, it should return a Promise rather than taking a callback as a parameter.

For example:

```javascript
const win = new BrowserWindow(options)
const result = await win.webContents.executeJavaScript('void 0')
```

Subscription-style APIs that returns results multiple times should still be implemented with callbacks.

```typescript
// Use callback if results are provided multiple times.
win.hookWindowMessage('MESSAGE', (args...) => {
  // ...
})
```

See https://w3ctag.github.io/design-principles/#promises.

### Manage resource lifetimes automatically

Users should not have to reason about when a resource should be destroyed. destroy() methods are an anti-pattern. Leave resource management up to V8’s garbage collector, and use `gin_helper::Pinnable` to keep resources alive when needed.

### Preserve run-to-completion semantics

Don’t modify data accessed via JavaScript APIs while a JavaScript event loop is running.

See https://w3ctag.github.io/design-principles/#js-rtc for details.

### Prefer dictionary parameters over primitive parameters

API methods should generally use dictionary parameters instead of a series of optional primitive parameters.

See https://w3ctag.github.io/design-principles/#prefer-dict-to-bool for details.

### Make function parameters optional if possible

If a parameter for an API function has a reasonable default value, make that parameter optional and specify the default value.

Optional parameters should be named to make the default value obvious without being named negatively (see https://w3ctag.github.io/design-principles/#naming-optional-parameters).

See https://w3ctag.github.io/design-principles/#optional-parameters for details.

### Cancel asynchronous operations using AbortSignal

If an asynchronous function can be cancelled, allow authors to pass in an AbortSignal as part of an options dictionary.

See https://w3ctag.github.io/design-principles/#aborting.

### Use strings for constants and enums

If your API needs a constant, or a set of enumerated values, use string values.

Strings are easier for developers to inspect, and in JavaScript engines there is no performance benefit from using integers instead of strings.

If you need to express a state which is a combination of properties, which might be expressed as a bitmask in another language, use a dictionary object instead. This object can be passed around as easily as a single bitmask value.

See https://w3ctag.github.io/design-principles/#string-constants.

### Use milliseconds for time measurement

If you are designing an API that accepts a time measurement, express the time measurement in milliseconds.

Even if seconds (or some other time unit) are more natural in the domain of an API, sticking with milliseconds ensures that APIs are interoperable with one another. This means that authors don’t need to convert values used in one API to be used in another API, or keep track of which time unit is needed where.

See https://w3ctag.github.io/design-principles/#milliseconds

### Provide create/read/update/delete options when dealing with arrays

If an API is designed to accept an array of items, consider providing methods of creating, reading, updating, and deleting items.

In the case of isolated components (e.g. third-party libraries), one might want to add a single item rather than replacing existings items.

```js
// Bad: third-party libraries can only replace registered schemes
protocol.registerSchemesAsPrivileged([
  { scheme: 'app', privileges: { standard: true } }
])

// Good: third-party libraries can create, read, update, and delete items
if (
  protocol.getPrivilegedSchemes().includes((scheme) => scheme.scheme === 'app')
) {
  protocol.unregisterSchemeAsPrivileged('app')
}
protocol.registerSchemeAsPrivileged({
  scheme: 'app',
  privileges: { standard: true }
})
protocol.updatePrivilegedScheme({
  scheme: 'app',
  privileges: { secure: true }
})
```

### Return null when no result from lookup functions

APIs that intend to find an object by an identifier should return `null` in the
case that the instance is not found.

## Classes

### Use a class's name as the module name

If a module exports only one class, the module's name should be the class's name.

For example:

```javascript
const { BrowserWindow } = require('electron')
```

If a module includes methods or more than one class, the module's name should be in `lowerCamelCase`.

For example:

```javascript
const { nativeImage } = require('electron')
nativeImage.createEmpty()
```

### Use a constructor if there's an inheritance relationship

There are 2 styles for creating instances of classes:

```javascript
const { BrowserWindow, nativeImage } = require('electron')

// Constructor
const w = new BrowserWindow(options)

// Factory method
nativeImage.createFromPath('/path')
```

#### Constructors

If the class has an inheritance relationship with other public classes, a constructor should be used for that class.

For example:

```javascript
const { BrowserWindow, TopLevelWindow } = require('electron')
const win = new BrowserWindow(options)
```

This enables users to detect inheritance relationships between classes.

```javascript
if (win instanceof TopLevelWindow) {
  // ...
}
```

#### Factory methods

If the class accepts different kinds of parameters with options, consider using factory methods.

For example:

```javascript
// Using module methods
const { nativeImage } = require('electron')
nativeImage.createFromNamedImage('NSImageNameListViewTemplate', [-1, 0, 1])

// Using static methods
Buffer.from('7468697320697320612074c3a97374', 'hex')
```

Factory methods help reduce ambiguity and usability issues. The `Buffer` API provides a good example of this: [Node.js Buffer API Changes][node-buffer-api].

Additionally, due to lack of function overloading, JavaScript class constructors can only be implemented with a single C++ function and are error-prone when parsing different kinds of parameters with C++. Conversely, factory methods can be perfectly mapped to C++ functions, thereby making code much easier to read and maintain.

### Use `EventEmitter` to emit events

If the class generates events, it should inherit from `EventEmitter` and use its interface for emitting events.

### Use instance properties for non-assignable objects

If an API of the class returns a _non-assignable_ `Object`, and the returned objects always strictly equal each other, the API should be implemented as property.

For example:

```javascript
const win = new BrowserWindow(options)
assert.strictEqual(win.webContents, win.webContents)
assert.strictEqual(win.webContents.session, win.webContents.session)
assert.strictEqual(win.webContents.debugger, win.webContents.debugger)
```

If an API of the class returns [non-primitives][primitive], and the returned values do not always strictly equal each other, it should _not_ be implemented as property.

For example:

```javascript
// Should NOT do these, as they have no effect.
const win = new BrowserWindow(options)
win.bounds.x = 42
win.browserViews.push(new BrowserView())
```

### Use methods instead of properties when options are needed

If an API of the class accepts options, or may accept option in future, it should be implemented as method rather than a property.

For example:

```javascript
const win = new BrowserWindow(options)
win.setBounds(bounds, animate)
```

### Use getters and setters, as well as properties

If the APIs of the class are used to set/get values, they should be implemented as methods.

If the type of the value being set/get is [primitive][primitive], it should be also added as a property interface for the value.

For example:

```javascript
const win = new BrowserWindow(options)
win.setTitle('str')
console.log(win.getTitle())
// Property also works.
win.title = 'str'
console.log(win.title)
```

[node-buffer-api]: https://medium.com/@jasnell/node-js-buffer-api-changes-3c21f1048f97
[primitive]: https://developer.mozilla.org/en-US/docs/Glossary/Primitive
