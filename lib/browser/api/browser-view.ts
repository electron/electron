import { EventEmitter } from 'events';

const { BrowserView } = process._linkedBinding('electron_browser_browser_view');

Object.setPrototypeOf(BrowserView.prototype, EventEmitter.prototype);

BrowserView.fromWebContents = (webContents: Electron.WebContents) => {
  for (const view of BrowserView.getAllViews()) {
    if (view.webContents.equal(webContents)) return view;
  }

  return null;
};

export default BrowserView;
