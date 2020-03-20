import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';

type IPCHandler = (event: Electron.IpcRendererEvent, ...args: any[]) => any

export const handle = function <T extends IPCHandler> (channel: string, handler: T) {
  ipcRendererInternal.on(channel, async (event, requestId, ...args) => {
    const replyChannel = `${channel}_RESPONSE_${requestId}`;
    try {
      event.sender.send(replyChannel, null, await handler(event, ...args));
    } catch (error) {
      event.sender.send(replyChannel, error);
    }
  });
};

export function invokeSync<T> (command: string, ...args: any[]): T {
  const [error, result] = ipcRendererInternal.sendSync(command, ...args);

  if (error) {
    throw error;
  } else {
    return result;
  }
}
