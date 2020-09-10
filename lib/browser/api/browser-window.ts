import { BaseWindow, WebContents, Event, BrowserView, TouchBar } from 'electron/main';
import type { BrowserWindow as BWT } from 'electron/main';
const { BrowserWindow } = process._linkedBinding('electron_browser_window') as { BrowserWindow: typeof BWT };

Object.setPrototypeOf(BrowserWindow.prototype, BaseWindow.prototype);

(BrowserWindow.prototype as any)._init = function (this: BWT) {
  // Call parent class's _init.
  (BaseWindow.prototype as any)._init.call(this);

  // Avoid recursive require.
  const { app } = require('electron');

  const nativeSetBounds = this.setBounds;
  this.setBounds = (bounds, ...opts) => {
    bounds = {
      ...this.getBounds(),
      ...bounds
    };
    nativeSetBounds.call(this, bounds, ...opts);
  };

  // Sometimes the webContents doesn't get focus when window is shown, so we
  // have to force focusing on webContents in this case. The safest way is to
  // focus it when we first start to load URL, if we do it earlier it won't
  // have effect, if we do it later we might move focus in the page.
  //
  // Though this hack is only needed on macOS when the app is launched from
  // Finder, we still do it on all platforms in case of other bugs we don't
  // know.
  if (this.webContents._initiallyShown) {
    this.webContents.once('load-url' as any, function (this: WebContents) {
      this.focus();
    });
  }

  // Redirect focus/blur event to app instance too.
  this.on('blur', (event: Event) => {
    app.emit('browser-window-blur', event, this);
  });
  this.on('focus', (event: Event) => {
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

  // Notify the creation of the window.
  const event = process._linkedBinding('electron_browser_event').createEmpty();
  app.emit('browser-window-created', event, this);

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
    if (window.isFocused() || window.isDevToolsFocused()) return window;
  }
  return null;
};

BrowserWindow.fromWebContents = (webContents: WebContents) => {
  return webContents.getOwnerBrowserWindow();
};

BrowserWindow.fromBrowserView = (browserView: BrowserView) => {
  for (const window of BrowserWindow.getAllWindows()) {
    if (window.getBrowserView() === browserView) return window;
  }

  return null;
};

BrowserWindow.prototype.setTouchBar = function (touchBar) {
  (TouchBar as any)._setOnWindow(touchBar, this);
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

module.exports = BrowserWindow;
