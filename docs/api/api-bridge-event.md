## Class: ApiBridgeEvent

> An event that frames receive as part of an API.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Create one with [`apiBridgeMain.event()`](api-bridge-main.md#apibridgemainevent) and put it on
an API you pass with [`frame.apiBridge.pass()`](api-bridge-frame-main.md). The same event
can be on several APIs, in several frames.

### Instance Methods

#### `apiBridgeEvent.emit(...args)`

* `...args` any[]

Calls the event's listeners with `args` in every frame that currently has an
API with this event. The arguments are copied once with the
[Structured Clone Algorithm][SCA]; this throws if they can't be.

[SCA]: https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm
