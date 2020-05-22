import { EventEmitter } from 'events';

const { BrowserView } = process.electronBinding('browser_view');

Object.setPrototypeOf(BrowserView.prototype, EventEmitter.prototype);

BrowserView.fromWebContents = (webContents: Electron.WebContents) => {
  for (const view of BrowserView.getAllViews()) {
    if (view.webContents.equal(webContents)) return view;
  }

  return null;
};

export default BrowserView;
