import { BrowserWindow, AutoResizeOptions, Rectangle, WebContentsView, WebPreferences } from 'electron/main';

export default class BrowserView {
  #webContentsView: WebContentsView

  constructor (options?: WebPreferences) {
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
  ownerWindow: BrowserWindow | null = null;

  get webContentsView () {
    return this.#webContentsView;
  }
}
