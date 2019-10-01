const { ipc } = process.electronBinding('ipc')
const v8Util = process.electronBinding('v8_util')

// Created by init.js.
export const ipcRendererInternal = v8Util.getHiddenValue<Electron.IpcRendererInternal>(global, 'ipc-internal')
const internal = true

ipcRendererInternal.send = function (channel, ...args) {
  return ipc.send(internal, channel, args)
}

ipcRendererInternal.sendSync = function (channel, ...args) {
  return ipc.sendSync(internal, channel, args)[0]
}

ipcRendererInternal.sendTo = function (webContentsId, channel, ...args) {
  return ipc.sendTo(internal, false, webContentsId, channel, args)
}

ipcRendererInternal.sendToAll = function (webContentsId, channel, ...args) {
  return ipc.sendTo(internal, true, webContentsId, channel, args)
}
