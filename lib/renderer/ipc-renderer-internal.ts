import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = true;

class IpcRendererInternal extends EventEmitter implements ElectronInternal.IpcRendererInternal {
  send (channel: string, ...args: any[]) {
    return ipc.send(internal, channel, args);
  }

  sendSync (channel: string, ...args: any[]) {
    return ipc.sendSync(internal, channel, args);
  }

  async invoke<T> (channel: string, ...args: any[]) {
    const { error, result } = await ipc.invoke<T>(internal, channel, args);
    if (error) {
      throw new Error(`Error invoking remote method '${channel}': ${error}`);
    }
    return result;
  };
}

export const ipcRendererInternal = new IpcRendererInternal();
