import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = false;

const ipcRenderer = new EventEmitter() as Electron.IpcRenderer;
ipcRenderer.send = function (channel, ...args) {
  return ipc.send(internal, channel, args);
};

ipcRenderer.sendSync = function (channel, ...args) {
  return ipc.sendSync(internal, channel, args);
};

ipcRenderer.sendToHost = function (channel, ...args) {
  return ipc.sendToHost(channel, args);
};

ipcRenderer.sendTo = function (webContentsId, channel, ...args) {
  return ipc.sendTo(webContentsId, channel, args);
};

ipcRenderer.invoke = async function (channel, ...args) {
  const { error, result } = await ipc.invoke(internal, channel, args);
  if (error) {
    throw new Error(`Error invoking remote method '${channel}': ${error}`);
  }
  return result;
};

ipcRenderer.postMessage = function (channel: string, message: any, transferables: any) {
  return ipc.postMessage(channel, message, transferables);
};

export default ipcRenderer;
