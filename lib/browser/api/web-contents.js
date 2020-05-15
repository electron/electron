'use strict';

const { EventEmitter } = require('events');
const electron = require('electron');
const path = require('path');
const url = require('url');
const { app, ipcMain, session } = electron;

const { internalWindowOpen } = require('@electron/internal/browser/guest-window-manager');
const NavigationController = require('@electron/internal/browser/navigation-controller');
const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal');
const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils');
const { parseFeatures } = require('@electron/internal/common/parse-features-string');
const { MessagePortMain } = require('@electron/internal/browser/message-port-main');

// session is not used here, the purpose is to make sure session is initalized
// before the webContents module.
// eslint-disable-next-line
session

let nextId = 0;
const getNextId = function () {
  return ++nextId;
};

// Stock page sizes
const PDFPageSizes = {
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

// Default printing setting
const defaultPrintingSetting = {
  // Customizable.
  pageRange: [],
  mediaSize: {},
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
  collate: true
};

// JavaScript implementations of WebContents.
const binding = process.electronBinding('web_contents');
const { WebContents } = binding;

Object.setPrototypeOf(NavigationController.prototype, EventEmitter.prototype);
Object.setPrototypeOf(WebContents.prototype, NavigationController.prototype);

// WebContents::send(channel, args..)
// WebContents::sendToAll(channel, args..)
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

WebContents.prototype.sendToAll = function (channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  }

  const internal = false;
  const sendToAll = true;

  return this._send(internal, sendToAll, channel, args);
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
WebContents.prototype.sendToFrame = function (frameId, channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  } else if (typeof frameId !== 'number') {
    throw new Error('Missing required frameId argument');
  }

  const internal = false;
  const sendToAll = false;

  return this._sendToFrame(internal, sendToAll, frameId, channel, args);
};
WebContents.prototype._sendToFrameInternal = function (frameId, channel, ...args) {
  if (typeof channel !== 'string') {
    throw new Error('Missing required channel argument');
  } else if (typeof frameId !== 'number') {
    throw new Error('Missing required frameId argument');
  }

  const internal = true;
  const sendToAll = false;

  return this._sendToFrame(internal, sendToAll, frameId, channel, args);
};

// Following methods are mapped to webFrame.
const webFrameMethods = [
  'insertCSS',
  'insertText',
  'removeInsertedCSS',
  'setVisualZoomLevelLimits'
];

for (const method of webFrameMethods) {
  WebContents.prototype[method] = function (...args) {
    return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', method, ...args);
  };
}

const waitTillCanExecuteJavaScript = async (webContents) => {
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
  return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', 'executeJavaScript', code, hasUserGesture);
};
WebContents.prototype.executeJavaScriptInIsolatedWorld = async function (code, hasUserGesture) {
  await waitTillCanExecuteJavaScript(this);
  return ipcMainUtils.invokeInWebContents(this, false, 'ELECTRON_INTERNAL_RENDERER_WEB_FRAME_METHOD', 'executeJavaScriptInIsolatedWorld', code, hasUserGesture);
};

// Translate the options of printToPDF.
WebContents.prototype.printToPDF = function (options) {
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
      // Dimensions in Microns
      // 1 meter = 10^6 microns
      printSettings.mediaSize = {
        name: 'CUSTOM',
        custom_display_name: 'Custom',
        height_microns: Math.ceil(pageSize.height),
        width_microns: Math.ceil(pageSize.width)
      };
    } else if (PDFPageSizes[pageSize]) {
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
    return this._printToPDF(printSettings);
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
        options.mediaSize = {
          name: 'CUSTOM',
          custom_display_name: 'Custom',
          height_microns: Math.ceil(pageSize.height),
          width_microns: Math.ceil(pageSize.width)
        };
      } else if (PDFPageSizes[pageSize]) {
        options.mediaSize = PDFPageSizes[pageSize];
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

const addReplyToEvent = (event) => {
  event.reply = (...args) => {
    event.sender.sendToFrame(event.frameId, ...args);
  };
};

const addReplyInternalToEvent = (event) => {
  Object.defineProperty(event, '_replyInternal', {
    configurable: false,
    enumerable: false,
    value: (...args) => {
      event.sender._sendToFrameInternal(event.frameId, ...args);
    }
  });
};

const addReturnValueToEvent = (event) => {
  Object.defineProperty(event, 'returnValue', {
    set: (value) => event.sendReply([value]),
    get: () => {}
  });
};

// Add JavaScript wrappers for WebContents class.
WebContents.prototype._init = function () {
  // The navigation controller.
  NavigationController.call(this, this);

  // Every remote callback from renderer process would add a listener to the
  // render-view-deleted event, so ignore the listeners warning.
  this.setMaxListeners(0);

  // Dispatch IPC messages to the ipc module.
  this.on('-ipc-message', function (event, internal, channel, args) {
    if (internal) {
      addReplyInternalToEvent(event);
      ipcMainInternal.emit(channel, event, ...args);
    } else {
      addReplyToEvent(event);
      this.emit('ipc-message', event, channel, ...args);
      ipcMain.emit(channel, event, ...args);
    }
  });

  this.on('-ipc-invoke', function (event, internal, channel, args) {
    event._reply = (result) => event.sendReply({ result });
    event._throw = (error) => {
      console.error(`Error occurred in handler for '${channel}':`, error);
      event.sendReply({ error: error.toString() });
    };
    const target = internal ? ipcMainInternal : ipcMain;
    if (target._invokeHandlers.has(channel)) {
      target._invokeHandlers.get(channel)(event, ...args);
    } else {
      event._throw(`No handler registered for '${channel}'`);
    }
  });

  this.on('-ipc-message-sync', function (event, internal, channel, args) {
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

  this.on('-ipc-ports', function (event, internal, channel, message, ports) {
    event.ports = ports.map(p => new MessagePortMain(p));
    ipcMain.emit(channel, event, message);
  });

  // Handle context menu action request from pepper plugin.
  this.on('pepper-context-menu', function (event, params, callback) {
    // Access Menu via electron.Menu to prevent circular require.
    const menu = electron.Menu.buildFromTemplate(params.menu);
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

  // The devtools requests the webContents to reload.
  this.on('devtools-reload-page', function () {
    this.reload();
  });

  if (this.getType() !== 'remote') {
    // Make new windows requested by links behave like "window.open".
    this.on('-new-window', (event, url, frameName, disposition,
      rawFeatures, referrer, postData) => {
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
    this.on('-add-new-contents', (event, webContents, disposition,
      userGesture, left, top, width, height, url, frameName,
      referrer, rawFeatures, postData) => {
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
        title: frameName,
        webPreferences,
        ...options
      };

      internalWindowOpen(event, url, referrer, frameName, disposition, mergedOptions, additionalFeatures, postData);
    });

    const prefs = this.getWebPreferences() || {};
    if (prefs.webviewTag && prefs.contextIsolation) {
      electron.deprecate.log('Security Warning: A WebContents was just created with both webviewTag and contextIsolation enabled.  This combination is fundamentally less secure and effectively bypasses the protections of contextIsolation.  We strongly recommend you move away from webviews to OOPIF or BrowserView in order for your app to be more secure');
    }
  }

  this.on('login', (event, ...args) => {
    app.emit('login', event, this, ...args);
  });

  const event = process.electronBinding('event').createEmpty();
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
module.exports = {
  create (options = {}) {
    return binding.create(options);
  },

  fromId (id) {
    return binding.fromId(id);
  },

  getFocusedWebContents () {
    let focused = null;
    for (const contents of binding.getAllWebContents()) {
      if (!contents.isFocused()) continue;
      if (focused == null) focused = contents;
      // Return webview web contents which may be embedded inside another
      // web contents that is also reporting as focused
      if (contents.getType() === 'webview') return contents;
    }
    return focused;
  },

  getAllWebContents () {
    return binding.getAllWebContents();
  }
};
