import {
  openGuestWindow,
  makeWebPreferences,
  parseContentTypeFormat
} from '@electron/internal/browser/guest-window-manager';
import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';
import { parseFeatures } from '@electron/internal/browser/parse-features-string';
import * as deprecate from '@electron/internal/common/deprecate';

import { app, webFrameMain, dialog } from 'electron/main';
import type { BrowserWindowConstructorOptions, MessageBoxOptions, NavigationEntry } from 'electron/main';

import * as path from 'path';
import * as url from 'url';
// session is not used here, the purpose of the import is to make sure session
// is initialized before the webContents module.
import '@electron/internal/browser/api/session';

// JavaScript implementations of WebContents.
const binding = process._linkedBinding('electron_browser_web_contents');
const { WebContents } = binding as { WebContents: { prototype: Electron.WebContents } };

WebContents.prototype.postMessage = function (...args) {
  return this.mainFrame.postMessage(...args);
};

WebContents.prototype.send = function (channel, ...args) {
  return this.mainFrame.send(channel, ...args);
};

WebContents.prototype._sendInternal = function (channel, ...args) {
  return this.mainFrame._sendInternal(channel, ...args);
};

function getWebFrame(contents: Electron.WebContents, frame: number | [number, number]) {
  let webFrame: Electron.WebFrameMain | undefined;
  if (typeof frame === 'number') {
    webFrame = webFrameMain.fromId(contents.mainFrame.processId, frame);
  } else if (Array.isArray(frame) && frame.length === 2 && frame.every((value) => typeof value === 'number')) {
    webFrame = webFrameMain.fromId(frame[0], frame[1]);
  } else {
    throw new Error('Missing required frame argument (must be number or [processId, frameId])');
  }
  // Frame ids are global; only address frames that belong to |contents|.
  if (webFrame && webFrame.top !== contents.mainFrame) return undefined;
  return webFrame;
}

WebContents.prototype.sendToFrame = function (frameId, channel, ...args) {
  const frame = getWebFrame(this, frameId);
  if (!frame) return false;
  frame.send(channel, ...args);
  return true;
};

const waitTillCanExecuteJavaScript = async (webContents: Electron.WebContents) => {
  if (webContents.getURL() && !webContents.isLoadingMainFrame()) return;

  return new Promise<void>((resolve) => {
    webContents.once('did-stop-loading', () => {
      resolve();
    });
  });
};

// Make sure WebContents::executeJavaScript would run the code only when the
// WebContents has been loaded.
WebContents.prototype.executeJavaScript = async function (code, hasUserGesture) {
  await waitTillCanExecuteJavaScript(this);
  return this._executeJavaScript(0, [{ code: String(code) }], !!hasUserGesture);
};
WebContents.prototype.executeJavaScriptInIsolatedWorld = async function (worldId, code, hasUserGesture) {
  if (!Number.isInteger(worldId)) throw new TypeError('worldId must be an integer');
  await waitTillCanExecuteJavaScript(this);
  return this._executeJavaScript(worldId, code, !!hasUserGesture);
};

WebContents.prototype.loadFile = function (filePath, options = {}) {
  if (typeof filePath !== 'string') {
    throw new TypeError('Must pass filePath as a string');
  }
  const { query, search, hash } = options;

  return this.loadURL(
    url.format({
      protocol: 'file',
      slashes: true,
      pathname: path.resolve(app.getAppPath(), filePath),
      query,
      search,
      hash
    })
  );
};

WebContents.prototype.copyVideoFrameAt = function (x: number, y: number) {
  this.mainFrame.copyVideoFrameAt(x, y);
};

WebContents.prototype.saveVideoFrameAs = function (x: number, y: number) {
  this.mainFrame.saveVideoFrameAs(x, y);
};

WebContents.prototype.setWindowOpenHandler = function (
  handler: (details: Electron.HandlerDetails) => Electron.WindowOpenHandlerResponse
) {
  this._windowOpenHandler = handler;
};

WebContents.prototype._callWindowOpenHandler = function (
  event: Electron.Event,
  details: Electron.HandlerDetails
): {
  browserWindowConstructorOptions: BrowserWindowConstructorOptions | null;
  outlivesOpener: boolean;
  createWindow?: Electron.CreateWindowFunction;
} {
  const defaultResponse = {
    browserWindowConstructorOptions: null,
    outlivesOpener: false,
    createWindow: undefined
  };
  if (!this._windowOpenHandler) {
    return defaultResponse;
  }

  const response = this._windowOpenHandler(details);

  if (typeof response !== 'object') {
    event.preventDefault();
    console.error(`The window open handler response must be an object, but was instead of type '${typeof response}'.`);
    return defaultResponse;
  }

  if (response === null) {
    event.preventDefault();
    console.error('The window open handler response must be an object, but was instead null.');
    return defaultResponse;
  }

  if (response.action === 'deny') {
    event.preventDefault();
    return defaultResponse;
  } else if (response.action === 'allow') {
    return {
      browserWindowConstructorOptions:
        typeof response.overrideBrowserWindowOptions === 'object' ? response.overrideBrowserWindowOptions : null,
      outlivesOpener: typeof response.outlivesOpener === 'boolean' ? response.outlivesOpener : false,
      createWindow: typeof response.createWindow === 'function' ? response.createWindow : undefined
    };
  } else {
    event.preventDefault();
    console.error("The window open handler response must be an object with an 'action' property of 'allow' or 'deny'.");
    return defaultResponse;
  }
};

// Deprecation warnings for navigation related APIs.
const canGoBackDeprecated = deprecate.warnOnce('webContents.canGoBack', 'webContents.navigationHistory.canGoBack');
WebContents.prototype.canGoBack = function () {
  canGoBackDeprecated();
  return this._canGoBack();
};

const canGoForwardDeprecated = deprecate.warnOnce(
  'webContents.canGoForward',
  'webContents.navigationHistory.canGoForward'
);
WebContents.prototype.canGoForward = function () {
  canGoForwardDeprecated();
  return this._canGoForward();
};

const canGoToOffsetDeprecated = deprecate.warnOnce(
  'webContents.canGoToOffset',
  'webContents.navigationHistory.canGoToOffset'
);
WebContents.prototype.canGoToOffset = function (index: number) {
  canGoToOffsetDeprecated();
  return this._canGoToOffset(index);
};

const clearHistoryDeprecated = deprecate.warnOnce('webContents.clearHistory', 'webContents.navigationHistory.clear');
WebContents.prototype.clearHistory = function () {
  clearHistoryDeprecated();
  return this._clearHistory();
};

const goBackDeprecated = deprecate.warnOnce('webContents.goBack', 'webContents.navigationHistory.goBack');
WebContents.prototype.goBack = function () {
  goBackDeprecated();
  return this._goBack();
};

const goForwardDeprecated = deprecate.warnOnce('webContents.goForward', 'webContents.navigationHistory.goForward');
WebContents.prototype.goForward = function () {
  goForwardDeprecated();
  return this._goForward();
};

const goToIndexDeprecated = deprecate.warnOnce('webContents.goToIndex', 'webContents.navigationHistory.goToIndex');
WebContents.prototype.goToIndex = function (index: number) {
  goToIndexDeprecated();
  return this._goToIndex(index);
};

const goToOffsetDeprecated = deprecate.warnOnce('webContents.goToOffset', 'webContents.navigationHistory.goToOffset');
WebContents.prototype.goToOffset = function (index: number) {
  goToOffsetDeprecated();
  return this._goToOffset(index);
};

const consoleMessageDeprecated = deprecate.warnOnceMessage(
  "'console-message' arguments are deprecated and will be removed. Please use Event<WebContentsConsoleMessageEventParams> object instead."
);

// Add JavaScript wrappers for WebContents class.
WebContents.prototype._init = function () {
  const prefs = this.getLastWebPreferences() || {};
  if (!prefs.nodeIntegration && prefs.preload != null && prefs.sandbox == null) {
    deprecate.log(
      "The default sandbox option for windows without nodeIntegration is changing. Presently, by default, when a window has a preload script, it defaults to being unsandboxed. In Electron 20, this default will be changing, and all windows that have nodeIntegration: false (which is the default) will be sandboxed by default. If your preload script doesn't use Node, no action is needed. If your preload script does use Node, either refactor it to move Node usage to the main process, or specify sandbox: false in your WebPreferences."
    );
  }
  // Read off the ID at construction time, so that it's accessible even after
  // the underlying C++ WebContents is destroyed.
  const id = this.id;
  Object.defineProperty(this, 'id', {
    value: id,
    writable: false
  });

  this._windowOpenHandler = null;

  const ipc = new IpcMainImpl();
  Object.defineProperty(this, 'ipc', {
    get() {
      return ipc;
    },
    enumerable: true
  });

  // Add navigationHistory property which handles session history,
  // maintaining a list of navigation entries for backward and forward navigation.
  Object.defineProperty(this, 'navigationHistory', {
    value: {
      canGoBack: this._canGoBack.bind(this),
      canGoForward: this._canGoForward.bind(this),
      canGoToOffset: this._canGoToOffset.bind(this),
      clear: this._clearHistory.bind(this),
      goBack: this._goBack.bind(this),
      goForward: this._goForward.bind(this),
      goToIndex: this._goToIndex.bind(this),
      goToOffset: this._goToOffset.bind(this),
      getActiveIndex: this._getActiveIndex.bind(this),
      length: this._historyLength.bind(this),
      getEntryAtIndex: this._getNavigationEntryAtIndex.bind(this),
      removeEntryAtIndex: this._removeNavigationEntryAtIndex.bind(this),
      getAllEntries: this._getHistory.bind(this),
      restore: ({ index, entries }: { index?: number; entries: NavigationEntry[] }) => {
        if (index === undefined) {
          index = entries.length - 1;
        }

        if (index < 0 || !entries[index]) {
          throw new Error(
            'Invalid index. Index must be a positive integer and within the bounds of the entries length.'
          );
        }

        try {
          return this._restoreHistory(index, entries);
        } catch (error) {
          return Promise.reject(error);
        }
      }
    },
    writable: false,
    enumerable: true
  });

  if (this.getType() !== 'remote') {
    // Make new windows requested by links behave like "window.open".
    this.on(
      '-new-window',
      (event, url, frameName, disposition, rawFeatures, referrer, postData, sandboxFlags, navigate) => {
        const postBody = postData
          ? {
              data: postData,
              ...parseContentTypeFormat(postData)
            }
          : undefined;
        const details: Electron.HandlerDetails = {
          url,
          frameName,
          features: rawFeatures,
          referrer,
          postBody,
          disposition
        };

        let result: ReturnType<typeof this._callWindowOpenHandler>;
        try {
          result = this._callWindowOpenHandler(event, details);
        } catch (err) {
          event.preventDefault();
          throw err;
        }

        const options = result.browserWindowConstructorOptions;
        if (!event.defaultPrevented) {
          openGuestWindow({
            embedder: this,
            disposition,
            referrer,
            postData,
            overrideBrowserWindowOptions: options || {},
            windowOpenArgs: details,
            outlivesOpener: result.outlivesOpener,
            createWindow: result.createWindow,
            inheritedSandboxFlags: sandboxFlags,
            navigate
          });
        }
      }
    );

    let windowOpenOverriddenOptions: BrowserWindowConstructorOptions | null = null;
    let windowOpenOutlivesOpenerOption: boolean = false;
    let createWindow: Electron.CreateWindowFunction | undefined;

    this.on('-will-add-new-contents', (event, url, frameName, rawFeatures, disposition, referrer, postData) => {
      const postBody = postData
        ? {
            data: postData,
            ...parseContentTypeFormat(postData)
          }
        : undefined;
      const details: Electron.HandlerDetails = {
        url,
        frameName,
        features: rawFeatures,
        disposition,
        referrer,
        postBody
      };

      let result: ReturnType<typeof this._callWindowOpenHandler>;
      try {
        result = this._callWindowOpenHandler(event, details);
      } catch (err) {
        event.preventDefault();
        throw err;
      }

      windowOpenOutlivesOpenerOption = result.outlivesOpener;
      windowOpenOverriddenOptions = result.browserWindowConstructorOptions;
      createWindow = result.createWindow;
      if (!event.defaultPrevented) {
        const secureOverrideWebPreferences = windowOpenOverriddenOptions
          ? {
              // Allow setting of backgroundColor as a webPreference even though
              // it's technically a BrowserWindowConstructorOptions option because
              // we need to access it in the renderer at init time.
              backgroundColor: windowOpenOverriddenOptions.backgroundColor,
              transparent: windowOpenOverriddenOptions.transparent,
              ...windowOpenOverriddenOptions.webPreferences
            }
          : undefined;
        const { webPreferences: parsedWebPreferences } = parseFeatures(rawFeatures);
        const webPreferences = makeWebPreferences({
          embedder: this,
          insecureParsedWebPreferences: parsedWebPreferences,
          secureOverrideWebPreferences
        });
        windowOpenOverriddenOptions = {
          ...windowOpenOverriddenOptions,
          webPreferences
        };
        this._setNextChildWebPreferences(webPreferences);
      }
    });

    // Create a new browser window for "window.open"
    this.on(
      '-add-new-contents',
      (
        event,
        webContents,
        disposition,
        _userGesture,
        _left,
        _top,
        _width,
        _height,
        url,
        frameName,
        referrer,
        rawFeatures,
        postData
      ) => {
        const overriddenOptions = windowOpenOverriddenOptions || undefined;
        const outlivesOpener = windowOpenOutlivesOpenerOption;
        const windowOpenFunction = createWindow;

        createWindow = undefined;
        windowOpenOverriddenOptions = null;
        // false is the default
        windowOpenOutlivesOpenerOption = false;

        if (disposition !== 'foreground-tab' && disposition !== 'new-window' && disposition !== 'background-tab') {
          event.preventDefault();
          return;
        }

        openGuestWindow({
          embedder: this,
          guest: webContents,
          overrideBrowserWindowOptions: overriddenOptions,
          disposition,
          referrer,
          postData,
          windowOpenArgs: {
            url,
            frameName,
            features: rawFeatures
          },
          outlivesOpener,
          createWindow: windowOpenFunction
        });
      }
    );
  }

  const originCounts = new Map<string, number>();
  const openDialogs = new Set<AbortController>();
  this.on('-run-dialog', async (info, callback) => {
    const origin = info.frame.origin === 'file://' ? info.frame.url : info.frame.origin;
    if ((originCounts.get(origin) ?? 0) < 0) return callback(false, '');

    const prefs = this.getLastWebPreferences();
    if (!prefs || prefs.disableDialogs) return callback(false, '');

    // We don't support prompt() for some reason :)
    if (info.dialogType === 'prompt') return callback(false, '');

    originCounts.set(origin, (originCounts.get(origin) ?? 0) + 1);

    // TODO: translate?
    const checkbox =
      originCounts.get(origin)! > 1 && prefs.safeDialogs
        ? prefs.safeDialogsMessage || 'Prevent this app from creating additional dialogs'
        : '';
    const parent = this.getOwnerBrowserWindow();
    const abortController = new AbortController();
    const options: MessageBoxOptions = {
      message: info.messageText,
      checkboxLabel: checkbox,
      signal: abortController.signal,
      ...(info.dialogType === 'confirm'
        ? {
            buttons: ['OK', 'Cancel'],
            defaultId: 0,
            cancelId: 1
          }
        : {
            buttons: ['OK'],
            defaultId: -1, // No default button
            cancelId: 0
          })
    };
    openDialogs.add(abortController);
    const promise =
      parent && !prefs.offscreen ? dialog.showMessageBox(parent, options) : dialog.showMessageBox(options);
    try {
      const result = await promise;
      if (abortController.signal.aborted || this.isDestroyed()) return;
      if (result.checkboxChecked) originCounts.set(origin, -1);
      return callback(result.response === 0, '');
    } finally {
      openDialogs.delete(abortController);
    }
  });

  this.on('-cancel-dialogs', () => {
    for (const controller of openDialogs) {
      controller.abort();
    }
    openDialogs.clear();
  });

  (this as NodeJS.EventEmitter).on('newListener', (eventName: string | symbol, listener: (...args: any[]) => void) => {
    if (eventName === 'console-message') {
      // TODO(samuelmaddock): remove deprecated 'console-message' arguments
      if (listener.length > 1) consoleMessageDeprecated();
      if (!this.isDestroyed()) this._setConsoleMessageObserved(true);
    }
  });
  this.on('removeListener' as any, (eventName: string | symbol) => {
    if (eventName === 'console-message' && !this.isDestroyed() && this.listenerCount('console-message') === 0) {
      this._setConsoleMessageObserved(false);
    }
  });
  // Properties

  Object.defineProperty(this, 'audioMuted', {
    get: () => this.isAudioMuted(),
    set: (muted) => this.setAudioMuted(muted)
  });

  Object.defineProperty(this, 'userAgent', {
    get: () => this.getUserAgent(),
    set: (agent) => this.setUserAgent(agent)
  });

  Object.defineProperty(this, 'zoomLevel', {
    get: () => this.getZoomLevel(),
    set: (level) => this.setZoomLevel(level)
  });

  Object.defineProperty(this, 'zoomFactor', {
    get: () => this.getZoomFactor(),
    set: (factor) => this.setZoomFactor(factor)
  });

  Object.defineProperty(this, 'zoomMode', {
    get: () => this.getZoomMode(),
    set: (mode) => this.setZoomMode(mode)
  });

  Object.defineProperty(this, 'frameRate', {
    get: () => this.getFrameRate(),
    set: (rate) => this.setFrameRate(rate)
  });

  Object.defineProperty(this, 'backgroundThrottling', {
    get: () => this.getBackgroundThrottling(),
    set: (allowed) => this.setBackgroundThrottling(allowed)
  });

  Object.defineProperty(this, 'caretBrowsingEnabled', {
    get: () => this.isCaretBrowsingEnabled(),
    set: (enabled) => this.setCaretBrowsingEnabled(enabled)
  });
};

// Public APIs.
export function create(options = {}): Electron.WebContents {
  return new (WebContents as any)(options);
}

export function fromId(id: number) {
  return binding.fromId(id);
}

export function fromFrame(frame: Electron.WebFrameMain) {
  return binding.fromFrame(frame);
}

export function fromDevToolsTargetId(targetId: string) {
  return binding.fromDevToolsTargetId(targetId);
}

export function getFocusedWebContents() {
  let focused = null;
  for (const contents of binding.getAllWebContents()) {
    if (!contents.isFocused()) continue;
    if (focused == null) focused = contents;
    // Return webview web contents which may be embedded inside another
    // web contents that is also reporting as focused
    if (contents.getType() === 'webview') return contents;
  }
  return focused;
}
export function getAllWebContents() {
  return binding.getAllWebContents();
}
