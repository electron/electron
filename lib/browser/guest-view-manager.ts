import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { parseWebViewWebPreferences } from '@electron/internal/browser/parse-features-string';
import { webViewEvents } from '@electron/internal/browser/web-view-events';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';
import { syncMethods, asyncMethods, properties, navigationHistorySyncMethods } from '@electron/internal/common/web-view-methods';

import { webContents } from 'electron/main';

interface GuestInstance {
  elementInstanceId: number;
  visibilityState?: DocumentVisibilityState;
  embedder: Electron.WebContents;
  guest: Electron.WebContents;
}

const webViewManager = process._linkedBinding('electron_browser_web_view_manager');
const netBinding = process._linkedBinding('electron_common_net');

const supportedWebViewEvents = Object.keys(webViewEvents);

const guestInstances = new Map<number, GuestInstance>();
const embedderElementsMap = new Map<string, number>();

function makeWebPreferences (embedder: Electron.WebContents, params: Record<string, any>) {
  // parse the 'webpreferences' attribute string, if set
  // this uses the same parsing rules as window.open uses for its features
  const parsedWebPreferences =
    typeof params.webpreferences === 'string'
      ? parseWebViewWebPreferences(params.webpreferences)
      : null;

  const webPreferences: Electron.WebPreferences = {
    nodeIntegration: params.nodeintegration ?? false,
    nodeIntegrationInSubFrames: params.nodeintegrationinsubframes ?? false,
    plugins: params.plugins,
    zoomFactor: embedder.zoomFactor,
    disablePopups: !params.allowpopups,
    webSecurity: !params.disablewebsecurity,
    enableBlinkFeatures: params.blinkfeatures,
    disableBlinkFeatures: params.disableblinkfeatures,
    partition: params.partition,
    ...parsedWebPreferences
  };

  if (params.preload) {
    webPreferences.preload = netBinding.fileURLToFilePath(params.preload);
  }

  // Security options that guest will always inherit from embedder
  const inheritedWebPreferences = new Map([
    ['contextIsolation', true],
    ['javascript', false],
    ['nodeIntegration', false],
    ['sandbox', true],
    ['nodeIntegrationInSubFrames', false],
    ['enableWebSQL', false]
  ]);

  // Inherit certain option values from embedder
  const lastWebPreferences = embedder.getLastWebPreferences()!;
  for (const [name, value] of inheritedWebPreferences) {
    if (lastWebPreferences[name as keyof Electron.WebPreferences] === value) {
      (webPreferences as any)[name] = value;
    }
  }

  return webPreferences;
}

function makeLoadURLOptions (params: Record<string, any>) {
  const opts: Electron.LoadURLOptions = {};
  if (params.httpreferrer) {
    opts.httpReferrer = params.httpreferrer;
  }
  if (params.useragent) {
    opts.userAgent = params.useragent;
  }
  return opts;
}

// Create a new guest instance.
const createGuest = function (embedder: Electron.WebContents, embedderFrameId: number, elementInstanceId: number, params: Record<string, any>) {
  const webPreferences = makeWebPreferences(embedder, params);
  const event = {
    sender: embedder,
    preventDefault () {
      this.defaultPrevented = true;
    },
    defaultPrevented: false
  };

  const { instanceId } = params;

  embedder.emit('will-attach-webview', event, webPreferences, params);
  if (event.defaultPrevented) {
    return -1;
  }

  const guest = (webContents as typeof ElectronInternal.WebContents).create({
    ...webPreferences,
    type: 'webview',
    embedder
  });

  const guestInstanceId = guest.id;
  guestInstances.set(guestInstanceId, {
    elementInstanceId,
    guest,
    embedder
  });

  // Clear the guest from map when it is destroyed.
  guest.once('destroyed', () => {
    if (guestInstances.has(guestInstanceId)) {
      detachGuest(embedder, guestInstanceId);
    }
  });

  // Init guest web view after attached.
  guest.once('did-attach' as any, function (this: Electron.WebContents, event: Electron.Event) {
    const previouslyAttached = this.viewInstanceId != null;
    this.viewInstanceId = instanceId;

    // Only load URL and set size on first attach
    if (previouslyAttached) {
      return;
    }

    if (params.src) {
      this.loadURL(params.src, makeLoadURLOptions(params));
    }
    embedder.emit('did-attach-webview', event, guest);
  });

  const sendToEmbedder = (channel: string, ...args: any[]) => {
    if (!embedder.isDestroyed()) {
      embedder._sendInternal(`${channel}-${guest.viewInstanceId}`, ...args);
    }
  };

  const makeProps = (eventKey: string, args: any[]) => {
    const props: Record<string, any> = {};
    for (const [index, prop] of webViewEvents[eventKey].entries()) {
      props[prop] = args[index];
    }
    return props;
  };

  // Dispatch events to embedder.
  for (const event of supportedWebViewEvents) {
    guest.on(event as any, function (_, ...args: any[]) {
      sendToEmbedder(IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT, event, makeProps(event, args));
    });
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('-ipc-message-host' as any, function (event: Electron.IpcMainEvent, channel: string, args: any[]) {
    sendToEmbedder(IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT, 'ipc-message', {
      frameId: [event.processId, event.frameId],
      channel,
      args
    });
  });

  // Dispatch guest's frame navigation event to embedder.
  guest.on('will-frame-navigate', function (event: Electron.WebContentsWillFrameNavigateEventParams) {
    sendToEmbedder(IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT, 'will-frame-navigate', {
      url: event.url,
      isMainFrame: event.isMainFrame,
      frameProcessId: event.processId,
      frameRoutingId: event.routingId
    });
  });

  // Notify guest of embedder window visibility when it is ready
  // FIXME Remove once https://github.com/electron/electron/issues/6828 is fixed
  guest.on('dom-ready', function () {
    const guestInstance = guestInstances.get(guestInstanceId);
    if (guestInstance != null && guestInstance.visibilityState != null) {
      guest._sendInternal(IPC_MESSAGES.GUEST_INSTANCE_VISIBILITY_CHANGE, guestInstance.visibilityState);
    }
  });

  // Destroy the old guest when attaching.
  const key = `${embedder.id}-${elementInstanceId}`;
  const oldGuestInstanceId = embedderElementsMap.get(key);
  if (oldGuestInstanceId != null) {
    const oldGuestInstance = guestInstances.get(oldGuestInstanceId);
    if (oldGuestInstance) {
      oldGuestInstance.guest.detachFromOuterFrame();
    }
  }

  embedderElementsMap.set(key, guestInstanceId);
  guest.setEmbedder(embedder);

  watchEmbedder(embedder);

  webViewManager.addGuest(guestInstanceId, embedder, guest, webPreferences);
  guest.attachToIframe(embedder, embedderFrameId);

  return guestInstanceId;
};

// Remove an guest-embedder relationship.
const detachGuest = function (embedder: Electron.WebContents, guestInstanceId: number) {
  const guestInstance = guestInstances.get(guestInstanceId);

  if (!guestInstance) return;

  if (embedder !== guestInstance.embedder) {
    return;
  }

  webViewManager.removeGuest(embedder, guestInstanceId);
  guestInstances.delete(guestInstanceId);

  const key = `${embedder.id}-${guestInstance.elementInstanceId}`;
  embedderElementsMap.delete(key);
};

// Once an embedder has had a guest attached we watch it for destruction to
// destroy any remaining guests.
const watchedEmbedders = new Set<Electron.WebContents>();
const watchEmbedder = function (embedder: Electron.WebContents) {
  if (watchedEmbedders.has(embedder)) {
    return;
  }
  watchedEmbedders.add(embedder);

  // Forward embedder window visibility change events to guest
  const onVisibilityChange = function (visibilityState: DocumentVisibilityState) {
    for (const guestInstance of guestInstances.values()) {
      guestInstance.visibilityState = visibilityState;
      if (guestInstance.embedder === embedder) {
        guestInstance.guest._sendInternal(IPC_MESSAGES.GUEST_INSTANCE_VISIBILITY_CHANGE, visibilityState);
      }
    }
  };
  embedder.on('-window-visibility-change', onVisibilityChange);

  embedder.once('will-destroy' as any, () => {
    // Usually the guestInstances is cleared when guest is destroyed, but it
    // may happen that the embedder gets manually destroyed earlier than guest,
    // and the embedder will be invalid in the usual code path.
    for (const [guestInstanceId, guestInstance] of guestInstances) {
      if (guestInstance.embedder === embedder) {
        detachGuest(embedder, guestInstanceId);
      }
    }
    // Clear the listeners.
    embedder.removeListener('-window-visibility-change', onVisibilityChange);
    watchedEmbedders.delete(embedder);
  });
};

const isWebViewTagEnabledCache = new WeakMap();

const isWebViewTagEnabled = function (contents: Electron.WebContents) {
  if (!isWebViewTagEnabledCache.has(contents)) {
    const webPreferences = contents.getLastWebPreferences() || {};
    isWebViewTagEnabledCache.set(contents, !!webPreferences.webviewTag);
  }

  return isWebViewTagEnabledCache.get(contents);
};

const makeSafeHandler = function<Event extends { sender: Electron.WebContents }> (channel: string, handler: (event: Event, ...args: any[]) => any) {
  return (event: Electron.IpcMainInvokeEvent | Electron.IpcMainServiceWorkerInvokeEvent, ...args: any[]) => {
    if (event.type !== 'frame') return;
    if (isWebViewTagEnabled(event.sender)) {
      return handler(event as unknown as Event, ...args);
    } else {
      console.error(`<webview> IPC message ${channel} sent by WebContents with <webview> disabled (${event.sender.id})`);
      throw new Error('<webview> disabled');
    }
  };
};

const handleMessage = function (channel: string, handler: (event: Electron.IpcMainInvokeEvent, ...args: any[]) => any) {
  ipcMainInternal.handle(channel, makeSafeHandler(channel, handler));
};

const handleMessageSync = function (channel: string, handler: (event: { sender: Electron.WebContents }, ...args: any[]) => any) {
  ipcMainUtils.handleSync(channel, makeSafeHandler(channel, handler));
};

handleMessage(IPC_MESSAGES.GUEST_VIEW_MANAGER_CREATE_AND_ATTACH_GUEST, function (event, embedderFrameId: number, elementInstanceId: number, params) {
  return createGuest(event.sender, embedderFrameId, elementInstanceId, params);
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_DETACH_GUEST, function (event, guestInstanceId: number) {
  return detachGuest(event.sender, guestInstanceId);
});

// this message is sent by the actual <webview>
ipcMainInternal.on(IPC_MESSAGES.GUEST_VIEW_MANAGER_FOCUS_CHANGE, function (event, focus: boolean) {
  if (event.type === 'frame') {
    event.sender.emit('-focus-change', {}, focus);
  }
});

handleMessage(IPC_MESSAGES.GUEST_VIEW_MANAGER_CALL, function (event, guestInstanceId: number, method: string, args: any[]) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!asyncMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return (guest as any)[method](...args);
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_CALL, function (event, guestInstanceId: number, method: string, args: any[]) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!syncMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }
  // Redirect history methods to updated navigationHistory property on webContents. See issue #42879.
  if (navigationHistorySyncMethods.has(method)) {
    let navigationMethod = method;
    if (method === 'clearHistory') {
      navigationMethod = 'clear';
    }
    return (guest as any).navigationHistory[navigationMethod](...args);
  }

  return (guest as any)[method](...args);
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_PROPERTY_GET, function (event, guestInstanceId: number, property: string) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  return (guest as any)[property];
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_PROPERTY_SET, function (event, guestInstanceId: number, property: string, val: any) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  (guest as any)[property] = val;
});

// Returns WebContents from its guest id hosted in given webContents.
const getGuestForWebContents = function (guestInstanceId: number, contents: Electron.WebContents) {
  const guestInstance = guestInstances.get(guestInstanceId);
  if (!guestInstance) {
    throw new Error(`Invalid guestInstanceId: ${guestInstanceId}`);
  }
  if (guestInstance.guest.hostWebContents !== contents) {
    throw new Error(`Access denied to guestInstanceId: ${guestInstanceId}`);
  }
  return guestInstance.guest;
};
