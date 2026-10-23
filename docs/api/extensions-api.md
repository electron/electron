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

#### `extensions.setTabMode(mode)` _Experimental_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/55270
```
-->

* `mode` string - Can be `all-web-contents` or `registered`.

Sets which `webContents` of this session are tabs to its extensions. The
default is `all-web-contents`.

* `all-web-contents` - Every `webContents` in the session is a tab, unless it
  was added with [`extensions.addView`](#extensionsaddviewwebcontents-details-experimental)
  as a `popup` or `side-panel`.
* `registered` - Only a `webContents` added with `extensions.addView` as a `tab`
  is a tab.

A `webContents` that is not a tab has no tab ID. Extensions cannot target it
with `chrome.tabs`, `chrome.scripting` or `chrome.tabs.sendMessage`, its
network requests are reported with a `tabId` of `-1`, and messages it sends
have no `sender.tab`.

Set the mode before loading extensions and creating tabs: changing it later
changes whether existing `webContents` are tabs.

#### `extensions.addView(webContents, details)` _Experimental_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/55270
```
-->

* `webContents` [WebContents](web-contents.md) - Must belong to this session.
* `details` Object
  * `type` string (optional) - Can be `tab`, `popup` or `side-panel`. Default
    is `tab`. Cannot be changed once added.
  * `windowId` Integer - ID of the window that contains the view. The app
    chooses its window IDs, which must not be negative.
    [`BaseWindow.id`](base-window.md#winid-readonly) is a good choice.
  * `index` Integer (optional) - Zero-based position of the tab in its window.
    Required for a `tab`.
  * `active` boolean (optional) - Whether the tab is the active tab of its
    window. Default is `false`. Setting it to `true` makes the window's other
    tabs inactive.
  * `highlighted` boolean (optional) - Default is the value of `active`.
  * `pinned` boolean (optional) - Default is `false`.
  * `groupId` Integer (optional) - Default is `-1`.
  * `splitViewId` Integer (optional) - Default is `-1`.
  * `openerTabId` Integer (optional)
  * `discarded` boolean (optional) - Default is `false`.
  * `autoDiscardable` boolean (optional) - Default is `false`.
  * `frozen` boolean (optional) - Default is `false`.
  * `lastAccessed` number (optional) - Default is the time the `webContents`
    was last active.
  * `favIconUrl` string (optional)
  * `sessionId` string (optional)

Registers a `webContents` hosted by the app with the extension system, either
as a tab or as an extension view that is not a tab.

The `type` decides what the view is to extensions:

* `tab` - A tab. Its tab ID is always `webContents.id`.
* `popup` - An extension's action popup. Not a tab.
* `side-panel` - An extension's side panel. Not a tab.

A `popup` or `side-panel` is never a tab, whatever the
[tab mode](#extensionssettabmodemode-experimental), and accepts no detail other than
`type` and `windowId`. The `type` is also what the extension gets as the
`contextType` of the view from
[`chrome.runtime.getContexts`](https://developer.chrome.com/docs/extensions/reference/api/runtime#method-getContexts).

For a tab, Electron itself uses `windowId`, `index` and `active`. It copies the
other details into the
[`Tab`](https://developer.chrome.com/docs/extensions/reference/api/tabs#type-Tab)
object that extensions receive as `sender.tab` and from `chrome.tabs.get`,
`chrome.tabs.query` and `chrome.tabs.update`.

`details` may also contain keys that are not listed above. Electron adds each
one to the `Tab` object as a custom property, with its value unchanged. The
values must be serializable to JSON. Every extension that can see the tab can
read its custom properties, so do not store anything private in them.

The properties `id`, `url`, `pendingUrl`, `title`, `status`, `audible`,
`mutedInfo`, `incognito`, `selected`, `width` and `height` are set by Electron
and cannot be set in `details`. A key that names any other property of
Chrome's `Tab` type must have that property's type. A future version of
Electron may add properties to `Tab`, at which point a custom property with
the same name becomes subject to these rules.

This method throws an error if:

* `webContents` is destroyed, belongs to another session, or was already added.
* `webContents` is an extension's background page or offscreen document.
* `type` is not one of the values above, or `windowId` is missing.
* `index` is missing for a `tab`, or a `popup` or `side-panel` is given a
  detail other than `type` and `windowId`.
* `details` sets a property that Electron sets, gives a property of `Tab` a
  value of the wrong type, or is larger than 64 KiB when serialized to JSON.

The view is removed automatically when its `webContents` is destroyed.

```js
const { app, BaseWindow, WebContentsView, session } = require('electron')

app.whenReady().then(() => {
  const ses = session.fromPartition('persist:browser')
  ses.extensions.setTabMode('registered')

  const win = new BaseWindow()
  win.on('focus', () => ses.extensions.setFocusedWindow(win.id))

  const view = new WebContentsView({ webPreferences: { session: ses } })
  win.contentView.addChildView(view)
  ses.extensions.addView(view.webContents, {
    windowId: win.id,
    index: 0,
    active: true
  })
  view.webContents.loadURL('https://electronjs.org')
})
```

#### `extensions.updateView(webContents, details)` _Experimental_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/55270
```
-->

* `webContents` [WebContents](web-contents.md) - A `webContents` added with
  [`extensions.addView`](#extensionsaddviewwebcontents-details-experimental).
* `details` Object
  * `windowId` Integer (optional)
  * `index` Integer (optional)
  * `active` boolean (optional)
  * `highlighted` boolean | null (optional)
  * `pinned` boolean | null (optional)
  * `groupId` Integer | null (optional)
  * `splitViewId` Integer | null (optional)
  * `openerTabId` Integer | null (optional)
  * `discarded` boolean | null (optional)
  * `autoDiscardable` boolean | null (optional)
  * `frozen` boolean | null (optional)
  * `lastAccessed` number | null (optional)
  * `favIconUrl` string | null (optional)
  * `sessionId` string | null (optional)

Changes the details of a view added with `extensions.addView`. The keys of
`details`, custom properties included, are merged into the stored details. A
key set to `null` is removed, which restores its default.

This method throws an error if `webContents` was not added, if `details`
contains `type`, and for the same invalid details as `extensions.addView`. To
change the type of a view, remove it and add it again.

#### `extensions.removeView(webContents)` _Experimental_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/55270
```
-->

* `webContents` [WebContents](web-contents.md)

Removes a view added with `extensions.addView`. Does nothing if `webContents`
was not added.

#### `extensions.setFocusedWindow(windowId)` _Experimental_

<!--
```YAML history
added:
  - pr-url: https://github.com/electron/electron/pull/55270
```
-->

* `windowId` Integer | null - The `windowId` of the focused window, or `null`
  if none of the app's windows is focused.

Sets the window that extensions get as the last focused window. It is also the
current window for an extension context that is not in a view added with
`extensions.addView`, such as a service worker.

When the session's tab mode is `registered`, a `chrome.tabs` method called
without a tab ID acts on the active tab of the caller's current window, and
fails if there is no such window or tab.
