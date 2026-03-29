import { webContents } from 'electron/main';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';

type IPCHandler = (event: ElectronInternal.IpcMainInternalEvent, ...args: any[]) => any

export const handleSync = function <T extends IPCHandler> (channel: string, handler: T) {
  ipcMainInternal.on(channel, async (event, ...args) => {
    try {
      event.returnValue = [null, await handler(event, ...args)];
    } catch (error) {
      event.returnValue = [error];
    }
  });
};

let nextId = 0;

export function invokeInWebContents<T> (sender: Electron.WebContents, command: string, ...args: any[]) {
  return new Promise<T>((resolve, reject) => {
    const requestId = ++nextId;
    const channel = `${command}_RESPONSE_${requestId}`;
    const cleanup = () => {
      (ipcMainInternal as any).removeListener(channel, handler);
    };
    function handler (event: any, error: Error, result: any) {
      if (event.type !== 'frame' || event.sender !== sender) {
        console.error(`Reply to ${command} sent by unexpected sender`);
        return;
      }

      cleanup();
      sender.removeListener('destroyed' as any, cleanup);

      if (error) {
        reject(error);
      } else {
        resolve(result);
      }
    }
    ipcMainInternal.on(channel, handler);
    sender.once('destroyed' as any, cleanup);

    sender._sendInternal(command, requestId, ...args);
  });
}

export function invokeInWebFrameMain<T> (sender: Electron.WebFrameMain, command: string, ...args: any[]) {
  return new Promise<T>((resolve, reject) => {
    const requestId = ++nextId;
    const channel = `${command}_RESPONSE_${requestId}`;
    const frameTreeNodeId = (sender as any).frameTreeNodeId;
    const cleanup = () => {
      (ipcMainInternal as any).removeListener(channel, handler);
    };
    function handler (event: any, error: Error, result: any) {
      if (event.type !== 'frame' || event.frameTreeNodeId !== frameTreeNodeId) {
        console.error(`Reply to ${command} sent by unexpected sender`);
        return;
      }

      cleanup();
      const contents = webContents.fromFrame(sender);
      if (contents) {
        contents.removeListener('destroyed' as any, cleanup);
      }

      if (error) {
        reject(error);
      } else {
        resolve(result);
      }
    }
    ipcMainInternal.on(channel, handler);
    const contents = webContents.fromFrame(sender);
    if (contents) {
      contents.once('destroyed' as any, cleanup);
    }

    sender._sendInternal(command, requestId, ...args);
  });
}
