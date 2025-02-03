import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';

import type { ServiceWorkerMain } from 'electron/main';

const v8Util = process._linkedBinding('electron_common_v8_util');

const addReturnValueToEvent = (event: Electron.IpcMainEvent | Electron.IpcMainServiceWorkerEvent) => {
  Object.defineProperty(event, 'returnValue', {
    set: (value) => event._replyChannel.sendReply(value),
    get: () => {}
  });
};

/**
 * Listens for IPC dispatch events on `api`.
 *
 * NOTE: Currently this only supports dispatching IPCs for ServiceWorkerMain.
 */
export function addIpcDispatchListeners (api: NodeJS.EventEmitter, serviceWorkers: Electron.ServiceWorkers) {
  const getServiceWorkerFromEvent = (event: Electron.IpcMainServiceWorkerEvent | Electron.IpcMainServiceWorkerInvokeEvent): ServiceWorkerMain | undefined => {
    return serviceWorkers._getWorkerFromVersionIDIfExists(event.versionId);
  };
  const addServiceWorkerPropertyToEvent = (event: Electron.IpcMainServiceWorkerEvent | Electron.IpcMainServiceWorkerInvokeEvent) => {
    Object.defineProperty(event, 'serviceWorker', {
      get: () => serviceWorkers.getWorkerFromVersionID(event.versionId)
    });
  };

  api.on('-ipc-message' as any, function (event: Electron.IpcMainEvent | Electron.IpcMainServiceWorkerEvent, channel: string, args: any[]) {
    const internal = v8Util.getHiddenValue<boolean>(event, 'internal');

    if (internal) {
      ipcMainInternal.emit(channel, event, ...args);
    } else if (event.type === 'service-worker') {
      addServiceWorkerPropertyToEvent(event);
      getServiceWorkerFromEvent(event)?.ipc.emit(channel, event, ...args);
    }
  } as any);

  api.on('-ipc-invoke' as any, async function (event: Electron.IpcMainInvokeEvent | Electron.IpcMainServiceWorkerInvokeEvent, channel: string, args: any[]) {
    const internal = v8Util.getHiddenValue<boolean>(event, 'internal');

    const replyWithResult = (result: any) => event._replyChannel.sendReply({ result });
    const replyWithError = (error: Error) => {
      console.error(`Error occurred in handler for '${channel}':`, error);
      event._replyChannel.sendReply({ error: error.toString() });
    };

    const targets: (Electron.IpcMainServiceWorker | ElectronInternal.IpcMainInternal | undefined)[] = [];

    if (internal) {
      targets.push(ipcMainInternal);
    } else if (event.type === 'service-worker') {
      addServiceWorkerPropertyToEvent(event);
      const workerIpc = getServiceWorkerFromEvent(event)?.ipc;
      targets.push(workerIpc);
    }

    const target = targets.find(target => (target as any)?._invokeHandlers.has(channel));
    if (target) {
      const handler = (target as any)._invokeHandlers.get(channel);
      try {
        replyWithResult(await Promise.resolve(handler(event, ...args)));
      } catch (err) {
        replyWithError(err as Error);
      }
    } else {
      replyWithError(new Error(`No handler registered for '${channel}'`));
    }
  } as any);

  api.on('-ipc-message-sync' as any, function (event: Electron.IpcMainEvent | Electron.IpcMainServiceWorkerEvent, channel: string, args: any[]) {
    const internal = v8Util.getHiddenValue<boolean>(event, 'internal');
    addReturnValueToEvent(event);
    if (internal) {
      ipcMainInternal.emit(channel, event, ...args);
    } else if (event.type === 'service-worker') {
      addServiceWorkerPropertyToEvent(event);
      getServiceWorkerFromEvent(event)?.ipc.emit(channel, event, ...args);
    }
  } as any);

  api.on('-ipc-ports' as any, function (event: Electron.IpcMainEvent | Electron.IpcMainServiceWorkerEvent, channel: string, message: any, ports: any[]) {
    event.ports = ports.map(p => new MessagePortMain(p));
    if (event.type === 'service-worker') {
      addServiceWorkerPropertyToEvent(event);
      getServiceWorkerFromEvent(event)?.ipc.emit(channel, event, message);
    }
  } as any);
}
