import { BaseWindow, WebContents, Event, BrowserView, TouchBar } from 'electron/main';
import * as deprecate from '@electron/internal/common/deprecate';
import type { BrowserWindow as BWT } from 'electron/main';
const { BrowserWindow } = process._linkedBinding('electron_browser_window') as { BrowserWindow: typeof BWT };

Object.setPrototypeOf(BrowserWindow.prototype, BaseWindow.prototype);

BrowserWindow.prototype._init = function (this: BWT) {
  // Call parent class's _init.
  BaseWindow.prototype._init.call(this);

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

BrowserWindow.prototype.setTouchBar = function (touchBar) {
  (TouchBar as any)._setOnWindow(touchBar, this);
};

// Forwarded to webContents:

const deprecatedMethods = [
  'loadURL',
  'getURL',
  'loadFile',
  'reload',
  'send',
  'openDevTools',
  'closeDevTools',
  'isDevToolsOpened',
  'isDevToolsFocused',
  'toggleDevTools',
  'inspectElement',
  'inspectSharedWorker',
  'inspectServiceWorker',
  'showDefinitionForSelection',
  'capturePage',
  'getBackgroundThrottling',
  'setBackgroundThrottling'
];

for (const method of deprecatedMethods as (keyof Electron.BrowserWindow)[]) {
  (BrowserWindow.prototype[method] as any) = function (this: Electron.BrowserWindow, ...args: any[]) {
    deprecate.warn(`browserWindow.${method}`, `browserWindow.webContents.${method}`);
    return (this.webContents[method as keyof Electron.WebContents] as any)(...args);
  };
}

module.exports = BrowserWindow;
