import { BrowserWindow, AutoResizeOptions, Rectangle, WebContentsView, WebPreferences, WebContents } from 'electron/main';

const v8Util = process._linkedBinding('electron_common_v8_util');

export default class BrowserView {
  #webContentsView: WebContentsView;
  #ownerWindow: BrowserWindow | null = null;

  #destroyListener: ((e: any) => void) | null = null;

  // AutoResize state
  #resizeListener: ((...args: any[]) => void) | null = null;
  #lastWindowSize: {width: number, height: number} = { width: 0, height: 0 };
  #autoResizeFlags: AutoResizeOptions = {};

  constructor (options: {webPreferences: WebPreferences, webContents?: WebContents} = { webPreferences: {} }) {
    const { webPreferences = {}, webContents } = options;
    if (webContents) {
      v8Util.setHiddenValue(webPreferences, 'webContents', webContents);
    }
    webPreferences.type = 'browserView';
    this.#webContentsView = new WebContentsView({ webPreferences });

    this.#destroyListener = this.#onDestroy.bind(this);
    this.#webContentsView.webContents.once('destroyed', this.#destroyListener);
  }

  get webContents () {
    return this.#webContentsView.webContents;
  }

  setBounds (bounds: Rectangle) {
    this.#webContentsView.setBounds(bounds);
    this.#autoHorizontalProportion = null;
    this.#autoVerticalProportion = null;
  }

  getBounds () {
    return this.#webContentsView.getBounds();
  }

  setAutoResize (options: AutoResizeOptions) {
    if (options == null || typeof options !== 'object') {
      throw new Error('Invalid auto resize options');
    }

    this.#autoResizeFlags = {
      width: !!options.width,
      height: !!options.height,
      horizontal: !!options.horizontal,
      vertical: !!options.vertical
    };

    this.#autoHorizontalProportion = null;
    this.#autoVerticalProportion = null;
  }

  setBackgroundColor (color: string) {
    this.#webContentsView.setBackgroundColor(color);
  }

  // Internal methods
  get ownerWindow (): BrowserWindow | null {
    return this.#ownerWindow;
  }

  // We can't rely solely on the webContents' owner window because
  // a webContents can be closed by the user while the BrowserView
  // remains alive and attached to a BrowserWindow.
  set ownerWindow (w: BrowserWindow | null) {
    this.#removeResizeListener();

    if (this.webContents && !this.webContents.isDestroyed()) {
      this.webContents._setOwnerWindow(w);
    }

    this.#ownerWindow = w;
    if (w) {
      this.#lastWindowSize = w.getBounds();
      w.on('resize', this.#resizeListener = this.#autoResize.bind(this));
      w.on('closed', () => {
        this.#removeResizeListener();
        this.#ownerWindow = null;
        this.#destroyListener = null;
      });
    }
  }

  #onDestroy () {
    // Ensure that if #webContentsView's webContents is destroyed,
    // the WebContentsView is removed from the view hierarchy.
    this.#ownerWindow?.contentView.removeChildView(this.webContentsView);
  }

  #removeResizeListener () {
    if (this.#ownerWindow && this.#resizeListener) {
      this.#ownerWindow.off('resize', this.#resizeListener);
      this.#resizeListener = null;
    }
  }

  #autoHorizontalProportion: {width: number, left: number} | null = null;
  #autoVerticalProportion: {height: number, top: number} | null = null;
  #autoResize () {
    if (!this.ownerWindow) {
      throw new Error('Electron bug: #autoResize called without owner window');
    };

    if (this.#autoResizeFlags.horizontal && this.#autoHorizontalProportion == null) {
      const viewBounds = this.#webContentsView.getBounds();
      this.#autoHorizontalProportion = {
        width: this.#lastWindowSize.width / viewBounds.width,
        left: this.#lastWindowSize.width / viewBounds.x
      };
    }

    if (this.#autoResizeFlags.vertical && this.#autoVerticalProportion == null) {
      const viewBounds = this.#webContentsView.getBounds();
      this.#autoVerticalProportion = {
        height: this.#lastWindowSize.height / viewBounds.height,
        top: this.#lastWindowSize.height / viewBounds.y
      };
    }

    const newBounds = this.ownerWindow.getBounds();
    let widthDelta = newBounds.width - this.#lastWindowSize.width;
    let heightDelta = newBounds.height - this.#lastWindowSize.height;
    if (!this.#autoResizeFlags.width) widthDelta = 0;
    if (!this.#autoResizeFlags.height) heightDelta = 0;

    const newViewBounds = this.#webContentsView.getBounds();
    if (widthDelta || heightDelta) {
      this.#webContentsView.setBounds({
        ...newViewBounds,
        width: newViewBounds.width + widthDelta,
        height: newViewBounds.height + heightDelta
      });
    }

    if (this.#autoHorizontalProportion) {
      newViewBounds.width = newBounds.width / this.#autoHorizontalProportion.width;
      newViewBounds.x = newBounds.width / this.#autoHorizontalProportion.left;
    }

    if (this.#autoVerticalProportion) {
      newViewBounds.height = newBounds.height / this.#autoVerticalProportion.height;
      newViewBounds.y = newBounds.y / this.#autoVerticalProportion.top;
    }

    if (this.#autoHorizontalProportion || this.#autoVerticalProportion) {
      this.#webContentsView.setBounds(newViewBounds);
    }

    // Update #lastWindowSize value after browser windows resize
    this.#lastWindowSize = {
      width: newBounds.width,
      height: newBounds.height
    };
  }

  get webContentsView () {
    return this.#webContentsView;
  }
}
