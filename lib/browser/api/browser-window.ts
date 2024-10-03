import { BaseWindow, WebContents, BrowserView } from 'electron/main';
import type { BrowserWindow as BWT } from 'electron/main';

const { BrowserWindow } = process._linkedBinding('electron_browser_window') as { BrowserWindow: typeof BWT };

Object.setPrototypeOf(BrowserWindow.prototype, BaseWindow.prototype);

BrowserWindow.prototype._init = function (this: BWT) {
  // Call parent class's _init.
  (BaseWindow.prototype as any)._init.call(this);

  // Avoid recursive require.
  const { app } = require('electron');

  // Set ID at construction time so it's accessible after
  // underlying window destruction.
  const id = this.id;
  Object.defineProperty(this, 'id', {
    value: id,
    writable: false
  });

  const nativeSetBounds = this.setBounds;
  this.setBounds = (bounds, ...opts) => {
    bounds = {
      ...this.getBounds(),
      ...bounds
    };
    nativeSetBounds.call(this, bounds, ...opts);
  };

  // Redirect focus/blur event to app instance too.
  this.on('blur', (event: Electron.Event) => {
    app.emit('browser-window-blur', event, this);
  });
  this.on('focus', (event: Electron.Event) => {
    app.emit('browser-window-focus', event, this);
  });

  // Subscribe to visibilityState changes and pass to renderer process.
  let isVisible = this.isVisible() && !this.isMinimized();
  const visibilityChanged = () => {
    const newState = this.isVisible() && !this.isMinimized();
    if (isVisible !== newState) {
      isVisible = newState;
      const visibilityState = isVisible ? 'visible' : 'hidden';
      this.webContents.emit('-window-visibility-change', visibilityState);
    }
  };

  const visibilityEvents = ['show', 'hide', 'minimize', 'maximize', 'restore'];
  for (const event of visibilityEvents) {
    this.on(event as any, visibilityChanged);
  }

  this._browserViews = [];

  this.on('closed', () => {
    this._browserViews.forEach(b => b.webContents?.close({ waitForBeforeUnload: true }));
  });

  // Notify the creation of the window.
  app.emit('browser-window-created', { preventDefault () {} }, this);

  Object.defineProperty(this, 'devToolsWebContents', {
    enumerable: true,
    configurable: false,
    get () {
      return this.webContents.devToolsWebContents;
    }
  });
};

const isBrowserWindow = (win: any) => {
  return win && win.constructor.name === 'BrowserWindow';
};

BrowserWindow.fromId = (id: number) => {
  const win = BaseWindow.fromId(id);
  return isBrowserWindow(win) ? win as any as BWT : null;
};

BrowserWindow.getAllWindows = () => {
  return BaseWindow.getAllWindows().filter(isBrowserWindow) as any[] as BWT[];
};

BrowserWindow.getFocusedWindow = () => {
  for (const window of BrowserWindow.getAllWindows()) {
    if (!window.isDestroyed() && window.webContents && !window.webContents.isDestroyed()) {
      if (window.isFocused() || window.webContents.isDevToolsFocused()) return window;
    }
  }
  return null;
};

BrowserWindow.fromWebContents = (webContents: WebContents) => {
  return webContents.getOwnerBrowserWindow();
};

BrowserWindow.fromBrowserView = (browserView: BrowserView) => {
  return BrowserWindow.fromWebContents(browserView.webContents);
};

// Forwarded to webContents:

BrowserWindow.prototype.loadURL = function (...args) {
  return this.webContents.loadURL(...args);
};

BrowserWindow.prototype.getURL = function () {
  return this.webContents.getURL();
};

BrowserWindow.prototype.loadFile = function (...args) {
  return this.webContents.loadFile(...args);
};

BrowserWindow.prototype.reload = function (...args) {
  return this.webContents.reload(...args);
};

BrowserWindow.prototype.send = function (...args) {
  return this.webContents.send(...args);
};

BrowserWindow.prototype.openDevTools = function (...args) {
  return this.webContents.openDevTools(...args);
};

BrowserWindow.prototype.closeDevTools = function () {
  return this.webContents.closeDevTools();
};

BrowserWindow.prototype.isDevToolsOpened = function () {
  return this.webContents.isDevToolsOpened();
};

BrowserWindow.prototype.isDevToolsFocused = function () {
  return this.webContents.isDevToolsFocused();
};

BrowserWindow.prototype.toggleDevTools = function () {
  return this.webContents.toggleDevTools();
};

BrowserWindow.prototype.inspectElement = function (...args) {
  return this.webContents.inspectElement(...args);
};

BrowserWindow.prototype.inspectSharedWorker = function () {
  return this.webContents.inspectSharedWorker();
};

BrowserWindow.prototype.inspectServiceWorker = function () {
  return this.webContents.inspectServiceWorker();
};

BrowserWindow.prototype.showDefinitionForSelection = function () {
  return this.webContents.showDefinitionForSelection();
};

BrowserWindow.prototype.capturePage = function (...args) {
  return this.webContents.capturePage(...args);
};

BrowserWindow.prototype.getBackgroundThrottling = function () {
  return this.webContents.getBackgroundThrottling();
};

BrowserWindow.prototype.setBackgroundThrottling = function (allowed: boolean) {
  return this.webContents.setBackgroundThrottling(allowed);
};

BrowserWindow.prototype.addBrowserView = function (browserView: BrowserView) {
  if (browserView.ownerWindow) { browserView.ownerWindow.removeBrowserView(browserView); }
  this.contentView.addChildView(browserView.webContentsView);
  browserView.ownerWindow = this;
  browserView.webContents._setOwnerWindow(this);
  this._browserViews.push(browserView);
};

BrowserWindow.prototype.setBrowserView = function (browserView: BrowserView) {
  this._browserViews.forEach(bv => {
    this.removeBrowserView(bv);
  });
  if (browserView) { this.addBrowserView(browserView); }
};

BrowserWindow.prototype.removeBrowserView = function (browserView: BrowserView) {
  const idx = this._browserViews.indexOf(browserView);
  if (idx >= 0) {
    this.contentView.removeChildView(browserView.webContentsView);
    browserView.ownerWindow = null;
    this._browserViews.splice(idx, 1);
  }
};

BrowserWindow.prototype.getBrowserView = function () {
  if (this._browserViews.length > 1) {
    throw new Error('This BrowserWindow has multiple BrowserViews - use getBrowserViews() instead');
  }
  return this._browserViews[0] ?? null;
};

BrowserWindow.prototype.getBrowserViews = function () {
  return [...this._browserViews];
};

BrowserWindow.prototype.setTopBrowserView = function (browserView: BrowserView) {
  if (browserView.ownerWindow !== this) {
    throw new Error('Given BrowserView is not attached to the window');
  }
  const idx = this._browserViews.indexOf(browserView);
  if (idx >= 0) {
    this.contentView.addChildView(browserView.webContentsView);
    this._browserViews.splice(idx, 1);
    this._browserViews.push(browserView);
  }
};

module.exports = BrowserWindow;
