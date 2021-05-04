import { app } from 'electron/main';
import type { WebContents } from 'electron/main';
import { clipboard, nativeImage } from 'electron/common';
import * as fs from 'fs';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import * as typeUtils from '@electron/internal/common/type-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

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
ipcMainInternal.on(IPC_MESSAGES.BROWSER_WINDOW_CLOSE, function (event) {
  const window = event.sender.getOwnerBrowserWindow();
  if (window) {
    window.close();
  }
  event.returnValue = null;
});

ipcMainInternal.handle(IPC_MESSAGES.BROWSER_GET_LAST_WEB_PREFERENCES, function (event) {
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

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_CLIPBOARD_SYNC, function (event, method: string, ...args: any[]) {
  if (!allowedClipboardMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return typeUtils.serialize((clipboard as any)[method](...typeUtils.deserialize(args)));
});

if (BUILDFLAG(ENABLE_DESKTOP_CAPTURER)) {
  const desktopCapturer = require('@electron/internal/browser/desktop-capturer');

  ipcMainInternal.handle(IPC_MESSAGES.DESKTOP_CAPTURER_GET_SOURCES, async function (event, options: Electron.SourcesOptions, stack: string) {
    logStack(event.sender, 'desktopCapturer.getSources()', stack);
    const customEvent = emitCustomEvent(event.sender, 'desktop-capturer-get-sources');

    if (customEvent.defaultPrevented) {
      console.error('Blocked desktopCapturer.getSources()');
      return [];
    }

    return typeUtils.serialize(await desktopCapturer.getSourcesImpl(event, options));
  });
}

const getPreloadScript = async function (preloadPath: string) {
  let preloadSrc = null;
  let preloadError = null;
  try {
    preloadSrc = await fs.promises.readFile(preloadPath, 'utf8');
  } catch (error) {
    preloadError = error;
  }
  return { preloadPath, preloadSrc, preloadError };
};

ipcMainUtils.handleSync(IPC_MESSAGES.BROWSER_SANDBOX_LOAD, async function (event) {
  const preloadPaths = event.sender._getPreloadPaths();

  return {
    preloadScripts: await Promise.all(preloadPaths.map(path => getPreloadScript(path))),
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

ipcMainInternal.on(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, function (event, preloadPath: string, error: Error) {
  event.sender.emit('preload-error', event, preloadPath, error);
});

ipcMainInternal.handle(IPC_MESSAGES.NATIVE_IMAGE_CREATE_THUMBNAIL_FROM_PATH, async (_, path: string, size: Electron.Size) => {
  return typeUtils.serialize(await nativeImage.createThumbnailFromPath(path, size));
});
