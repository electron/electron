# contextBridge

> Create a safe, bi-directional, synchronous bridge across isolated contexts

Process: [Renderer](../glossary.md#renderer-process)

An example of exposing an API to a renderer from an isolated preload script is given below:

```javascript
// Preload (Isolated World)
const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld(
  'electron',
  {
    doThing: () => ipcRenderer.send('do-a-thing')
  }
)
```

```javascript
// Renderer (Main World)

window.electron.doThing()
```

## Glossary

### Main World

The "Main World" is the JavaScript context that your main renderer code runs in. By default, the
page you load in your renderer executes code in this world.

### Isolated World

When `contextIsolation` is enabled in your `webPreferences`, your `preload` scripts run in an
"Isolated World".  You can read more about context isolation and what it affects in the
[security](../tutorial/security.md#3-enable-context-isolation-for-remote-content) docs.

## Methods

The `contextBridge` module has the following methods:

### `contextBridge.exposeInMainWorld(apiKey, api)` _Experimental_

* `apiKey` String - The key to inject the API onto `window` with.  The API will be accessible on `window[apiKey]`.
* `api` Record<String, any> - Your API object, more information on what this API can be and how it works is available [below](#parameter--error--return-type-support)).

### `contextBridge.exposeSimpleObjectInMainWorld(objectKey, object)` _Experimental_

* `objectKey` String - The key to inject the object onto `window` with.  The object will be accessible on `window[objectKey]`.
* `object` Record<String, any> - Your object, the keys and values of this object must be **simple** data types from the [Type Support](#parameter--error--return-type-support)) table below.
Non-simple data types will result in an error being thrown.

This method can be used instead of `exposeInMainWorld` if you just want to provide a set of data and not expose any methods.  Sending
large data structures as "simple" can be around three times faster so if you only want to expose a data structure it can make sense to
use `exposeSimpleObjectInMainWorld`.

### `contextBridge.prepareSimpleFunction<T extends Function>(func)` _Experimental_

* `func` T - A function that you want to expose over the context bridge.

Returns `T`, a function of the same type as the function that was provided.

This method can be used to omptimize certain functions to be faster when called over the context bridge. The function
returned from this method will **only** support being called with, and returning, **simple** data types from the
[Type Support](#parameter--error--return-type-support) table.  Non-simple data types will result in an error being thrown.

Sending large data structures as "simple" can be around three times faster so if you plan on calling this function a lot
or sending a lot of data it can make sense to wrap it with `prepareSimpleFunction`.

## Usage

### API Objects

The `api` object provided to [`exposeInMainWorld`](#contextbridgeexposeinmainworldapikey-api-experimental) must be an object
whose keys are strings and values are a `Function`, `String`, `Number`, `Array`, `Boolean`, or another nested object that meets the same conditions.

`Function` values are proxied to the other context and all other values are **copied** and **frozen**. Any data / primitives sent in
the API object become immutable and updates on either side of the bridge do not result in an update on the other side.

An example of a complex API object is shown below:

```javascript
const { contextBridge } = require('electron')

contextBridge.exposeInMainWorld(
  'electron',
  {
    doThing: () => ipcRenderer.send('do-a-thing'),
    myPromises: [Promise.resolve(), Promise.reject(new Error('whoops'))],
    anAsyncFunction: async () => 123,
    data: {
      myFlags: ['a', 'b', 'c'],
      bootTime: 1234
    },
    nestedAPI: {
      evenDeeper: {
        youCanDoThisAsMuchAsYouWant: {
          fn: () => ({
            returnData: 123
          })
        }
      }
    }
  }
)
```

### API Functions

`Function` values that you bind through the `contextBridge` are proxied through Electron to ensure that contexts remain isolated.  This
results in some key limitations that we've outlined below.

#### Parameter / Error / Return Type support

Because parameters, errors and return values are **copied** when they are sent over the bridge, there are only certain types that can be used.
At a high level, if the type you want to use can be serialized and deserialized into the same object it will work.  A table of type support
has been included below for completeness:

| Type | Complexity | Parameter Support | Return Value Support | Limitations |
| ---- | ---------- | ----------------- | -------------------- | ----------- |
| `String` | Simple | ✅ | ✅ | N/A |
| `Number` | Simple | ✅ | ✅ | N/A |
| `Boolean` | Simple | ✅ | ✅ | N/A |
| `Object` | Simple | ✅ | ✅ | Keys must be supported using only "Simple" types in this table.  Values must be supported in this table.  Prototype modifications are dropped.  Sending custom classes will copy values but not the prototype. |
| `Array` | Simple | ✅ | ✅ | Same limitations as the `Object` type |
| [Cloneable Types](https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm) | Simple | ✅ | ✅ | See the linked document on cloneable types |
| `Error` | Complex | ✅ | ✅ | Errors that are thrown are also copied, this can result in the message and stack trace of the error changing slightly due to being thrown in a different context |
| `Promise` | Complex | ✅ | ✅ | Promises are only proxied if they are the return value or exact parameter.  Promises nested in arrays or objects will be dropped. |
| `Function` | Complex | ✅ | ✅ | Prototype modifications are dropped.  Sending classes or constructors will not work. |
| `Symbol` | N/A | ❌ | ❌ | Symbols cannot be copied across contexts so they are dropped |


If the type you care about is not in the above table, it is probably not supported.
