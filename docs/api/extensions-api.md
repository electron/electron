## Class: Extensions

> Load and interact with extensions.

Process: [Main](../glossary.md#main-process)<br />
_This class is not exported from the `'electron'` module. It is only available as a return value of other methods in the Electron API._

Instances of the `Extensions` class are accessed by using `extensions` property of
a `Session`.

### Instance Events

The following events are available on instances of `Extensions`:

#### Event: 'extension-loaded'

Returns:

* `event` Event
* `extension` [Extension](structures/extension.md)

Emitted after an extension is loaded. This occurs whenever an extension is
added to the "enabled" set of extensions. This includes:

* Extensions being loaded from `Extensions.loadExtension`.
* Extensions being reloaded:
  * from a crash.
  * if the extension requested it ([`chrome.runtime.reload()`](https://developer.chrome.com/extensions/runtime#method-reload)).

#### Event: 'extension-unloaded'

Returns:

* `event` Event
* `extension` [Extension](structures/extension.md)

Emitted after an extension is unloaded. This occurs when
`Session.removeExtension` is called.

#### Event: 'extension-ready'

Returns:

* `event` Event
* `extension` [Extension](structures/extension.md)

Emitted after an extension is loaded and all necessary browser state is
initialized to support the start of the extension's background page.

### Instance Methods

The following methods are available on instances of `Extensions`:

#### `extensions.loadExtension(path[, options])`

* `path` string - Path to a directory containing an unpacked Chrome extension
* `options` Object (optional)
  * `allowFileAccess` boolean - Whether to allow the extension to read local files over `file://`
    protocol and inject content scripts into `file://` pages. This is required e.g. for loading
    DevTools extensions on `file://` URLs. Defaults to false.

Returns `Promise<Extension>` - resolves when the extension is loaded.

This method will raise an exception if the extension could not be loaded. If
there are warnings when installing the extension (e.g. if the extension
requests an API that Electron does not support) then they will be logged to the
console.

Note that Electron does not support the full range of Chrome extensions APIs.
See [Supported Extensions APIs](extensions.md#supported-extensions-apis) for
more details on what is supported.

Note that in previous versions of Electron, extensions that were loaded would
be remembered for future runs of the application. This is no longer the case:
`loadExtension` must be called on every boot of your app if you want the
extension to be loaded.

```js
const { app, session } = require('electron')

const path = require('node:path')

app.whenReady().then(async () => {
  await session.defaultSession.extensions.loadExtension(
    path.join(__dirname, 'react-devtools'),
    // allowFileAccess is required to load the DevTools extension on file:// URLs.
    { allowFileAccess: true }
  )
  // Note that in order to use the React DevTools extension, you'll need to
  // download and unzip a copy of the extension.
})
```

This API does not support loading packed (.crx) extensions.

> [!NOTE]
> This API cannot be called before the `ready` event of the `app` module
> is emitted.

> [!NOTE]
> Loading extensions into in-memory (non-persistent) sessions is not
> supported and will throw an error.

#### `extensions.removeExtension(extensionId)`

* `extensionId` string - ID of extension to remove

Unloads an extension.

> [!NOTE]
> This API cannot be called before the `ready` event of the `app` module
> is emitted.

#### `extensions.getExtension(extensionId)`

* `extensionId` string - ID of extension to query

Returns `Extension | null` - The loaded extension with the given ID.

> [!NOTE]
> This API cannot be called before the `ready` event of the `app` module
> is emitted.

#### `extensions.getAllExtensions()`

Returns `Extension[]` - A list of all loaded extensions.

> [!NOTE]
> This API cannot be called before the `ready` event of the `app` module
> is emitted.
