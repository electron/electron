# `<webview2>` Tag

Embeds an independently owned WebContents in an iframe. Enable `webview2Tag`
in the host window's webPreferences. Create the contents in the main process:

```js
const { BrowserWindow, webContents } = require('electron');
const contents = webContents.create({ type: 'webview2' });
const host = new BrowserWindow({ webPreferences: { webview2Tag: true } });
contents.loadURL('https://example.com');
```

The host document uses `<webview2 web-contents-id="42"></webview2>` with the
actual `contents.id`. The tag has no WebContents method proxies.

`attached` and `detached` are DOM CustomEvents. Their `detail` contains
`webContentsId` and a unique `attachmentId`. Each attachment creates a new empty
iframe, reads its LocalFrameToken, and attaches through synchronous IPC. `attached` means the browser embedding
relationship exists, rather than that the first compositor frame has appeared.
Main-process detachment notifications are delivered asynchronously.

Removing the element, navigating or closing its host, and calling
`contents.detach()` do not destroy the contents. `contents.destroy()` does not
remove the element. Detach before attaching the same contents in another window.
The independent contents remain alive until explicitly destroyed or app shutdown,
including after JavaScript references have been collected.

After detachment the element stays empty and does not automatically reclaim the
contents. Reinsert it, or remove and reset `web-contents-id`, to attach again.
The same session must be used by the contents and its host.

This API uses Chromium's experimental unowned inner WebContents implementation.
