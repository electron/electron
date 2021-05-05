import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { webViewEvents } from '@electron/internal/common/web-view-events';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

export interface GuestViewDelegate {
  dispatchEvent (eventName: string, props: Record<string, any>): void;
  reset(): void;
}

const DEPRECATED_EVENTS: Record<string, string> = {
  'page-title-updated': 'page-title-set'
} as const;

const dispatchEvent = function (delegate: GuestViewDelegate, eventName: string, eventKey: string, ...args: Array<any>) {
  if (DEPRECATED_EVENTS[eventName] != null) {
    dispatchEvent(delegate, DEPRECATED_EVENTS[eventName], eventKey, ...args);
  }

  const props: Record<string, any> = {};
  webViewEvents[eventKey].forEach((prop, index) => {
    props[prop] = args[index];
  });

  delegate.dispatchEvent(eventName, props);
};

export function registerEvents (viewInstanceId: number, delegate: GuestViewDelegate) {
  ipcRendererInternal.on(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DESTROY_GUEST}-${viewInstanceId}`, function () {
    delegate.reset();
    delegate.dispatchEvent('destroyed', {});
  });

  ipcRendererInternal.on(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT}-${viewInstanceId}`, function (event, eventName, ...args) {
    dispatchEvent(delegate, eventName, eventName, ...args);
  });

  ipcRendererInternal.on(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_IPC_MESSAGE}-${viewInstanceId}`, function (event, channel, ...args) {
    delegate.dispatchEvent('ipc-message', { channel, args });
  });
}

export function deregisterEvents (viewInstanceId: number) {
  ipcRendererInternal.removeAllListeners(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DESTROY_GUEST}-${viewInstanceId}`);
  ipcRendererInternal.removeAllListeners(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT}-${viewInstanceId}`);
  ipcRendererInternal.removeAllListeners(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_IPC_MESSAGE}-${viewInstanceId}`);
}

export function createGuest (params: Record<string, any>): Promise<number> {
  return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_VIEW_MANAGER_CREATE_GUEST, params);
}

export function attachGuest (embedderFrameId: number, elementInstanceId: number, guestInstanceId: number, params: Record<string, any>) {
  return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_VIEW_MANAGER_ATTACH_GUEST, embedderFrameId, elementInstanceId, guestInstanceId, params);
}

export function detachGuest (guestInstanceId: number) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_DETACH_GUEST, guestInstanceId);
}

export function capturePage (guestInstanceId: number, args: any[]) {
  return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_VIEW_MANAGER_CAPTURE_PAGE, guestInstanceId, args);
}

export function invoke (guestInstanceId: number, method: string, args: any[]) {
  return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_VIEW_MANAGER_CALL, guestInstanceId, method, args);
}

export function invokeSync (guestInstanceId: number, method: string, args: any[]) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_CALL, guestInstanceId, method, args);
}

export function propertyGet (guestInstanceId: number, name: string) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_PROPERTY_GET, guestInstanceId, name);
}

export function propertySet (guestInstanceId: number, name: string, value: any) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_PROPERTY_SET, guestInstanceId, name, value);
}
