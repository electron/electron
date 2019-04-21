# Secure Mode

This is an opt-in locked down mode designed for apps loading remote content.

## Default values

Changes the default values of the following `webPreferences`:
- `sandbox: true`
- `contextIsolation: true`
- `nativeWindowOpen: true`
- `enableRemoteModule: false`

Potentially unsafe options must be overriden explicitly:
```js
let win = new BrowserWindow({
  webPreferences: {
    sandbox: false,
    contextIsolation: false,
    nativeWindowOpen: false,
    enableRemoteModule: true // filtering events need to be handled
  }
})
```

## Disabled features

Removes the ability to use the following `webPreferences`:
- `nodeIntegration`
- `nodeIntegrationInWorker`

Preload scripts must be used to explicitly expose native APIs safely.

## Event handling

When the following events are unhandled, `event.preventDefault()` is called implicitly:

`webContents`:
- `new-window`
- `will-attach-webview`
- `select-bluetooth-device`

`webContents` / `app`:
- `desktop-capturer-get-sources`
- `remote-require`
- `remote-get-builtin`
- `remote-get-global`
- `remote-get-current-window`
- `remote-get-current-web-contents`
- `remote-get-guest-web-contents`

These events must be handled (with potential filtering / argument sanitization) explicitly:
```js
app.on('desktop-capturer-get-sources', (event) => {
  // suppress event.preventDefault()
})
```

## Permission handling

When no handlers are set using the following APIs, permissions are denied implicitly:
- `session.setPermissionCheckHandler()`
- `session.setPermissionRequestHandler()`

Request handlers must be used to explicitly grant the permissions (potentially asking the user for consent):
```js
session.setPermissionCheckHandler((webContents, permission) => {
  return true
})

session.setPermissionRequestHandler((webContents, permission, callback) => {
  callback(true)
})
```
