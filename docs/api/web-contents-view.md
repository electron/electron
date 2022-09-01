# WebContentsView

> A View that displays a WebContents.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

```javascript
// TODO example
```

## Class: WebContentsView extends `View`

> A View that displays a WebContents.

Process: [Main](../glossary.md#main-process)

`WebContentsView` is an [EventEmitter][event-emitter].

### `new WebContentsView([options])`

* `options` WebPreferences (optional) - See ... (TODO)

### Instance Properties

Objects created with `new WebContentsView` have the following properties:

```javascript
const { WebContentsView } = require('electron')
// In this example `win` is our instance
const win = new WebContentsView()
```

#### `win.webContents` _Readonly_

A `WebContents` property containing a reference to the displayed `WebContents`.
