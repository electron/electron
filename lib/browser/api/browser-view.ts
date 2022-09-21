import { BrowserWindow, AutoResizeOptions, Rectangle, WebContentsView, WebPreferences } from 'electron/main';

const v8Util = process._linkedBinding('electron_common_v8_util');

export default class BrowserView {
  #webContentsView: WebContentsView

  constructor (options?: WebPreferences) {
    if (options && 'webContents' in options) {
      v8Util.setHiddenValue(options, 'webContents', (options as any).webContents);
    }
    this.#webContentsView = new WebContentsView(options);
  }

  get webContents () {
    return this.#webContentsView.webContents;
  }

  setBounds (bounds: Rectangle) {
    this.#webContentsView.setBounds(bounds);
  }

  getBounds () {
    return this.#webContentsView.getBounds();
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  setAutoResize (options: AutoResizeOptions) {
    throw new Error('not implemented');
  }

  setBackgroundColor (color: string) {
    this.#webContentsView.setBackgroundColor(color);
  }

  // Internal methods
  get ownerWindow (): BrowserWindow | null {
    return this.webContents.getOwnerBrowserWindow();
  }

  set ownerWindow (w: BrowserWindow | null) {
    this.webContents._setOwnerWindow(w);
  }

  get webContentsView () {
    return this.#webContentsView;
  }
}
