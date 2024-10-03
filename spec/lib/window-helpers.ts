import { BaseWindow, BrowserWindow } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

async function ensureWindowIsClosed (window: BaseWindow | null) {
  if (window && !window.isDestroyed()) {
    if (window instanceof BrowserWindow && window.webContents && !window.webContents.isDestroyed()) {
      // If a window isn't destroyed already, and it has non-destroyed WebContents,
      // then calling destroy() won't immediately destroy it, as it may have
      // <webview> children which need to be destroyed first. In that case, we
      // await the 'closed' event which signals the complete shutdown of the
      // window.
      const isClosed = once(window, 'closed');
      window.destroy();
      await isClosed;
    } else {
      // If there's no WebContents or if the WebContents is already destroyed,
      // then the 'closed' event has already been emitted so there's nothing to
      // wait for.
      window.destroy();
    }
  }
}

export const closeWindow = async (
  window: BaseWindow | null = null,
  { assertNotWindows } = { assertNotWindows: true }
) => {
  await ensureWindowIsClosed(window);

  if (assertNotWindows) {
    let windows = BaseWindow.getAllWindows();
    if (windows.length > 0) {
      setTimeout(async () => {
        // Wait until next tick to assert that all windows have been closed.
        windows = BaseWindow.getAllWindows();
        try {
          expect(windows).to.have.lengthOf(0);
        } finally {
          for (const win of windows) {
            await ensureWindowIsClosed(win);
          }
        }
      });
    }
  }
};

export async function closeAllWindows () {
  for (const w of BaseWindow.getAllWindows()) {
    await closeWindow(w, { assertNotWindows: false });
  }
}
