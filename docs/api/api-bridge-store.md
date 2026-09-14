## Class: ApiBridgeStore

> A value that frames receive as part of an API and keep up to date.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Create one with [`apiBridgeMain.store()`](api-bridge-main.md#apibridgemainstoreinitialvalue)
and put it on an API you pass with [`frame.apiBridge.pass()`](api-bridge-frame-main.md).
Each frame with the API holds the current value, so reading it in the renderer
never asks the main process. The same store can be on several APIs, in several
frames.

### Instance Methods

#### `apiBridgeStore.get()`

Returns `any` - The current value.

#### `apiBridgeStore.set(value)`

* `value` any

Replaces the value and sends it to every frame that currently has an API with
this store, where the store's subscribers are called with it. The value is
copied once with the [Structured Clone Algorithm][SCA]; this throws, and keeps
the old value, if it can't be.

[SCA]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm
