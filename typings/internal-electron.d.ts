/// <reference path="../electron.d.ts" />

 /**
 * This file augments the Electron TS namespace with the internal APIs
 * that are not documented but are used by Electron internally
 */

declare namespace Electron {
  enum ProcessType {
    browser = 'browser',
    renderer = 'renderer',
    worker = 'worker'
  }

  interface App {
    setVersion(version: string): void;
    setDesktopName(name: string): void;
    setAppPath(path: string | null): void;
  }

  interface SerializedError {
    message: string;
    stack?: string,
    name: string,
    from: Electron.ProcessType,
    cause: SerializedError,
    __ELECTRON_SERIALIZED_ERROR__: true
  }

  interface IpcRendererInternal extends Electron.IpcRenderer {
    sendToAll(webContentsId: number, channel: string, ...args: any[]): void
  }
}
