'use strict';

const { webContents } = require('electron');
const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal');
const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils');
const parseFeaturesString = require('@electron/internal/common/parse-features-string');
const { syncMethods, asyncMethods, properties } = require('@electron/internal/common/web-view-methods');
const { serialize } = require('@electron/internal/common/type-utils');

// Doesn't exist in early initialization.
let webViewManager = null;

const supportedWebViewEvents = [
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
  'new-window',
  'will-navigate',
  'did-start-navigation',
  'did-navigate',
  'did-frame-navigate',
  'did-navigate-in-page',
  'focus-change',
  'close',
  'crashed',
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

const guestInstances = {};
const embedderElementsMap = {};

// Create a new guest instance.
const createGuest = function (embedder, params) {
  if (webViewManager == null) {
    webViewManager = process.electronBinding('web_view_manager');
  }

  const guest = webContents.create({
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
  guest.once('did-attach', function (event) {
    params = this.attachParams;
    delete this.attachParams;

    const previouslyAttached = this.viewInstanceId != null;
    this.viewInstanceId = params.instanceId;

    // Only load URL and set size on first attach
    if (previouslyAttached) {
      return;
    }

    if (params.src) {
      const opts = {};
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

  const sendToEmbedder = (channel, ...args) => {
    if (!embedder.isDestroyed()) {
      embedder._sendInternal(`${channel}-${guest.viewInstanceId}`, ...args);
    }
  };

  // Dispatch events to embedder.
  const fn = function (event) {
    guest.on(event, function (_, ...args) {
      sendToEmbedder('ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT', event, ...args);
    });
  };
  for (const event of supportedWebViewEvents) {
    fn(event);
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_, channel, args) {
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

  // Forward internal web contents event to embedder to handle
  // native window.open setup
  guest.on('-add-new-contents', (...args) => {
    if (guest.getLastWebPreferences().nativeWindowOpen === true) {
      const embedder = getEmbedder(guestInstanceId);
      if (embedder != null) {
        embedder.emit('-add-new-contents', ...args);
      }
    }
  });

  return guestInstanceId;
};

// Attach the guest to an element of embedder.
const attachGuest = function (event, embedderFrameId, elementInstanceId, guestInstanceId, params) {
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
      webViewManager.removeGuest(guestInstance.embedder, guestInstanceId);
      guestInstance.embedder._sendInternal(`ELECTRON_GUEST_VIEW_INTERNAL_DESTROY_GUEST-${guest.viewInstanceId}`);
    }
  }

  const webPreferences = {
    guestInstanceId: guestInstanceId,
    nodeIntegration: params.nodeintegration != null ? params.nodeintegration : false,
    nodeIntegrationInSubFrames: params.nodeintegrationinsubframes != null ? params.nodeintegrationinsubframes : false,
    enableRemoteModule: params.enableremotemodule,
    plugins: params.plugins,
    zoomFactor: embedder.zoomFactor,
    disablePopups: !params.allowpopups,
    webSecurity: !params.disablewebsecurity,
    enableBlinkFeatures: params.blinkfeatures,
    disableBlinkFeatures: params.disableblinkfeatures
  };

  // parse the 'webpreferences' attribute string, if set
  // this uses the same parsing rules as window.open uses for its features
  if (typeof params.webpreferences === 'string') {
    parseFeaturesString(params.webpreferences, function (key, value) {
      if (value === undefined) {
        // no value was specified, default it to true
        value = true;
      }
      webPreferences[key] = value;
    });
  }

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
  const lastWebPreferences = embedder.getLastWebPreferences();
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

  webViewManager.addGuest(guestInstanceId, elementInstanceId, embedder, guest, webPreferences);
  guest.attachToIframe(embedder, embedderFrameId);
};

// Remove an guest-embedder relationship.
const detachGuest = function (embedder, guestInstanceId) {
  const guestInstance = guestInstances[guestInstanceId];

  if (!guestInstance) return;

  if (embedder !== guestInstance.embedder) {
    return;
  }

  webViewManager.removeGuest(embedder, guestInstanceId);
  delete guestInstances[guestInstanceId];

  const key = `${embedder.id}-${guestInstance.elementInstanceId}`;
  delete embedderElementsMap[key];
};

// Once an embedder has had a guest attached we watch it for destruction to
// destroy any remaining guests.
const watchedEmbedders = new Set();
const watchEmbedder = function (embedder) {
  if (watchedEmbedders.has(embedder)) {
    return;
  }
  watchedEmbedders.add(embedder);

  // Forward embedder window visiblity change events to guest
  const onVisibilityChange = function (visibilityState) {
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

const isWebViewTagEnabled = function (contents) {
  if (!isWebViewTagEnabledCache.has(contents)) {
    const webPreferences = contents.getLastWebPreferences() || {};
    isWebViewTagEnabledCache.set(contents, !!webPreferences.webviewTag);
  }

  return isWebViewTagEnabledCache.get(contents);
};

const handleMessage = function (channel, handler) {
  ipcMainUtils.handle(channel, (event, ...args) => {
    if (isWebViewTagEnabled(event.sender)) {
      return handler(event, ...args);
    } else {
      console.error(`<webview> IPC message ${channel} sent by WebContents with <webview> disabled (${event.sender.id})`);
      throw new Error('<webview> disabled');
    }
  });
};

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_CREATE_GUEST', function (event, params) {
  return createGuest(event.sender, params);
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_ATTACH_GUEST', function (event, embedderFrameId, elementInstanceId, guestInstanceId, params) {
  try {
    attachGuest(event, embedderFrameId, elementInstanceId, guestInstanceId, params);
  } catch (error) {
    console.error(`Guest attach failed: ${error}`);
  }
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_DETACH_GUEST', function (event, guestInstanceId) {
  return detachGuest(event.sender, guestInstanceId);
});

// this message is sent by the actual <webview>
ipcMainInternal.on('ELECTRON_GUEST_VIEW_MANAGER_FOCUS_CHANGE', function (event, focus, guestInstanceId) {
  const guest = getGuest(guestInstanceId);
  if (guest === event.sender) {
    event.sender.emit('focus-change', {}, focus, guestInstanceId);
  } else {
    console.error(`focus-change for guestInstanceId: ${guestInstanceId}`);
  }
});

const allMethods = new Set([ ...syncMethods, ...asyncMethods ]);

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_CALL', function (event, guestInstanceId, method, args) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!allMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return guest[method](...args);
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_PROPERTY_GET', function (event, guestInstanceId, property) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  return guest[property];
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_PROPERTY_SET', function (event, guestInstanceId, property, val) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  guest[property] = val;
});

handleMessage('ELECTRON_GUEST_VIEW_MANAGER_CAPTURE_PAGE', async function (event, guestInstanceId, args) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);

  return serialize(await guest.capturePage(...args));
});

// Returns WebContents from its guest id hosted in given webContents.
const getGuestForWebContents = function (guestInstanceId, contents) {
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
const getGuest = function (guestInstanceId) {
  const guestInstance = guestInstances[guestInstanceId];
  if (guestInstance != null) return guestInstance.guest;
};

// Returns the embedder of the guest.
const getEmbedder = function (guestInstanceId) {
  const guestInstance = guestInstances[guestInstanceId];
  if (guestInstance != null) return guestInstance.embedder;
};

exports.getGuestForWebContents = getGuestForWebContents;
exports.isWebViewTagEnabled = isWebViewTagEnabled;
