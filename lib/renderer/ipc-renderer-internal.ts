import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = true;

const ipcRendererInternal = new EventEmitter() as any as ElectronInternal.IpcRendererInternal;
ipcRendererInternal.send = function (channel, ...args) {
  return ipc.send(internal, channel, args);
};

ipcRendererInternal.sendSync = function (channel, ...args) {
  return ipc.sendSync(internal, channel, args);
};

ipcRendererInternal.sendTo = function (webContentsId, channel, ...args) {
  return ipc.sendTo(internal, webContentsId, channel, args);
};

ipcRendererInternal.invoke = async function<T> (channel: string, ...args: any[]) {
  const { error, result } = await ipc.invoke<T>(internal, channel, args);
  if (error) {
    throw new Error(`Error invoking remote method '${channel}': ${error}`);
  }
  return result;
};

ipcRendererInternal.onMessageFromMain = function (channel: string, listener: (event: Electron.IpcRendererEvent, ...args: any[]) => void) {
  return ipcRendererInternal.on(channel, (event, ...args) => {
    if (event.senderId !== 0) {
      console.error(`Message ${channel} sent by unexpected WebContents (${event.senderId})`);
      return;
    }

    listener(event, ...args);
  });
};

ipcRendererInternal.onceMessageFromMain = function (channel: string, listener: (event: Electron.IpcRendererEvent, ...args: any[]) => void) {
  return ipcRendererInternal.on(channel, function wrapper (event, ...args) {
    if (event.senderId !== 0) {
      console.error(`Message ${channel} sent by unexpected WebContents (${event.senderId})`);
      return;
    }

    ipcRendererInternal.removeListener(channel, wrapper);
    listener(event, ...args);
  });
};

export { ipcRendererInternal };
