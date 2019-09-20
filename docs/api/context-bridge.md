# contextBridge

> Create a safe, bi-directional, synchronous bridge across isolated contexts

Process: [Renderer](../glossary.md#renderer-process)

An example of exposing a basic API to a renderer from an isolated preload script is given below:

```javascript
// Preload (Isolated World)
const { contextBridge, ipcRenderer } = require('electron')

contextBridge.bindAPIInMainWorld(
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

The "Main World" is the javascript context that your main renderer code runs in.  By default the page you load in your renderer
executes code in this world.

### Isolated World

When `contextIsolation` is enabled in your `webPreferences` your `preload` scripts run in an "Isolated World".  You can read more about
context isolation and what it affects in the [BrowserWindow](browser-window.md) docs.

## Methods

The `contextBridge` module has the following instance methods:

### `contextBridge.bindAPIInMainWorld(apiKey, api[, options])`

* `apiKey` String - The key to inject the API onto `window` with.  The API will be accessible on `window[apiKey]`.
* `api` Record<String, any> - Your API object, more information on what this API can be and how it works is available below.
* `options` Object (optional)
  * `allowReverseBinding` Boolean - Allows the renderer to bind a [Reverse Binding](#reverse-bindings) back into the isolated context to allow bidirection communication.

### `contextBridge.getReverseBinding(apiKey)`

* `apiKey` String - The key for the API that you bound into the main world that corresponds to the reverse binding you wish to fetch.

Returns `Record<String, any> | null` the reverse bound API provided by the main world.

## Usage

### API Objects

The `api` object provided to both [`bindAPIInMainWorld`](#contextbridgebindapiinmainworldapikey-api-options) and [Reverse Bindings (`window[apiKey].bind`)](#reverse-bindings) must be an object
whose keys are strings and values are a `Function`, `String`, `Number`, `Array` or another nested object that meets the same conditions.

`Function` values are proxied to the other context and all other values are **copied** and **frozen**.  I.e. Any data / primtives sent in
the API object become immutable and updates on either side of the bridge do not result in an update on the other side.

**NOTE:** Arrays containing Functions are currently not supported, Functions must not be a child of an array.

An example complex API object is uncluded below.

```javascript
const { contextBridge } = require('electron')

contextBridge.bindAPIInMainWorld(
  'electron',
  {
    doThing: () => ipcRenderer.send('do-a-thing'),
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

Because parameters, errors and return values are **copied** when they are sent over the bridge there are only certain types that can be used.
At a high level if the type you want to use can be serialized and un-serialized into the same object it will work.  A table of type support
has been included below for completeness.

| Type | Parameter Support | Return Value Support | Limitations |
| ---- | ----------------- | -------------------- | ----------- |
| `String` | ✅ | ✅ | N/A |
| `Number` | ✅ | ✅ | N/A |
| `Boolean` | ✅ | ✅ | N/A |
| `Object` | ✅ | ✅ | Keys and values must be supported in this table.  Prototype modifications are dropped.  Sending custom classes will copy values but not the prototype. |
| `Error` | ✅ | ✅ | Errors that are thrown are also copied, this can result in the message and stack trace of the error changing slightly due to being thrown in a different context |
| `Promise` | ❌ | ✅ | Promises are proxied through the bridge **only** if it is the return value, promises as function parameters are dropped |
| `Function` | ❌ | ❌ | N/A |
| `Symbol` | ❌ | ❌ | Symbols cannot be copied across contexts so they are dropped |


If the type you care about is not in the above table it is probably not supported.

### Reverse Bindings

The `contextBridge.bindAPIInMainWorld` allows you to expose APIs from the **Isolated World** the preload script runs in to the **Main World** the renderer runs in.  If you want
to allow the main world to provide an API back to the isolated world you can set the `allowReverseBinding` option to `true`.  By setting that option the API exposed in the main
world will have a new method injected on it on the `bind` key.  You can use this method by providing it a single `api` object in the same structure as the original `bindAPIInMainWorld`
API.  An example is included below:

```javascript
// Main World
window[apiKey].bind({
  doThingInMainWorld: () => doAThing()
})
```

Then from the isolated world you can use the `getReverseBinding` API to get a handle to that reverse binding.  Example is continued below.

```javascript
// Isolated World
contextBridge.getReverseBinding(apiKey).doThingInMainWorld()
```
