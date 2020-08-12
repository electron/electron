import { NewWindowEvent, Referrer, IpcMainEvent, IpcMainInvokeEvent, webContents } from 'electron/main';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { parseWebViewWebPreferences } from '@electron/internal/common/parse-features-string';
import { syncMethods, asyncMethods, properties } from '@electron/internal/common/web-view-methods';
import { serialize } from '@electron/internal/common/type-utils';

import type { BrowserWindow, WebContents } from 'electron/main';

enum Disposition {
  default = 'default',
  ['foreground-tab'] = 'foreground-tab',
  ['background-tab'] = 'background-tab',
  ['new-window'] = 'new-window',
  ['save-to-disk'] = 'save-to-disk',
  other = 'other'
}

interface WebVM {
  addGuest(guestInstanceId: number, elementInstanceId: number, embedder: WebContents, guest: any, webPreferences: {[key: string]: any}): void;
  removeGuest(embedder: WebContents, guestInstanceId: number): void
}

// Doesn't exist in early initialization.
let webViewManager: WebVM | null = null;

const supportedWebViewEvents: Array<string> = [
  'load-commit',
  'did-attach',
  'did-finish-load',
  'did-fail-load',
  'did-frame-finish-load',
  'did-start-loading',
  'did-stop-loading',
  'dom-ready',
  'console-message',
  'context-menu',
  'devtools-opened',
  'devtools-closed',
  'devtools-focused',
  'will-navigate',
  'did-start-navigation',
  'did-navigate',
  'did-frame-navigate',
  'did-navigate-in-page',
  'focus-change',
  'close',
  'crashed',
  'render-process-gone',
  'plugin-crashed',
  'destroyed',
  'page-title-updated',
  'page-favicon-updated',
  'enter-html-full-screen',
  'leave-html-full-screen',
  'media-started-playing',
  'media-paused',
  'found-in-page',
  'did-change-theme-color',
  'update-target-url'
];

const guestInstances: { [key: string]: WebContents & Electron.WebContentsInternal | any; } = {};
const embedderElementsMap: { [key: string]: number } = {};

function sanitizeOptionsForGuest (options: BrowserWindow) {
  const ret = { ...options };
  // WebContents values can't be sent over IPC.
  delete ret.webContents;
  return ret;
}

// Create a new guest instance.
const createGuest = function (embedder: Electron.WebContentsInternal & WebContents, params: Record<string, any>): number {
  if (webViewManager == null) {
    webViewManager = process._linkedBinding('electron_browser_web_view_manager');
  }

  const guest = (webContents as any).create({
    type: 'webview',
    partition: params.partition,
    embedder: embedder
  });
  const guestInstanceId = guest.id;
  guestInstances[guestInstanceId] = {
    guest: guest,
    embedder: embedder
  };

  // Clear the guest from map when it is destroyed.
  guest.once('destroyed', () => {
    if (Object.prototype.hasOwnProperty.call(guestInstances, guestInstanceId)) {
      detachGuest(embedder, guestInstanceId);
    }
  });

  // Init guest web view after attached.
  guest.once('did-attach', function (this: WebContents, event: Electron.IpcRendererEvent): void {
    params = this.attachParams;
    delete this.attachParams;

    const previouslyAttached = this.viewInstanceId != null;
    this.viewInstanceId = params.instanceId;

    // Only load URL and set size on first attach
    if (previouslyAttached) {
      return;
    }

    if (params.src) {
      const opts: {[key: string]: string} = {};
      if (params.httpreferrer) {
        opts.httpReferrer = params.httpreferrer;
      }
      if (params.useragent) {
        opts.userAgent = params.useragent;
      }
      this.loadURL(params.src, opts);
    }
    embedder.emit('did-attach-webview', event, guest);
  });

  const sendToEmbedder = (channel: string, ...args: any[]) => {
    if (!embedder.isDestroyed()) {
      embedder._sendInternal(`${channel}-${guest.viewInstanceId}`, args);
    }
  };

  // Dispatch events to embedder.
  const fn = function (event: string) {
    guest.on(event, function (_: any, ...args: any[]) {
      sendToEmbedder('ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT', event, ...args);
    });
  };
  for (const event of supportedWebViewEvents) {
    fn(event);
  }

  // The 'options' here vs. in documentation for 'webContents.on(new-window)' (BrowserWindowConstructorOptions) are different.
  guest.on('new-window', function (event: NewWindowEvent, url: string, frameName: string, disposition: Disposition, options: BrowserWindow, additionalFeatures: String[], referrer: Referrer) {
    sendToEmbedder('ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT', 'new-window', url,
      frameName, disposition, sanitizeOptionsForGuest(options),
      additionalFeatures, referrer);
  });

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_: any, channel: string, args: any[]) {
    sendToEmbedder('ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE', channel, ...args);
  });

  // Notify guest of embedder window visibility when it is ready
  // FIXME Remove once https://github.com/electron/electron/issues/6828 is fixed
  guest.on('dom-ready', function () {
    const guestInstance = guestInstances[guestInstanceId];
    if (guestInstance != null && guestInstance.visibilityState != null) {
      guest._sendInternal('ELECTRON_GUEST_INSTANCE_VISIBILITY_CHANGE', guestInstance.visibilityState);
    }
  });

  return guestInstanceId;
};

// Attach the guest to an element of embedder.
const attachGuest = function (event: Electron.IpcMainEvent, embedderFrameId: number, elementInstanceId: number, guestInstanceId: number, params: Record<string, any>) {
  const embedder = event.sender;
  // Destroy the old guest when attaching.
  const key = `${embedder.id}-${elementInstanceId}`;
  const oldGuestInstanceId = embedderElementsMap[key];
  if (oldGuestInstanceId != null) {
    // Reattachment to the same guest is just a no-op.
    if (oldGuestInstanceId === guestInstanceId) {
      return;
    }

    const oldGuestInstance = guestInstances[oldGuestInstanceId];
    if (oldGuestInstance) {
      oldGuestInstance.guest.detachFromOuterFrame();
    }
  }

  const guestInstance = guestInstances[guestInstanceId];
  // If this isn't a valid guest instance then do nothing.
  if (!guestInstance) {
    throw new Error(`Invalid guestInstanceId: ${guestInstanceId}`);
  }
  const { guest } = guestInstance;
  if (guest.hostWebContents !== event.sender) {
    throw new Error(`Access denied to guestInstanceId: ${guestInstanceId}`);
  }

  // If this guest is already attached to an element then remove it
  if (guestInstance.elementInstanceId) {
    const oldKey = `${guestInstance.embedder.id}-${guestInstance.elementInstanceId}`;
    delete embedderElementsMap[oldKey];

    // Remove guest from embedder if moving across web views
    if (guest.viewInstanceId !== params.instanceId) {
      webViewManager!.removeGuest(guestInstance.embedder, guestInstanceId);
      guestInstance.embedder._sendInternal(`ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-${guest.viewInstanceId}`);
    }
  }

  // parse the 'webpreferences' attribute string, if set
  // this uses the same parsing rules as window.open uses for its features
  const parsedWebPreferences =
    typeof params.webpreferences === 'string'
      ? parseWebViewWebPreferences(params.webpreferences)
      : null;

  const webPreferences: Electron.WebPreferences & {[key: string]: any} = {
    guestInstanceId: guestInstanceId,
    nodeIntegration: params.nodeintegration != null ? params.nodeintegration : false,
    nodeIntegrationInSubFrames: params.nodeintegrationinsubframes != null ? params.nodeintegrationinsubframes : false,
    enableRemoteModule: params.enableremotemodule,
    plugins: params.plugins,
    zoomFactor: embedder.zoomFactor,
    disablePopups: !params.allowpopups,
    webSecurity: !params.disablewebsecurity,
    enableBlinkFeatures: params.blinkfeatures,
    disableBlinkFeatures: params.disableblinkfeatures,
    ...parsedWebPreferences
  };

  if (params.preload) {
    webPreferences.preloadURL = params.preload;
  }

  // Security options that guest will always inherit from embedder
  const inheritedWebPreferences = new Map([
    ['contextIsolation', true],
    ['javascript', false],
    ['nativeWindowOpen', true],
    ['nodeIntegration', false],
    ['enableRemoteModule', false],
    ['sandbox', true],
    ['nodeIntegrationInSubFrames', false],
    ['enableWebSQL', false]
  ]);

  // Inherit certain option values from embedder
  const lastWebPreferences: Electron.WebPreferences & {[key: string]: any} = embedder.getLastWebPreferences();
  // Within the for loop, we cannot infer what the specific key is based on the above typings.
  // So, we need to provide a general typing for keys in lastWebPreferences and webPreferences.
  // https://www.typescriptlang.org/docs/handbook/interfaces.html#indexable-types
  for (const [name, value] of inheritedWebPreferences) {
    if (lastWebPreferences[name] === value) {
      webPreferences[name] = value;
    }
  }

  embedder.emit('will-attach-webview', event, webPreferences, params);
  if (event.defaultPrevented) {
    if (guest.viewInstanceId == null) guest.viewInstanceId = params.instanceId;
    guest.destroy();
    return;
  }

  guest.attachParams = params;
  embedderElementsMap[key] = guestInstanceId;

  guest.setEmbedder(embedder);
  guestInstance.embedder = embedder;
  guestInstance.elementInstanceId = elementInstanceId;

  watchEmbedder(embedder);
  webViewManager!.addGuest(guestInstanceId, elementInstanceId, embedder, guest, webPreferences);
  guest.attachToIframe(embedder, embedderFrameId);
};

// Remove an guest-embedder relationship.
const detachGuest = function (embedder: WebContents, guestInstanceId: number) {
  const guestInstance = guestInstances[guestInstanceId];

  if (!guestInstance) return;

  if (embedder !== guestInstance.embedder) {
    return;
  }
  webViewManager!.removeGuest(embedder, guestInstanceId);
  delete guestInstances[guestInstanceId];

  const key = `${embedder.id}-${guestInstance.elementInstanceId}`;
  delete embedderElementsMap[key];
};

// Once an embedder has had a guest attached we watch it for destruction to
// destroy any remaining guests.
const watchedEmbedders = new Set();
const watchEmbedder = function (embedder: any) {
  if (watchedEmbedders.has(embedder)) {
    return;
  }
  watchedEmbedders.add(embedder);

  // Forward embedder window visiblity change events to guest
  const onVisibilityChange = function (visibilityState: VisibilityState) {
    for (const guestInstanceId of Object.keys(guestInstances)) {
      const guestInstance = guestInstances[guestInstanceId];
      guestInstance.visibilityState = visibilityState;
      if (guestInstance.embedder === embedder) {
        guestInstance.guest._sendInternal('ELECTRON_GUEST_INSTANCE_VISIBILITY_CHANGE', visibilityState);
      }
    }
  };
  embedder.on('-window-visibility-change', onVisibilityChange);

  embedder.once('will-destroy', () => {
    // Usually the guestInstances is cleared when guest is destroyed, but it
    // may happen that the embedder gets manually destroyed earlier than guest,
    // and the embedder will be invalid in the usual code path.
    for (const guestInstanceId of Object.keys(guestInstances)) {
      const guestInstance = guestInstances[guestInstanceId];
      if (guestInstance.embedder === embedder) {
        detachGuest(embedder, parseInt(guestInstanceId));
      }
    }
    // Clear the listeners.
    embedder.removeListener('-window-visibility-change', onVisibilityChange);
    watchedEmbedders.delete(embedder);
  });
};

const isWebViewTagEnabledCache = new WeakMap();

export const isWebViewTagEnabled = function (contents: WebContents): WeakMap<object, any> {
  if (!isWebViewTagEnabledCache.has(contents)) {
    const webPreferences = contents.getLastWebPreferences() || {};
    isWebViewTagEnabledCache.set(contents, !!webPreferences.webviewTag);
  }

  return isWebViewTagEnabledCache.get(contents);
};

const makeSafeHandler = function (channel: string, handler: Function) {
  return (event: IpcMainInvokeEvent, ...args: any[]) => {
    if (isWebViewTagEnabled(event.sender)) {
      return handler(event, ...args);
    } else {
      console.error(`<webview> IPC message ${channel} sent by WebContents with <webview> disabled (${event.sender.id})`);
      throw new Error('<webview> disabled');
    }
  };
};

const handleMessage = function (channel: string, handler: Function) {
  ipcMainInternal.handle(channel, makeSafeHandler(channel, handler));
};

const handleMessageSync = function (channel: string, handler: Function) {
  ipcMainUtils.handleSync(channel, makeSafeHandler(channel, handler));
};

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST', function (event: { sender: Electron.WebContentsInternal; }, params: Record<string, any>): number {
  return createGuest(event.sender, params);
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', function (event: IpcMainEvent, embedderFrameId: number, elementInstanceId: number, guestInstanceId: number, params: Record<string, any>): void {
  try {
    attachGuest(event, embedderFrameId, elementInstanceId, guestInstanceId, params);
  } catch (error) {
    console.error(`Guest attach failed: ${error}`);
  }
});

handleMessageSync('ELECTRON_GUEST_VIEW_MANAGER_DETACH_GUEST', function (event: IpcMainEvent, guestInstanceId: number): void {
  return detachGuest(event.sender, guestInstanceId);
});

// this message is sent by the actual <webview>
ipcMainInternal.on('ELECTRON_GUEST_VIEW_MANAGER_FOCUS_CHANGE', function (event: ElectronInternal.IpcMainInternalEvent, focus: boolean, guestInstanceId: number): boolean | void {
  const guest = getGuest(guestInstanceId);
  if (guest === event.sender) {
    event.sender.emit('focus-change', {}, focus, guestInstanceId);
  } else {
    console.error(`focus-change for guestInstanceId: ${guestInstanceId}`);
  }
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_CALL', function (event: IpcMainEvent, guestInstanceId: number, method: string, args: any[]): Function | Error {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!asyncMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return guest[method](...args);
});

handleMessageSync('ELECTRON_GUEST_VIEW_MANAGER_CALL', function (event: IpcMainEvent, guestInstanceId: number, method: string, args: any[]): Function | Error {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!syncMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return guest[method](...args);
});

handleMessageSync('ELECTRON_GUEST_VIEW_MANAGER_PROPERTY_GET', function (event: IpcMainEvent, guestInstanceId: number, property: string): Error | any {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  return guest[property];
});

handleMessageSync('ELECTRON_GUEST_VIEW_MANAGER_PROPERTY_SET', function (event: IpcMainEvent, guestInstanceId: number, property: string, val: any): Error | void {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  guest[property] = val;
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_CAPTURE_PAGE', async function (event: IpcMainEvent, guestInstanceId: number, args: any[]): Promise<any> {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);

  return serialize(await guest.capturePage(...args));
});

// Returns WebContents from its guest id hosted in given webContents.
const getGuestForWebContents = function (guestInstanceId: number, contents: WebContents) {
  const guest = getGuest(guestInstanceId);
  if (!guest) {
    throw new Error(`Invalid guestInstanceId: ${guestInstanceId}`);
  }
  if (guest.hostWebContents !== contents) {
    throw new Error(`Access denied to guestInstanceId: ${guestInstanceId}`);
  }
  return guest;
};

// Returns WebContents from its guest id.
const getGuest = function (guestInstanceId: number) {
  const guestInstance = guestInstances[guestInstanceId];
  if (guestInstance != null) return guestInstance.guest;
};

exports.isWebViewTagEnabled = isWebViewTagEnabled;
