/**
 * Manage guest windows when using the default BrowserWindowProxy version of the
 * renderer's window.open (i.e. nativeWindowOpen off). This module mostly
 * consists of marshaling IPC requests from the BrowserWindowProxy to the
 * WebContents.
 */
import { webContents, BrowserWindow } from 'electron/main';
import type { WebContents } from 'electron/main';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { openGuestWindow } from '@electron/internal/browser/guest-window-manager';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const { isSameOrigin } = process._linkedBinding('electron_common_v8_util');

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
  return target.getLastWebPreferences()!.openerId === sender.id;
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
  return sender.getLastWebPreferences()!.nodeIntegration === true;
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
  IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_OPEN,
  (
    event,
    url: string,
    frameName: string,
    features: string
  ) => {
    // This should only be allowed for senders that have nativeWindowOpen: false
    const lastWebPreferences = event.sender.getLastWebPreferences()!;
    if (lastWebPreferences.nativeWindowOpen || lastWebPreferences.sandbox) {
      event.returnValue = null;
      throw new Error(
        'GUEST_WINDOW_MANAGER_WINDOW_OPEN denied: expected native window.open'
      );
    }

    const referrer: Electron.Referrer = { url: '', policy: 'strict-origin-when-cross-origin' };
    const browserWindowOptions = event.sender._callWindowOpenHandler(event, { url, frameName, features, disposition: 'new-window', referrer });
    if (event.defaultPrevented) {
      event.returnValue = null;
      return;
    }
    const guest = openGuestWindow({
      event,
      embedder: event.sender,
      referrer,
      disposition: 'new-window',
      overrideBrowserWindowOptions: browserWindowOptions!,
      windowOpenArgs: {
        url: url || 'about:blank',
        frameName: frameName || '',
        features: features || ''
      }
    });

    event.returnValue = guest ? guest.webContents.id : null;
  }
);

type IpcHandler<T, Event> = (event: Event, guestContents: Electron.WebContents, ...args: any[]) => T;
const makeSafeHandler = function<T, Event> (handler: IpcHandler<T, Event>) {
  return (event: Event, guestId: number, ...args: any[]) => {
    // Access webContents via electron to prevent circular require.
    const guestContents = webContents.fromId(guestId);
    if (!guestContents) {
      throw new Error(`Invalid guestId: ${guestId}`);
    }

    return handler(event, guestContents as Electron.WebContents, ...args);
  };
};

const handleMessage = function (channel: string, handler: IpcHandler<any, Electron.IpcMainInvokeEvent>) {
  ipcMainInternal.handle(channel, makeSafeHandler(handler));
};

const handleMessageSync = function (channel: string, handler: IpcHandler<any, ElectronInternal.IpcMainInternalEvent>) {
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
  IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_METHOD,
  (event, guestContents, method, ...args) => {
    securityCheck(event.sender, guestContents, canAccessWindow);

    if (!windowMethods.has(method)) {
      console.error(
        `Blocked ${event.senderFrame.url} from calling method: ${method}`
      );
      throw new Error(`Invalid method: ${method}`);
    }

    return (getGuestWindow(guestContents) as any)[method](...args);
  }
);

handleMessage(
  IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE,
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
        IPC_MESSAGES.GUEST_WINDOW_POSTMESSAGE,
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
  IPC_MESSAGES.GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD,
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
  IPC_MESSAGES.GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD,
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
