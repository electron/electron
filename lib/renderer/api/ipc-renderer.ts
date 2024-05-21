
import { IpcRendererInvokeEvent } from 'electron/renderer';
import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = false;
class IpcRenderer extends EventEmitter implements Electron.IpcRenderer {
  private _invokeHandlers: Map<string, (e: IpcRendererInvokeEvent, ...args: any[]) => void> = new Map();

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

  handle: Electron.IpcRenderer['handle'] = (method, fn) => {
    if (this._invokeHandlers.has(method)) {
      throw new Error(`Attempted to register a second handler for '${method}'`);
    }
    if (typeof fn !== 'function') {
      throw new TypeError(`Expected handler to be a function, but found type '${typeof fn}'`);
    }

    this._invokeHandlers.set(method, fn);
  };

  handleOnce: Electron.IpcRenderer['handleOnce'] = (method, fn) => {
    this.handle(method, (e, ...args) => {
      this.removeHandler(method);
      return fn(e, ...args);
    });
  };

  removeHandler (method: string) {
    this._invokeHandlers.delete(method);
  }
}

export default new IpcRenderer();
