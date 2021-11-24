import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = true;

export const ipcRendererInternal = new EventEmitter() as any as ElectronInternal.IpcRendererInternal;

ipcRendererInternal.send = function (channel, ...args) {
  return ipc.send(internal, channel, args);
};

ipcRendererInternal.sendSync = function (channel, ...args) {
  return ipc.sendSync(internal, channel, args);
};

ipcRendererInternal.invoke = async function<T> (channel: string, ...args: any[]) {
  const { error, result } = await ipc.invoke<T>(internal, channel, args);
  if (error) {
    throw new Error(`Error invoking remote method '${channel}': ${error}`);
  }
  return result;
};
