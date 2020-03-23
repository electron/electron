import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as errorUtils from '@electron/internal/common/error-utils';

type IPCHandler = (event: ElectronInternal.IpcMainInternalEvent, ...args: any[]) => any

const callHandler = async function (handler: IPCHandler, event: ElectronInternal.IpcMainInternalEvent, args: any[], reply: (args: any[]) => void) {
  try {
    const result = await handler(event, ...args);
    reply([null, result]);
  } catch (error) {
    reply([errorUtils.serialize(error)]);
  }
};

export const handle = function <T extends IPCHandler> (channel: string, handler: T) {
  ipcMainInternal.on(channel, (event, requestId, ...args) => {
    callHandler(handler, event, args, responseArgs => {
      if (requestId) {
        event._replyInternal(`${channel}_RESPONSE_${requestId}`, ...responseArgs);
      } else {
        event.returnValue = responseArgs;
      }
    });
  });
};

let nextId = 0;

export function invokeInWebContents<T> (sender: Electron.WebContentsInternal, sendToAll: boolean, command: string, ...args: any[]) {
  return new Promise<T>((resolve, reject) => {
    const requestId = ++nextId;
    const channel = `${command}_RESPONSE_${requestId}`;
    ipcMainInternal.on(channel, function handler (
      event, error: Electron.SerializedError, result: any
    ) {
      if (event.sender !== sender) {
        console.error(`Reply to ${command} sent by unexpected WebContents (${event.sender.id})`);
        return;
      }

      ipcMainInternal.removeListener(channel, handler);

      if (error) {
        reject(errorUtils.deserialize(error));
      } else {
        resolve(result);
      }
    });

    if (sendToAll) {
      sender._sendInternalToAll(command, requestId, ...args);
    } else {
      sender._sendInternal(command, requestId, ...args);
    }
  });
}
