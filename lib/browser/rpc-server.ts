import { app } from 'electron/main';
import type { IpcMainInvokeEvent, WebContents } from 'electron/main';
import { clipboard, crashReporter } from 'electron/common';
import * as fs from 'fs';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import * as guestViewManager from '@electron/internal/browser/guest-view-manager';
import * as typeUtils from '@electron/internal/common/type-utils';

const eventBinding = process._linkedBinding('electron_browser_event');

const emitCustomEvent = function (contents: WebContents, eventName: string, ...args: any[]) {
  const event = eventBinding.createWithSender(contents);

  app.emit(eventName, event, contents, ...args);
  contents.emit(eventName, event, ...args);

  return event;
};

const logStack = function (contents: WebContents, code: string, stack: string) {
  if (stack) {
    console.warn(`WebContents (${contents.id}): ${code}`, stack);
  }
};

// Implements window.close()
ipcMainInternal.on('ELECTRON_BROWSER_WINDOW_CLOSE', function (event: ElectronInternal.IpcMainInternalEvent) {
  const window = event.sender.getOwnerBrowserWindow();
  if (window) {
    window.close();
  }
  event.returnValue = null;
});

ipcMainInternal.handle('ELECTRON_BROWSER_GET_LAST_WEB_PREFERENCES', function (event: IpcMainInvokeEvent) {
  return event.sender.getLastWebPreferences();
});

// Methods not listed in this set are called directly in the renderer process.
const allowedClipboardMethods = (() => {
  switch (process.platform) {
    case 'darwin':
      return new Set(['readFindText', 'writeFindText']);
    case 'linux':
      return new Set(Object.keys(clipboard));
    default:
      return new Set();
  }
})();

ipcMainUtils.handleSync('ELECTRON_BROWSER_CLIPBOARD_SYNC', function (event: IpcMainInvokeEvent, method: string, ...args: any[]) {
  if (!allowedClipboardMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return typeUtils.serialize((clipboard as any)[method](...typeUtils.deserialize(args)));
});

if (BUILDFLAG(ENABLE_DESKTOP_CAPTURER)) {
  const desktopCapturer = require('@electron/internal/browser/desktop-capturer');

  ipcMainInternal.handle('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', async function (event: IpcMainInvokeEvent, options: Electron.SourcesOptions, stack: string) {
    logStack(event.sender, 'desktopCapturer.getSources()', stack);
    const customEvent = emitCustomEvent(event.sender, 'desktop-capturer-get-sources');

    if (customEvent.defaultPrevented) {
      console.error('Blocked desktopCapturer.getSources()');
      return [];
    }

    return typeUtils.serialize(await desktopCapturer.getSourcesImpl(event, options));
  });

  ipcMainInternal.handle('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_MEDIA_SOURCE_ID_FOR_WEB_CONTENTS', function (event: IpcMainInvokeEvent, webContentsId: number) {
    return desktopCapturer.getMediaSourceIdForWebContents(event, webContentsId);
  });
}

const isRemoteModuleEnabled = BUILDFLAG(ENABLE_REMOTE_MODULE)
  ? require('@electron/internal/browser/remote/server').isRemoteModuleEnabled
  : () => false;

const getPreloadScript = async function (preloadPath: string) {
  let preloadSrc = null;
  let preloadError = null;
  try {
    preloadSrc = (await fs.promises.readFile(preloadPath)).toString();
  } catch (error) {
    preloadError = error;
  }
  return { preloadPath, preloadSrc, preloadError };
};

ipcMainUtils.handleSync('ELECTRON_BROWSER_SANDBOX_LOAD', async function (event: IpcMainInvokeEvent) {
  const preloadPaths = event.sender._getPreloadPaths();
  const webPreferences = event.sender.getLastWebPreferences() || {};

  return {
    preloadScripts: await Promise.all(preloadPaths.map(path => getPreloadScript(path))),
    isRemoteModuleEnabled: isRemoteModuleEnabled(event.sender),
    isWebViewTagEnabled: guestViewManager.isWebViewTagEnabled(event.sender),
    guestInstanceId: webPreferences.guestInstanceId,
    openerId: webPreferences.openerId,
    process: {
      arch: process.arch,
      platform: process.platform,
      env: { ...process.env },
      version: process.version,
      versions: process.versions,
      execPath: process.helperExecPath
    }
  };
});

ipcMainInternal.on('ELECTRON_BROWSER_PRELOAD_ERROR', function (event: ElectronInternal.IpcMainInternalEvent, preloadPath: string, error: Error) {
  event.sender.emit('preload-error', event, preloadPath, error);
});

ipcMainUtils.handleSync('ELECTRON_CRASH_REPORTER_GET_LAST_CRASH_REPORT', () => {
  return crashReporter.getLastCrashReport();
});

ipcMainUtils.handleSync('ELECTRON_CRASH_REPORTER_GET_UPLOADED_REPORTS', () => {
  return crashReporter.getUploadedReports();
});

ipcMainUtils.handleSync('ELECTRON_CRASH_REPORTER_GET_UPLOAD_TO_SERVER', () => {
  return crashReporter.getUploadToServer();
});

ipcMainUtils.handleSync('ELECTRON_CRASH_REPORTER_SET_UPLOAD_TO_SERVER', (event: IpcMainInvokeEvent, uploadToServer: boolean) => {
  return crashReporter.setUploadToServer(uploadToServer);
});

ipcMainUtils.handleSync('ELECTRON_CRASH_REPORTER_GET_CRASHES_DIRECTORY', () => {
  return crashReporter.getCrashesDirectory();
});
