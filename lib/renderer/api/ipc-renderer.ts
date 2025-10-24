import { getIPCRenderer } from '@electron/internal/renderer/ipc-renderer-bindings';

import { EventEmitter } from 'events';

const ipc = getIPCRenderer();
const internal = false;

class IpcRenderer extends EventEmitter implements Electron.IpcRenderer {
  send (channel: string, ...args: unknown[]) {
    return ipc.send(internal, channel, args);
  }

  sendSync (channel: string, ...args: unknown[]) {
    return ipc.sendSync(internal, channel, args);
  }

  sendToHost (channel: string, ...args: unknown[]) {
    return ipc.sendToHost(channel, args);
  }

  async invoke (channel: string, ...args: unknown[]) {
    const { error, result } = await ipc.invoke(internal, channel, args);
    if (error) {
      throw new Error(`Error invoking remote method '${channel}': ${error}`);
    }
    return result;
  }

  postMessage (channel: string, message: unknown, transferables: Transferable[]) {
    return ipc.postMessage(channel, message, transferables);
  }
}

export default new IpcRenderer();
