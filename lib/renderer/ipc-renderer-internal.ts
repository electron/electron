const binding = process.atomBinding('ipc')
const v8Util = process.atomBinding('v8_util')

// Created by init.js.
export const ipcRendererInternal: Electron.IpcRendererInternal = v8Util.getHiddenValue(global, 'ipc-internal')
const internal = true

ipcRendererInternal.send = function (channel, ...args) {
  return binding.send(internal, channel, args)
}

ipcRendererInternal.sendSync = function (channel, ...args) {
  return binding.sendSync(internal, channel, args)[0]
}

ipcRendererInternal.sendTo = function (webContentsId, channel, ...args) {
  return binding.sendTo(internal, false, webContentsId, channel, args)
}

ipcRendererInternal.sendToAll = function (webContentsId, channel, ...args) {
  return binding.sendTo(internal, true, webContentsId, channel, args)
}
