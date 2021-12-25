import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const { mainFrame: webFrame } = process._linkedBinding('electron_renderer_web_frame');

export interface GuestViewDelegate {
  dispatchEvent (eventName: string, props: Record<string, any>): void;
  reset(): void;
}

const DEPRECATED_EVENTS: Record<string, string> = {
  'page-title-updated': 'page-title-set'
} as const;

export function registerEvents (viewInstanceId: number, delegate: GuestViewDelegate) {
  ipcRendererInternal.on(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DESTROY_GUEST}-${viewInstanceId}`, function () {
    delegate.reset();
    delegate.dispatchEvent('destroyed', {});
  });

  ipcRendererInternal.on(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT}-${viewInstanceId}`, function (event, eventName, props) {
    if (DEPRECATED_EVENTS[eventName] != null) {
      delegate.dispatchEvent(DEPRECATED_EVENTS[eventName], props);
    }

    delegate.dispatchEvent(eventName, props);
  });
}

export function deregisterEvents (viewInstanceId: number) {
  ipcRendererInternal.removeAllListeners(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DESTROY_GUEST}-${viewInstanceId}`);
  ipcRendererInternal.removeAllListeners(`${IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT}-${viewInstanceId}`);
}

export function createGuest (iframe: HTMLIFrameElement, elementInstanceId: number, params: Record<string, any>): Promise<number> {
  if (!(iframe instanceof HTMLIFrameElement)) {
    throw new Error('Invalid embedder frame');
  }

  const embedderFrameId = webFrame.getWebFrameId(iframe.contentWindow!);
  if (embedderFrameId < 0) { // this error should not happen.
    throw new Error('Invalid embedder frame');
  }

  return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_VIEW_MANAGER_CREATE_AND_ATTACH_GUEST, embedderFrameId, elementInstanceId, params);
}

export function detachGuest (guestInstanceId: number) {
  return ipcRendererUtils.invokeSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_DETACH_GUEST, guestInstanceId);
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
