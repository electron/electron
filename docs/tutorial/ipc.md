# Communicating Between Processes

Inter-process communication (IPC) is key part of building feature-rich desktop
applications in Electron. For instance, you might want to call main process functionality
from your renderer's web UI, or trigger changes in your web contents from native menus.

In general, IPC in Electron is done by passing messages through developer-defined channels
with the `ipcMain` and `ipcRenderer` modules.

## Using `ipcRenderer` with `contextIsolation`

In order to

```js title='preload.js'

```
