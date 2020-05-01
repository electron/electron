import { webFrame, IpcMessageEvent } from 'electron';
import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';

import { WebViewImpl } from '@electron/internal/renderer/web-view/web-view-impl';

const WEB_VIEW_EVENTS: Record<string, Array<string>> = {
  'load-commit': ['url', 'isMainFrame'],
  'did-attach': [],
  'did-finish-load': [],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-frame-finish-load': ['isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'dom-ready': [],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'context-menu': ['params'],
  'devtools-opened': [],
  'devtools-closed': [],
  'devtools-focused': [],
  'new-window': ['url', 'frameName', 'disposition', 'options'],
  'will-navigate': ['url'],
  'did-start-navigation': ['url', 'isInPlace', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-navigate': ['url', 'httpResponseCode', 'httpStatusText'],
  'did-frame-navigate': ['url', 'httpResponseCode', 'httpStatusText', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'did-navigate-in-page': ['url', 'isMainFrame', 'frameProcessId', 'frameRoutingId'],
  'focus-change': ['focus', 'guestInstanceId'],
  close: [],
  crashed: [],
  'plugin-crashed': ['name', 'version'],
  destroyed: [],
  'page-title-updated': ['title', 'explicitSet'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'media-started-playing': [],
  'media-paused': [],
  'found-in-page': ['result'],
  'did-change-theme-color': ['themeColor'],
  'update-target-url': ['url']
};

const DEPRECATED_EVENTS: Record<string, string> = {
  'page-title-updated': 'page-title-set'
};

const dispatchEvent = function (
  webView: WebViewImpl, eventName: string, eventKey: string, ...args: Array<any>
) {
  if (DEPRECATED_EVENTS[eventName] != null) {
    dispatchEvent(webView, DEPRECATED_EVENTS[eventName], eventKey, ...args);
  }

  const domEvent = new Event(eventName) as ElectronInternal.WebViewEvent;
  WEB_VIEW_EVENTS[eventKey].forEach((prop, index) => {
    (domEvent as any)[prop] = args[index];
  });

  webView.dispatchEvent(domEvent);

  if (eventName === 'load-commit') {
    webView.onLoadCommit(domEvent);
  } else if (eventName === 'focus-change') {
    webView.onFocusChange();
  }
};

export function registerEvents (webView: WebViewImpl, viewInstanceId: number) {
  ipcRendererInternal.on(`ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-${viewInstanceId}`, function () {
    webView.guestInstanceId = undefined;
    webView.reset();
    const domEvent = new Event('destroyed');
    webView.dispatchEvent(domEvent);
  });

  ipcRendererInternal.on(`ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-${viewInstanceId}`, function (event, eventName, ...args) {
    dispatchEvent(webView, eventName, eventName, ...args);
  });

  ipcRendererInternal.on(`ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-${viewInstanceId}`, function (event, channel, ...args) {
    const domEvent = new Event('ipc-message') as IpcMessageEvent;
    domEvent.channel = channel;
    domEvent.args = args;

    webView.dispatchEvent(domEvent);
  });
}

export function deregisterEvents (viewInstanceId: number) {
  ipcRendererInternal.removeAllListeners(`ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-${viewInstanceId}`);
  ipcRendererInternal.removeAllListeners(`ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-${viewInstanceId}`);
  ipcRendererInternal.removeAllListeners(`ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-${viewInstanceId}`);
}

export function createGuest (params: Record<string, any>): Promise<number> {
  return ipcRendererInternal.invoke('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST', params);
}

export function attachGuest (
  elementInstanceId: number, guestInstanceId: number, params: Record<string, any>, contentWindow: Window
) {
  const embedderFrameId = (webFrame as ElectronInternal.WebFrameInternal).getWebFrameId(contentWindow);
  if (embedderFrameId < 0) { // this error should not happen.
    throw new Error('Invalid embedder frame');
  }
  ipcRendererInternal.invoke('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', embedderFrameId, elementInstanceId, guestInstanceId, params);
}

export function detachGuest (guestInstanceId: number) {
  return ipcRendererUtils.invokeSync('ELECTRON_GUEST_VIEW_MANAGER_DETACH_GUEST', guestInstanceId);
}

export const guestViewInternalModule = {
  deregisterEvents,
  createGuest,
  attachGuest,
  detachGuest
};
