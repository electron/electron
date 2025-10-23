import { getIPCRenderer } from '@electron/internal/renderer/ipc-renderer-bindings';

import { EventEmitter } from 'events';

const ipc = getIPCRenderer();
const internal = false;

class IpcRenderer extends EventEmitter implements Electron.IpcRenderer {
  send (channel: string, ...args: any[]) {
    return ipc.send(internal, channel, args);
  }

  sendSync (channel: string, ...args: any[]) {
    return ipc.sendSync(internal, channel, args);
  }

  sendToHost (channel: string, ...args: any[]) {
    return ipc.sendToHost(channel, args);
  }

  async invoke (channel: string, ...args: any[]) {
    const { error, result } = await ipc.invoke(internal, channel, args);
    if (error) {
      throw new Error(`Error invoking remote method '${channel}': ${error}`);
    }
    return result;
  }

  postMessage (channel: string, message: any, transferables: any) {
    return ipc.postMessage(channel, message, transferables);
  }
}

export default new IpcRenderer();
