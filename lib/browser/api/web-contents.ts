import { app, ipcMain, session, deprecate } from 'electron/main';
import type { MenuItem, MenuItemConstructorOptions } from 'electron/main';

import * as url from 'url';
import * as path from 'path';
import { internalWindowOpen } from '@electron/internal/browser/guest-window-manager';
import { NavigationController } from '@electron/internal/browser/navigation-controller';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { parseFeatures } from '@electron/internal/common/parse-features-string';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';

// session is not used here, the purpose is to make sure session is initalized
// before the webContents module.
// eslint-disable-next-line
session

let nextId = 0;
const getNextId = function () {
  return ++nextId;
};

/* eslint-disable camelcase */
type MediaSize = {
  name: string,
  custom_display_name: string,
  height_microns: number,
  width_microns: number,
  is_default?: 'true',
}
/* eslint-enable camelcase */

// Stock page sizes
const PDFPageSizes: Record<string, MediaSize> = {
  A5: {
    custom_display_name: 'A5',
    height_microns: 210000,
    name: 'ISO_A5',
    width_microns: 148000
  },
  A4: {
    custom_display_name: 'A4',
    height_microns: 297000,
    name: 'ISO_A4',
    is_default: 'true',
    width_microns: 210000
  },
  A3: {
    custom_display_name: 'A3',
    height_microns: 420000,
    name: 'ISO_A3',
    width_microns: 297000
  },
  Legal: {
    custom_display_name: 'Legal',
    height_microns: 355600,
    name: 'NA_LEGAL',
    width_microns: 215900
  },
  Letter: {
    custom_display_name: 'Letter',
    height_microns: 279400,
    name: 'NA_LETTER',
    width_microns: 215900
  },
  Tabloid: {
    height_microns: 431800,
    name: 'NA_LEDGER',
    width_microns: 279400,
    custom_display_name: 'Tabloid'
  }
};

// The minimum micron size Chromium accepts is that where:
// Per printing/units.h:
//  * kMicronsPerInch - Length of an inch in 0.001mm unit.
//  * kPointsPerInch - Length of an inch in CSS's 1pt unit.
//
// Formula: (kPointsPerInch / kMicronsPerInch) * size >= 1
//
// Practically, this means microns need to be > 352 microns.
// We therefore need to verify this or it will silently fail.
const isValidCustomPageSize = (width: number, height: number) => {
  return [width, height].every(x => x > 352);
};

// Default printing setting
const defaultPrintingSetting = {
  // Customizable.
  pageRange: [] as {from: number, to: number}[],
  mediaSize: {} as MediaSize,
  landscape: false,
  headerFooterEnabled: false,
  marginsType: 0,
  scaleFactor: 100,
  shouldPrintBackgrounds: false,
  shouldPrintSelectionOnly: false,
  // Non-customizable.
  printWithCloudPrint: false,
  printWithPrivet: false,
  printWithExtension: false,
  pagesPerSheet: 1,
  isFirstRequest: false,
  previewUIID: 0,
  previewModifiable: true,
  printToPDF: true,
  deviceName: 'Save as PDF',
  generateDraftData: true,
  dpiHorizontal: 72,
  dpiVertical: 72,
  rasterizePDF: false,
  duplex: 0,
  copies: 1,
  // 2 = color - see ColorModel in //printing/print_job_constants.h
  color: 2,
  collate: true,
  printerType: 2,
  title: undefined as string | undefined,
  url: undefined as string | undefined
};

// JavaScript implementations of WebContents.
const binding = process._linkedBinding('electron_browser_web_contents');
const { WebContents } = binding as { WebContents: { prototype: Electron.WebContentsInternal } };

WebContents.prototype.send = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  }

  const internal = false;
  const sendToAll = false;

  return this._send(internal, sendToAll, channel, args);
};

WebContents.prototype.postMessage = function (...args) {
  if (Array.isArray(args[2])) {
    args[2] = args[2].map(o => o instanceof MessagePortMain ? o._internalPort : o);
  }
  this._postMessage(...args);
};

WebContents.prototype._sendInternal = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  }

  const internal = true;
  const sendToAll = false;

  return this._send(internal, sendToAll, channel, args);
};
WebContents.prototype._sendInternalToAll = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  }

  const internal = true;
  const sendToAll = true;

  return this._send(internal, sendToAll, channel, args);
};
WebContents.prototype.sendToFrame = function (frame, channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  } else if (!(typeof frame === 'number' || Array.isArray(frame))) {
    throw new Error('Missing required frame argument (must be number or array)');
  }

  const internal = false;
  const sendToAll = false;

  return this._sendToFrame(internal, sendToAll, frame, channel, args);
};
WebContents.prototype._sendToFrameInternal = function (frame, channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  } else if (!(typeof frame === 'number' || Array.isArray(frame))) {
    throw new Error('Missing required frame argument (must be number or array)');
  }

  const internal = true;
  const sendToAll = false;

  return this._sendToFrame(internal, sendToAll, frame, channel, args);
};

// Following methods are mapped to webFrame.
const webFrameMethods = [
  'insertCSS',
  'insertText',
  'removeInsertedCSS',
  'setVisualZoomLevelLimits'
] as ('insertCSS' | 'insertText' | 'removeInsertedCSS' | 'setVisualZoomLevelLimits')[];

for (const method of webFrameMethods) {
  WebContents.prototype[method] = function (...args: any[]): Promise<any> {
    return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', method, ...args);
  };
}

const waitTillCanExecuteJavaScript = async (webContents: Electron.WebContentsInternal) => {
  if (webContents.getURL() && !webContents.isLoadingMainFrame()) return;

  return new Promise((resolve) => {
    webContents.once('did-stop-loading', () => {
      resolve();
    });
  });
};

// Make sure WebContents::executeJavaScript would run the code only when the
// WebContents has been loaded.
WebContents.prototype.executeJavaScript = async function (code, hasUserGesture) {
  await waitTillCanExecuteJavaScript(this);
  return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', 'executeJavaScript', String(code), !!hasUserGesture);
};
WebContents.prototype.executeJavaScriptInIsolatedWorld = async function (worldId, code, hasUserGesture) {
  await waitTillCanExecuteJavaScript(this);
  return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', 'executeJavaScriptInIsolatedWorld', worldId, code, !!hasUserGesture);
};

// Translate the options of printToPDF.

let pendingPromise: Promise<any> | undefined;
WebContents.prototype.printToPDF = async function (options) {
  const printSettings = {
    ...defaultPrintingSetting,
    requestID: getNextId()
  };

  if (options.landscape !== undefined) {
    if (typeof options.landscape !== 'boolean') {
      const error = new Error('landscape must be a Boolean');
      return Promise.reject(error);
    }
    printSettings.landscape = options.landscape;
  }

  if (options.scaleFactor !== undefined) {
    if (typeof options.scaleFactor !== 'number') {
      const error = new Error('scaleFactor must be a Number');
      return Promise.reject(error);
    }
    printSettings.scaleFactor = options.scaleFactor;
  }

  if (options.marginsType !== undefined) {
    if (typeof options.marginsType !== 'number') {
      const error = new Error('marginsType must be a Number');
      return Promise.reject(error);
    }
    printSettings.marginsType = options.marginsType;
  }

  if (options.printSelectionOnly !== undefined) {
    if (typeof options.printSelectionOnly !== 'boolean') {
      const error = new Error('printSelectionOnly must be a Boolean');
      return Promise.reject(error);
    }
    printSettings.shouldPrintSelectionOnly = options.printSelectionOnly;
  }

  if (options.printBackground !== undefined) {
    if (typeof options.printBackground !== 'boolean') {
      const error = new Error('printBackground must be a Boolean');
      return Promise.reject(error);
    }
    printSettings.shouldPrintBackgrounds = options.printBackground;
  }

  if (options.pageRanges !== undefined) {
    const pageRanges = options.pageRanges;
    if (!Object.prototype.hasOwnProperty.call(pageRanges, 'from') || !Object.prototype.hasOwnProperty.call(pageRanges, 'to')) {
      const error = new Error('pageRanges must be an Object with \'from\' and \'to\' properties');
      return Promise.reject(error);
    }

    if (typeof pageRanges.from !== 'number') {
      const error = new Error('pageRanges.from must be a Number');
      return Promise.reject(error);
    }

    if (typeof pageRanges.to !== 'number') {
      const error = new Error('pageRanges.to must be a Number');
      return Promise.reject(error);
    }

    // Chromium uses 1-based page ranges, so increment each by 1.
    printSettings.pageRange = [{
      from: pageRanges.from + 1,
      to: pageRanges.to + 1
    }];
  }

  if (options.headerFooter !== undefined) {
    const headerFooter = options.headerFooter;
    printSettings.headerFooterEnabled = true;
    if (typeof headerFooter === 'object') {
      if (!headerFooter.url || !headerFooter.title) {
        const error = new Error('url and title properties are required for headerFooter');
        return Promise.reject(error);
      }
      if (typeof headerFooter.title !== 'string') {
        const error = new Error('headerFooter.title must be a String');
        return Promise.reject(error);
      }
      printSettings.title = headerFooter.title;

      if (typeof headerFooter.url !== 'string') {
        const error = new Error('headerFooter.url must be a String');
        return Promise.reject(error);
      }
      printSettings.url = headerFooter.url;
    } else {
      const error = new Error('headerFooter must be an Object');
      return Promise.reject(error);
    }
  }

  // Optionally set size for PDF.
  if (options.pageSize !== undefined) {
    const pageSize = options.pageSize;
    if (typeof pageSize === 'object') {
      if (!pageSize.height || !pageSize.width) {
        const error = new Error('height and width properties are required for pageSize');
        return Promise.reject(error);
      }

      // Dimensions in Microns - 1 meter = 10^6 microns
      const height = Math.ceil(pageSize.height);
      const width = Math.ceil(pageSize.width);
      if (!isValidCustomPageSize(width, height)) {
        const error = new Error('height and width properties must be minimum 352 microns.');
        return Promise.reject(error);
      }

      printSettings.mediaSize = {
        name: 'CUSTOM',
        custom_display_name: 'Custom',
        height_microns: height,
        width_microns: width
      };
    } else if (Object.prototype.hasOwnProperty.call(PDFPageSizes, pageSize)) {
      printSettings.mediaSize = PDFPageSizes[pageSize];
    } else {
      const error = new Error(`Unsupported pageSize: ${pageSize}`);
      return Promise.reject(error);
    }
  } else {
    printSettings.mediaSize = PDFPageSizes.A4;
  }

  // Chromium expects this in a 0-100 range number, not as float
  printSettings.scaleFactor = Math.ceil(printSettings.scaleFactor) % 100;
  // PrinterType enum from //printing/print_job_constants.h
  printSettings.printerType = 2;
  if (this._printToPDF) {
    if (pendingPromise) {
      pendingPromise = pendingPromise.then(() => this._printToPDF(printSettings));
    } else {
      pendingPromise = this._printToPDF(printSettings);
    }
    return pendingPromise;
  } else {
    const error = new Error('Printing feature is disabled');
    return Promise.reject(error);
  }
};

WebContents.prototype.print = function (options = {}, callback) {
  // TODO(codebytere): deduplicate argument sanitization by moving rest of
  // print param logic into new file shared between printToPDF and print
  if (typeof options === 'object') {
    // Optionally set size for PDF.
    if (options.pageSize !== undefined) {
      const pageSize = options.pageSize;
      if (typeof pageSize === 'object') {
        if (!pageSize.height || !pageSize.width) {
          throw new Error('height and width properties are required for pageSize');
        }

        // Dimensions in Microns - 1 meter = 10^6 microns
        const height = Math.ceil(pageSize.height);
        const width = Math.ceil(pageSize.width);
        if (!isValidCustomPageSize(width, height)) {
          throw new Error('height and width properties must be minimum 352 microns.');
        }

        (options as any).mediaSize = {
          name: 'CUSTOM',
          custom_display_name: 'Custom',
          height_microns: height,
          width_microns: width
        };
      } else if (PDFPageSizes[pageSize]) {
        (options as any).mediaSize = PDFPageSizes[pageSize];
      } else {
        throw new Error(`Unsupported pageSize: ${pageSize}`);
      }
    }
  }

  if (this._print) {
    if (callback) {
      this._print(options, callback);
    } else {
      this._print(options);
    }
  } else {
    console.error('Error: Printing feature is disabled.');
  }
};

WebContents.prototype.getPrinters = function () {
  if (this._getPrinters) {
    return this._getPrinters();
  } else {
    console.error('Error: Printing feature is disabled.');
    return [];
  }
};

WebContents.prototype.loadFile = function (filePath, options = {}) {
  if (typeof filePath !== 'string') {
    throw new Error('Must pass filePath as a string');
  }
  const { query, search, hash } = options;

  return this.loadURL(url.format({
    protocol: 'file',
    slashes: true,
    pathname: path.resolve(app.getAppPath(), filePath),
    query,
    search,
    hash
  }));
};

const addReplyToEvent = (event: any) => {
  const { processId, frameId } = event;
  event.reply = (...args: any[]) => {
    event.sender.sendToFrame([processId, frameId], ...args);
  };
};

const addReplyInternalToEvent = (event: any) => {
  Object.defineProperty(event, '_replyInternal', {
    configurable: false,
    enumerable: false,
    value: (...args: any[]) => {
      event.sender._sendToFrameInternal(event.frameId, ...args);
    }
  });
};

const addReturnValueToEvent = (event: any) => {
  Object.defineProperty(event, 'returnValue', {
    set: (value) => event.sendReply([value]),
    get: () => {}
  });
};

const loggingEnabled = () => {
  return process.env.ELECTRON_ENABLE_LOGGING || app.commandLine.hasSwitch('enable-logging');
};

// Add JavaScript wrappers for WebContents class.
WebContents.prototype._init = function () {
  // The navigation controller.
  const navigationController = new NavigationController(this);
  this.loadURL = navigationController.loadURL.bind(navigationController);
  this.getURL = navigationController.getURL.bind(navigationController);
  this.stop = navigationController.stop.bind(navigationController);
  this.reload = navigationController.reload.bind(navigationController);
  this.reloadIgnoringCache = navigationController.reloadIgnoringCache.bind(navigationController);
  this.canGoBack = navigationController.canGoBack.bind(navigationController);
  this.canGoForward = navigationController.canGoForward.bind(navigationController);
  this.canGoToIndex = navigationController.canGoToIndex.bind(navigationController);
  this.canGoToOffset = navigationController.canGoToOffset.bind(navigationController);
  this.clearHistory = navigationController.clearHistory.bind(navigationController);
  this.goBack = navigationController.goBack.bind(navigationController);
  this.goForward = navigationController.goForward.bind(navigationController);
  this.goToIndex = navigationController.goToIndex.bind(navigationController);
  this.goToOffset = navigationController.goToOffset.bind(navigationController);
  this.getActiveIndex = navigationController.getActiveIndex.bind(navigationController);
  this.length = navigationController.length.bind(navigationController);
  // Read off the ID at construction time, so that it's accessible even after
  // the underlying C++ WebContents is destroyed.
  const id = this.id;
  Object.defineProperty(this, 'id', {
    value: id,
    writable: false
  });

  // Every remote callback from renderer process would add a listener to the
  // render-view-deleted event, so ignore the listeners warning.
  this.setMaxListeners(0);

  // Dispatch IPC messages to the ipc module.
  this.on('-ipc-message' as any, function (this: Electron.WebContentsInternal, event: any, internal: boolean, channel: string, args: any[]) {
    if (internal) {
      addReplyInternalToEvent(event);
      ipcMainInternal.emit(channel, event, ...args);
    } else {
      addReplyToEvent(event);
      this.emit('ipc-message', event, channel, ...args);
      ipcMain.emit(channel, event, ...args);
    }
  });

  this.on('-ipc-invoke' as any, function (event: any, internal: boolean, channel: string, args: any[]) {
    event._reply = (result: any) => event.sendReply({ result });
    event._throw = (error: Error) => {
      console.error(`Error occurred in handler for '${channel}':`, error);
      event.sendReply({ error: error.toString() });
    };
    const target = internal ? ipcMainInternal : ipcMain;
    if ((target as any)._invokeHandlers.has(channel)) {
      (target as any)._invokeHandlers.get(channel)(event, ...args);
    } else {
      event._throw(`No handler registered for '${channel}'`);
    }
  });

  this.on('-ipc-message-sync' as any, function (this: Electron.WebContentsInternal, event: any, internal: boolean, channel: string, args: any[]) {
    addReturnValueToEvent(event);
    if (internal) {
      addReplyInternalToEvent(event);
      ipcMainInternal.emit(channel, event, ...args);
    } else {
      addReplyToEvent(event);
      this.emit('ipc-message-sync', event, channel, ...args);
      ipcMain.emit(channel, event, ...args);
    }
  });

  this.on('-ipc-ports' as any, function (event: any, internal: boolean, channel: string, message: any, ports: any[]) {
    event.ports = ports.map(p => new MessagePortMain(p));
    ipcMain.emit(channel, event, message);
  });

  // Handle context menu action request from pepper plugin.
  this.on('pepper-context-menu' as any, function (event: any, params: {x: number, y: number, menu: Array<(MenuItemConstructorOptions) | (MenuItem)>}, callback: () => void) {
    // Access Menu via electron.Menu to prevent circular require.
    const menu = require('electron').Menu.buildFromTemplate(params.menu);
    menu.popup({
      window: event.sender.getOwnerBrowserWindow(),
      x: params.x,
      y: params.y,
      callback
    });
  });

  this.on('crashed', (event, ...args) => {
    app.emit('renderer-process-crashed', event, this, ...args);
  });

  this.on('render-process-gone', (event, details) => {
    app.emit('render-process-gone', event, this, details);

    // Log out a hint to help users better debug renderer crashes.
    if (loggingEnabled()) {
      console.info(`Renderer process ${details.reason} - see https://www.electronjs.org/docs/tutorial/application-debugging for potential debugging information.`);
    }
  });

  // The devtools requests the webContents to reload.
  this.on('devtools-reload-page', function (this: Electron.WebContentsInternal) {
    this.reload();
  });

  if (this.getType() !== 'remote') {
    // Make new windows requested by links behave like "window.open".
    this.on('-new-window' as any, (event: any, url: string, frameName: string, disposition: string,
      rawFeatures: string, referrer: string, postData: string) => {
      const { options, webPreferences, additionalFeatures } = parseFeatures(rawFeatures);
      const mergedOptions = {
        show: true,
        width: 800,
        height: 600,
        title: frameName,
        webPreferences,
        ...options
      };

      internalWindowOpen(event, url, referrer, frameName, disposition, mergedOptions, additionalFeatures, postData);
    });

    // Create a new browser window for the native implementation of
    // "window.open", used in sandbox and nativeWindowOpen mode.
    this.on('-add-new-contents' as any, (event: any, webContents: Electron.WebContentsInternal, disposition: string,
      userGesture: boolean, left: number, top: number, width: number, height: number, url: string, frameName: string,
      referrer: string, rawFeatures: string, postData: string) => {
      if ((disposition !== 'foreground-tab' && disposition !== 'new-window' &&
           disposition !== 'background-tab')) {
        event.preventDefault();
        return;
      }

      const { options, webPreferences, additionalFeatures } = parseFeatures(rawFeatures);
      const mergedOptions = {
        show: true,
        width: 800,
        height: 600,
        webContents,
        webPreferences,
        ...options
      };

      internalWindowOpen(event, url, referrer, frameName, disposition, mergedOptions, additionalFeatures, postData);
    });

    const prefs = this.getWebPreferences() || {};
    if (prefs.webviewTag && prefs.contextIsolation) {
      deprecate.log('Security Warning: A WebContents was just created with both webviewTag and contextIsolation enabled.  This combination is fundamentally less secure and effectively bypasses the protections of contextIsolation.  We strongly recommend you move away from webviews to OOPIF or BrowserView in order for your app to be more secure');
    }
  }

  this.on('login', (event, ...args) => {
    app.emit('login', event, this, ...args);
  });

  this.on('ready-to-show' as any, () => {
    const owner = this.getOwnerBrowserWindow();
    if (owner && !owner.isDestroyed()) {
      process.nextTick(() => {
        owner.emit('ready-to-show');
      });
    }
  });

  const event = process._linkedBinding('electron_browser_event').createEmpty();
  app.emit('web-contents-created', event, this);

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

  Object.defineProperty(this, 'frameRate', {
    get: () => this.getFrameRate(),
    set: (rate) => this.setFrameRate(rate)
  });

  Object.defineProperty(this, 'backgroundThrottling', {
    get: () => this.getBackgroundThrottling(),
    set: (allowed) => this.setBackgroundThrottling(allowed)
  });
};

// Public APIs.
export function create (options = {}) {
  return binding.create(options);
}

export function fromId (id: string) {
  return binding.fromId(id);
}

export function getFocusedWebContents () {
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
export function getAllWebContents () {
  return binding.getAllWebContents();
}
