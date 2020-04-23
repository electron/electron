import {
  PostData,
  webContents,
  BrowserWindow,
  WebContentsInternal,
  Referrer,
  LoadURLOptions,
  BrowserWindowConstructorOptionsInternal,
  BrowserWindowConstructorOptions,
  WebContents
} from 'electron';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import { parseFeatures } from '@electron/internal/common/parse-features-string';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';

const { isSameOrigin } = process.electronBinding('v8_util');

const hasProp = {}.hasOwnProperty;
const frameToGuest = new Map();

// Security options that child windows will always inherit from parent windows
const inheritedWebPreferences = new Map([
  ['contextIsolation', true],
  ['javascript', false],
  ['nativeWindowOpen', true],
  ['nodeIntegration', false],
  ['enableRemoteModule', false],
  ['sandbox', true],
  ['webviewTag', false],
  ['nodeIntegrationInSubFrames', false]
]);

// Copy attribute of |parent| to |child| if it is not defined in |child|.
const mergeOptions = function (child: any, parent: any, visited?: Set<any>) {
  // Check for circular reference.
  if (visited == null) visited = new Set();
  if (visited.has(parent)) return;

  visited.add(parent);
  for (const key in parent) {
    if (key === 'type') continue;
    if (!hasProp.call(parent, key)) continue;
    if (key in child && key !== 'webPreferences') continue;

    const value = parent[key];
    if (typeof value === 'object' && !Array.isArray(value)) {
      child[key] = mergeOptions(child[key] || {}, value, visited);
    } else {
      child[key] = value;
    }
  }
  visited.delete(parent);

  return child;
};

// Merge |options| with the |embedder|'s window's options.
const mergeBrowserWindowOptions = function (embedder: any, options: any) {
  if (options.webPreferences == null) {
    options.webPreferences = {};
  }
  if (embedder.browserWindowOptions != null) {
    let parentOptions = embedder.browserWindowOptions;

    // if parent's visibility is available, that overrides 'show' flag (#12125)
    const win = BrowserWindow.fromWebContents(embedder.webContents);
    if (win != null) {
      parentOptions = {
        ...win.getBounds(),
        ...embedder.browserWindowOptions,
        show: win.isVisible()
      };
    }

    // Inherit the original options if it is a BrowserWindow.
    mergeOptions(options, parentOptions);
  } else {
    // Or only inherit webPreferences if it is a webview.
    mergeOptions(options.webPreferences, embedder.getLastWebPreferences());
  }

  // Inherit certain option values from parent window
  const webPreferences = embedder.getLastWebPreferences();
  for (const [name, value] of inheritedWebPreferences) {
    if (webPreferences[name] === value) {
      options.webPreferences[name] = value;
    }
  }

  if (!webPreferences.nativeWindowOpen) {
    // Sets correct openerId here to give correct options to 'new-window' event handler
    options.webPreferences.openerId = embedder.id;
  }

  return options;
};

const MULTIPART_CONTENT_TYPE = 'multipart/form-data';
const URL_ENCODED_CONTENT_TYPE = 'application/x-www-form-urlencoded';
function makeContentTypeHeader ({
  contentType,
  boundary
}: {
  contentType: string
  boundary?: string
}) {
  const header = `content-type: ${contentType};`;
  if (contentType === MULTIPART_CONTENT_TYPE) {
    return `${header} boundary=${boundary}`;
  }
  return header;
}

// Figure out appropriate headers for post data.
const parseContentTypeFormat = function (postData: PostData[]) {
  if (postData.length) {
    // For multipart forms, the first element will start with the boundary
    // notice, which looks something like `------WebKitFormBoundary12345678`
    // Note, this regex would fail when submitting a urlencoded form with an
    // input attribute of name="--theKey", but, uhh, don't do that?
    const postDataFront = postData[0].bytes!.toString();
    const boundary = /^--.*[^-\r\n]/.exec(postDataFront);
    if (boundary) {
      return {
        boundary: boundary[0].substr(2),
        contentType: MULTIPART_CONTENT_TYPE
      };
    }
  }
  // Either the form submission didn't contain any inputs (the postData array
  // was empty), or we couldn't find the boundary and thus we can assume this is
  // a key=value style form.
  return {
    contentType: URL_ENCODED_CONTENT_TYPE
  };
};

// Setup a new guest with |embedder|
const setupGuest = function (
  embedder: WebContentsInternal,
  frameName: string,
  guest: BrowserWindow
) {
  // When |embedder| is destroyed we should also destroy attached guest, and if
  // guest is closed by user then we should prevent |embedder| from double
  // closing guest.
  const guestId = guest.webContents.id;
  const closedByEmbedder = function () {
    guest.removeListener('closed', closedByUser);
    guest.destroy();
  };
  const closedByUser = function () {
    embedder._sendInternal(
      'ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_' + guestId
    );
    embedder.removeListener('current-render-view-deleted' as any, closedByEmbedder);
  };
  embedder.once('current-render-view-deleted' as any, closedByEmbedder);
  guest.once('closed', closedByUser);
  if (frameName) {
    frameToGuest.set(frameName, guest);
    (guest as any).frameName = frameName;
    guest.once('closed', function () {
      frameToGuest.delete(frameName);
    });
  }
  return guestId;
};

// Create a new guest created by |embedder| with |options|.
const createGuest = function (
  embedder: WebContentsInternal,
  url: string,
  referrer: Referrer,
  frameName: string,
  options: BrowserWindowConstructorOptionsInternal,
  postData: PostData[] | null
) {
  let guest = frameToGuest.get(frameName);
  if (frameName && guest != null) {
    guest.loadURL(url);
    return guest.webContents.id;
  }

  // Remember the embedder window's id.
  if (options.webPreferences == null) {
    options.webPreferences = {};
  }

  guest = new BrowserWindow(options);
  if (!options.webContents) {
    // We should not call `loadURL` if the window was constructed from an
    // existing webContents (window.open in a sandboxed renderer).
    //
    // Navigating to the url when creating the window from an existing
    // webContents is not necessary (it will navigate there anyway).
    const loadOptions: LoadURLOptions = {
      httpReferrer: referrer
    };
    if (postData != null) {
      loadOptions.postData = postData as any;
      loadOptions.extraHeaders = makeContentTypeHeader(
        parseContentTypeFormat(postData)
      );
    }
    guest.loadURL(url, loadOptions);
  }

  return setupGuest(embedder, frameName, guest);
};

const getGuestWindow = function (guestContents: WebContents) {
  let guestWindow = BrowserWindow.fromWebContents(guestContents);
  if (guestWindow == null) {
    const hostContents = guestContents.hostWebContents;
    if (hostContents != null) {
      guestWindow = BrowserWindow.fromWebContents(hostContents);
    }
  }
  if (!guestWindow) {
    throw new Error('getGuestWindow failed');
  }
  return guestWindow;
};

const isChildWindow = function (sender: WebContents, target: WebContents) {
  return (target as any).getLastWebPreferences().openerId === sender.id;
};

const isRelatedWindow = function (sender: WebContents, target: WebContents) {
  return isChildWindow(sender, target) || isChildWindow(target, sender);
};

const isScriptableWindow = function (sender: WebContents, target: WebContents) {
  return (
    isRelatedWindow(sender, target) &&
    isSameOrigin(sender.getURL(), target.getURL())
  );
};

const isNodeIntegrationEnabled = function (sender: WebContents) {
  return (sender as any).getLastWebPreferences().nodeIntegration === true;
};

// Checks whether |sender| can access the |target|:
const canAccessWindow = function (sender: WebContents, target: WebContents) {
  return (
    isChildWindow(sender, target) ||
    isScriptableWindow(sender, target) ||
    isNodeIntegrationEnabled(sender)
  );
};

// Routed window.open messages with raw options
ipcMainInternal.on(
  'ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN',
  (
    event: ElectronInternal.IpcMainInternalEvent,
    url: string,
    frameName: string,
    features: string
  ) => {
    // This should only be allowed for senders that have nativeWindowOpen: false
    const lastWebPreferences = (event.sender as any).getLastWebPreferences();
    if (lastWebPreferences.nativeWindowOpen || lastWebPreferences.sandbox) {
      event.returnValue = null;
      throw new Error(
        'GUEST_WINDOW_MANAGER_WINDOW_OPEN denied: expected native window.open'
      );
    }
    if (url == null || url === '') url = 'about:blank';
    if (frameName == null) frameName = '';
    if (features == null) features = '';

    const disposition = 'new-window';
    const { options, webPreferences, additionalFeatures } = parseFeatures(
      features
    );
    if (!options.title) options.title = frameName;
    (options as BrowserWindowConstructorOptions).webPreferences = webPreferences;

    const referrer: Referrer = { url: '', policy: 'default' };
    internalWindowOpen(
      event,
      url,
      referrer,
      frameName,
      disposition,
      options,
      additionalFeatures,
      null
    );
  }
);

// Routed window.open messages with fully parsed options
function internalWindowOpen (
  event: ElectronInternal.IpcMainInternalEvent,
  url: string,
  referrer: Referrer,
  frameName: string,
  disposition: string,
  options: BrowserWindowConstructorOptionsInternal,
  additionalFeatures: string[],
  postData: PostData[] | null
) {
  options = mergeBrowserWindowOptions(event.sender, options);
  const postBody = postData
    ? {
      data: postData,
      ...parseContentTypeFormat(postData)
    }
    : null;

  event.sender.emit(
    'new-window',
    event,
    url,
    frameName,
    disposition,
    options,
    additionalFeatures,
    referrer,
    postBody
  );
  const { newGuest } = event as any;
  if (
    (event.sender.getType() === 'webview' &&
      (event.sender as any).getLastWebPreferences().disablePopups) ||
    event.defaultPrevented
  ) {
    if (newGuest != null) {
      if (options.webContents === newGuest.webContents) {
        // the webContents is not changed, so set defaultPrevented to false to
        // stop the callers of this event from destroying the webContents.
        (event as any).defaultPrevented = false;
      }
      event.returnValue = setupGuest(event.sender as WebContentsInternal, frameName, newGuest);
    } else {
      event.returnValue = null;
    }
  } else {
    event.returnValue = createGuest(
      event.sender as WebContentsInternal,
      url,
      referrer,
      frameName,
      options,
      postData
    );
  }
}

type IpcHandler = (event: Electron.IpcMainInvokeEvent, guestContents: WebContentsInternal, ...args: any[]) => void;
const makeSafeHandler = function (handler: IpcHandler) {
  return (event: Electron.IpcMainInvokeEvent, guestId: number, ...args: any[]) => {
    // Access webContents via electron to prevent circular require.
    const guestContents = webContents.fromId(guestId);
    if (!guestContents) {
      throw new Error(`Invalid guestId: ${guestId}`);
    }

    return handler(event, guestContents as WebContentsInternal, ...args);
  };
};

const handleMessage = function (channel: string, handler: IpcHandler) {
  ipcMainInternal.handle(channel, makeSafeHandler(handler));
};

const handleMessageSync = function (channel: string, handler: IpcHandler) {
  ipcMainUtils.handleSync(channel, makeSafeHandler(handler));
};

type ContentsCheck = (contents: WebContents, guestContents: WebContents) => boolean;
const securityCheck = function (contents: WebContents, guestContents: WebContents, check: ContentsCheck) {
  if (!check(contents, guestContents)) {
    console.error(
      `Blocked ${contents.getURL()} from accessing guestId: ${guestContents.id}`
    );
    throw new Error(`Access denied to guestId: ${guestContents.id}`);
  }
};

const windowMethods = new Set(['destroy', 'focus', 'blur']);

handleMessage(
  'ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_METHOD',
  (event, guestContents, method, ...args) => {
    securityCheck(event.sender, guestContents, canAccessWindow);

    if (!windowMethods.has(method)) {
      console.error(
        `Blocked ${event.sender.getURL()} from calling method: ${method}`
      );
      throw new Error(`Invalid method: ${method}`);
    }

    return (getGuestWindow(guestContents) as any)[method](...args);
  }
);

handleMessage(
  'ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE',
  (event, guestContents, message, targetOrigin, sourceOrigin) => {
    if (targetOrigin == null) {
      targetOrigin = '*';
    }

    // The W3C does not seem to have word on how postMessage should work when the
    // origins do not match, so we do not do |canAccessWindow| check here since
    // postMessage across origins is useful and not harmful.
    securityCheck(event.sender, guestContents, isRelatedWindow);

    if (
      targetOrigin === '*' ||
      isSameOrigin(guestContents.getURL(), targetOrigin)
    ) {
      const sourceId = event.sender.id;
      guestContents._sendInternal(
        'ELECTRON_GUEST_WINDOW_POSTMESSAGE',
        sourceId,
        message,
        sourceOrigin
      );
    }
  }
);

const webContentsMethodsAsync = new Set([
  'loadURL',
  'executeJavaScript',
  'print'
]);

handleMessage(
  'ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD',
  (event, guestContents, method, ...args) => {
    securityCheck(event.sender, guestContents, canAccessWindow);

    if (!webContentsMethodsAsync.has(method)) {
      console.error(
        `Blocked ${event.sender.getURL()} from calling method: ${method}`
      );
      throw new Error(`Invalid method: ${method}`);
    }

    return (guestContents as any)[method](...args);
  }
);

const webContentsMethodsSync = new Set(['getURL']);

handleMessageSync(
  'ELECTRON_GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD',
  (event, guestContents, method, ...args) => {
    securityCheck(event.sender, guestContents, canAccessWindow);

    if (!webContentsMethodsSync.has(method)) {
      console.error(
        `Blocked ${event.sender.getURL()} from calling method: ${method}`
      );
      throw new Error(`Invalid method: ${method}`);
    }

    return (guestContents as any)[method](...args);
  }
);

exports.internalWindowOpen = internalWindowOpen;
