import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = true;

const ipcRendererInternal = new EventEmitter() as any as Electron.IpcRendererInternal;
ipcRendererInternal.send = function (channel, ...args) {
  return ipc.send(internal, channel, args);
};

ipcRendererInternal.sendSync = function (channel, ...args) {
  return ipc.sendSync(internal, channel, args)[0];
};

ipcRendererInternal.sendTo = function (webContentsId, channel, ...args) {
  return ipc.sendTo(internal, false, webContentsId, channel, args);
};

ipcRendererInternal.sendToAll = function (webContentsId, channel, ...args) {
  return ipc.sendTo(internal, true, webContentsId, channel, args);
};

ipcRendererInternal.invoke = async function<T> (channel: string, ...args: any[]) {
  const { error, result } = await ipc.invoke<T>(internal, channel, args);
  if (error) {
    throw new Error(`Error invoking remote method '${channel}': ${error}`);
  }
  return result;
};

export { ipcRendererInternal };
