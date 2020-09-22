import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import type { WebContents, LoadURLOptions } from 'electron/main';
import { EventEmitter } from 'events';

// The history operation in renderer is redirected to browser.
ipcMainInternal.on('ELECTRON_NAVIGATION_CONTROLLER_GO_BACK', function (event) {
  event.sender.goBack();
});

ipcMainInternal.on('ELECTRON_NAVIGATION_CONTROLLER_GO_FORWARD', function (event) {
  event.sender.goForward();
});

ipcMainInternal.on('ELECTRON_NAVIGATION_CONTROLLER_GO_TO_OFFSET', function (event, offset) {
  event.sender.goToOffset(offset);
});

ipcMainInternal.on('ELECTRON_NAVIGATION_CONTROLLER_LENGTH', function (event) {
  event.returnValue = (event.sender as any).length();
});

// JavaScript implementation of Chromium's NavigationController.
// Instead of relying on Chromium for history control, we compeletely do history
// control on user land, and only rely on WebContents.loadURL for navigation.
// This helps us avoid Chromium's various optimizations so we can ensure renderer
// process is restarted everytime.
export class NavigationController extends EventEmitter {
  currentIndex: number = -1;
  inPageIndex: number = -1;
  pendingIndex: number = -1;
  history: string[] = [];

  constructor (private webContents: WebContents) {
    super();
    this.clearHistory();

    // webContents may have already navigated to a page.
    if (this.webContents._getURL()) {
      this.currentIndex++;
      this.history.push(this.webContents._getURL());
    }
    this.webContents.on('navigation-entry-committed' as any, (event: any, url: string, inPage: boolean, replaceEntry: boolean) => {
      if (this.inPageIndex > -1 && !inPage) {
        // Navigated to a new page, clear in-page mark.
        this.inPageIndex = -1;
      } else if (this.inPageIndex === -1 && inPage && !replaceEntry) {
        // Started in-page navigations.
        this.inPageIndex = this.currentIndex;
      }
      if (this.pendingIndex >= 0) {
        // Go to index.
        this.currentIndex = this.pendingIndex;
        this.pendingIndex = -1;
        this.history[this.currentIndex] = url;
      } else if (replaceEntry) {
        // Non-user initialized navigation.
        this.history[this.currentIndex] = url;
      } else {
        // Normal navigation. Clear history.
        this.history = this.history.slice(0, this.currentIndex + 1);
        this.currentIndex++;
        this.history.push(url);
      }
    });
  }

  loadURL (url: string, options?: LoadURLOptions): Promise<void> {
    if (options == null) {
      options = {};
    }
    const p = new Promise<void>((resolve, reject) => {
      const resolveAndCleanup = () => {
        removeListeners();
        resolve();
      };
      const rejectAndCleanup = (errorCode: number, errorDescription: string, url: string) => {
        const err = new Error(`${errorDescription} (${errorCode}) loading '${typeof url === 'string' ? url.substr(0, 2048) : url}'`);
        Object.assign(err, { errno: errorCode, code: errorDescription, url });
        removeListeners();
        reject(err);
      };
      const finishListener = () => {
        resolveAndCleanup();
      };
      const failListener = (event: any, errorCode: number, errorDescription: string, validatedURL: string, isMainFrame: boolean) => {
        if (isMainFrame) {
          rejectAndCleanup(errorCode, errorDescription, validatedURL);
        }
      };

      let navigationStarted = false;
      const navigationListener = (event: any, url: string, isSameDocument: boolean, isMainFrame: boolean) => {
        if (isMainFrame) {
          if (navigationStarted && !isSameDocument) {
            // the webcontents has started another unrelated navigation in the
            // main frame (probably from the app calling `loadURL` again); reject
            // the promise
            // We should only consider the request aborted if the "navigation" is
            // actually navigating and not simply transitioning URL state in the
            // current context.  E.g. pushState and `location.hash` changes are
            // considered navigation events but are triggered with isSameDocument.
            // We can ignore these to allow virtual routing on page load as long
            // as the routing does not leave the document
            return rejectAndCleanup(-3, 'ERR_ABORTED', url);
          }
          navigationStarted = true;
        }
      };
      const stopLoadingListener = () => {
        // By the time we get here, either 'finish' or 'fail' should have fired
        // if the navigation occurred. However, in some situations (e.g. when
        // attempting to load a page with a bad scheme), loading will stop
        // without emitting finish or fail. In this case, we reject the promise
        // with a generic failure.
        // TODO(jeremy): enumerate all the cases in which this can happen. If
        // the only one is with a bad scheme, perhaps ERR_INVALID_ARGUMENT
        // would be more appropriate.
        rejectAndCleanup(-2, 'ERR_FAILED', url);
      };
      const removeListeners = () => {
        this.webContents.removeListener('did-finish-load', finishListener);
        this.webContents.removeListener('did-fail-load', failListener);
        this.webContents.removeListener('did-start-navigation', navigationListener);
        this.webContents.removeListener('did-stop-loading', stopLoadingListener);
      };
      this.webContents.on('did-finish-load', finishListener);
      this.webContents.on('did-fail-load', failListener);
      this.webContents.on('did-start-navigation', navigationListener);
      this.webContents.on('did-stop-loading', stopLoadingListener);
    });
    // Add a no-op rejection handler to silence the unhandled rejection error.
    p.catch(() => {});
    this.pendingIndex = -1;
    this.webContents._loadURL(url, options);
    this.webContents.emit('load-url', url, options);
    return p;
  }

  getURL () {
    if (this.currentIndex === -1) {
      return '';
    } else {
      return this.history[this.currentIndex];
    }
  }

  stop () {
    this.pendingIndex = -1;
    return this.webContents._stop();
  }

  reload () {
    this.pendingIndex = this.currentIndex;
    return this.webContents._loadURL(this.getURL(), {});
  }

  reloadIgnoringCache () {
    this.pendingIndex = this.currentIndex;
    return this.webContents._loadURL(this.getURL(), {
      extraHeaders: 'pragma: no-cache\n',
      reloadIgnoringCache: true
    } as any);
  }

  canGoBack () {
    return this.getActiveIndex() > 0;
  }

  canGoForward () {
    return this.getActiveIndex() < this.history.length - 1;
  }

  canGoToIndex (index: number) {
    return index >= 0 && index < this.history.length;
  }

  canGoToOffset (offset: number) {
    return this.canGoToIndex(this.currentIndex + offset);
  }

  clearHistory () {
    this.history = [];
    this.currentIndex = -1;
    this.pendingIndex = -1;
    this.inPageIndex = -1;
  }

  goBack () {
    if (!this.canGoBack()) {
      return;
    }
    this.pendingIndex = this.getActiveIndex() - 1;
    if (this.inPageIndex > -1 && this.pendingIndex >= this.inPageIndex) {
      return this.webContents._goBack();
    } else {
      return this.webContents._loadURL(this.history[this.pendingIndex], {});
    }
  }

  goForward () {
    if (!this.canGoForward()) {
      return;
    }
    this.pendingIndex = this.getActiveIndex() + 1;
    if (this.inPageIndex > -1 && this.pendingIndex >= this.inPageIndex) {
      return this.webContents._goForward();
    } else {
      return this.webContents._loadURL(this.history[this.pendingIndex], {});
    }
  }

  goToIndex (index: number) {
    if (!this.canGoToIndex(index)) {
      return;
    }
    this.pendingIndex = index;
    return this.webContents._loadURL(this.history[this.pendingIndex], {});
  }

  goToOffset (offset: number) {
    if (!this.canGoToOffset(offset)) {
      return;
    }
    const pendingIndex = this.currentIndex + offset;
    if (this.inPageIndex > -1 && pendingIndex >= this.inPageIndex) {
      this.pendingIndex = pendingIndex;
      return this.webContents._goToOffset(offset);
    } else {
      return this.goToIndex(pendingIndex);
    }
  }

  getActiveIndex () {
    if (this.pendingIndex === -1) {
      return this.currentIndex;
    } else {
      return this.pendingIndex;
    }
  }

  length () {
    return this.history.length;
  }
}
