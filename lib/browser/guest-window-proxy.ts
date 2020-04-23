import {
  webContents,
  BrowserWindow,
  WebContentsInternal,
  Referrer,
  BrowserWindowConstructorOptions,
  WebContents
} from 'electron';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import { parseFeatures } from '@electron/internal/common/parse-features-string';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { internalWindowOpen } from './guest-window-manager';

const { isSameOrigin } = process.electronBinding('v8_util');

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
