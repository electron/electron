import { EventEmitter } from 'events';
import { IpcMainInvokeEvent } from 'electron/main';

export class IpcMainImpl extends EventEmitter {
  private _invokeHandlers: Map<string, (e: IpcMainInvokeEvent, ...args: any[]) => void> = new Map();

  handle: Electron.IpcMain['handle'] = (method, fn) => {
    if (this._invokeHandlers.has(method)) {
      throw new Error(`Attempted to register a second handler for '${method}'`);
    }
    if (typeof fn !== 'function') {
      throw new Error(`Expected handler to be a function, but found type '${typeof fn}'`);
    }
    this._invokeHandlers.set(method, async (e, ...args) => {
      try {
        e._reply(await Promise.resolve(fn(e, ...args)));
      } catch (err) {
        e._throw(err as Error);
      }
    });
  }

  handleOnce: Electron.IpcMain['handleOnce'] = (method, fn) => {
    this.handle(method, (e, ...args) => {
      this.removeHandler(method);
      return fn(e, ...args);
    });
  }

  removeHandler (method: string) {
    this._invokeHandlers.delete(method);
  }
}
