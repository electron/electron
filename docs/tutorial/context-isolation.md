# Context Isolation

## What is it?

Context Isolation is a feature that ensures that both your `preload` scripts and Electron's internal logic run in a separate context to the website you load in a [`webContents`](../api/web-contents.md).  This is important for security purposes as it helps prevent the website from accessing Electron internals or the powerful APIs your preload script has access to.

This means that the `window` object that your preload script has access to is actually a **different** object than the website would have access to.  For example, if you set `window.hello = 'wave'` in your preload script and context isolation is enabled `window.hello` will be undefined if the website tries to access it.

Every single application should have context isolation enabled and from Electron 12 it will be enabled by default.

## How do I enable it?

From Electron 12, it will be enabled by default. For lower versions it is an option in the `webPreferences` option when constructing `new BrowserWindow`'s.

```javascript
const mainWindow = new BrowserWindow({
  webPreferences: {
    contextIsolation: true
  }
})
```

## Migration

> I used to provide APIs from my preload script using `window.X = apiObject` now what?

Exposing APIs from your preload script to the loaded website is a common usecase and there is a dedicated module in Electron to help you do this in a painless way.

**Before: With context isolation disabled**

```javascript
window.myAPI = {
  doAThing: () => {}
}
```

**After: With context isolation enabled**

```javascript
const { contextBridge } = require('electron')

contextBridge.exposeInMainWorld('myAPI', {
  doAThing: () => {}
})
```

The [`contextBridge`](../api/context-bridge.md) module can be used to **safely** expose APIs from the isolated context your preload script runs in to the context the website is running in. The API will also be accessible from the website on `window.myAPI` just like it was before.

You should read the `contextBridge` documentation linked above to fully understand its limitations.  For instance you can't send custom prototypes or symbols over the bridge.

## Security Considerations

Just enabling `contextIsolation` and using `contextBridge` does not automatically mean that everything you do is safe.  For instance this code is **unsafe**.

```javascript
// ❌ Bad code
contextBridge.exposeInMainWorld('myAPI', {
  send: ipcRenderer.send
})
```

It directly exposes a powerful API without any kind of argument filtering. This would allow any website to send arbitrary IPC messages which you do not want to be possible. The correct way to expose IPC-based APIs would instead be to provide one method per IPC message.

```javascript
// ✅ Good code
contextBridge.exposeInMainWorld('myAPI', {
  loadPreferences: () => ipcRenderer.invoke('load-prefs')
})
```

