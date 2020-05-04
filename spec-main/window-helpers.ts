import { expect } from 'chai';
import { BrowserWindow } from 'electron/main';
import { emittedOnce } from './events-helpers';

async function ensureWindowIsClosed (window: BrowserWindow | null) {
  if (window && !window.isDestroyed()) {
    if (window.webContents && !window.webContents.isDestroyed()) {
      // If a window isn't destroyed already, and it has non-destroyed WebContents,
      // then calling destroy() won't immediately destroy it, as it may have
      // <webview> children which need to be destroyed first. In that case, we
      // await the 'closed' event which signals the complete shutdown of the
      // window.
      const isClosed = emittedOnce(window, 'closed');
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
  window: BrowserWindow | null = null,
  { assertNotWindows } = { assertNotWindows: true }
) => {
  await ensureWindowIsClosed(window);

  if (assertNotWindows) {
    const windows = BrowserWindow.getAllWindows();
    try {
      expect(windows).to.have.lengthOf(0);
    } finally {
      for (const win of windows) {
        await ensureWindowIsClosed(win);
      }
    }
  }
};

export async function closeAllWindows () {
  for (const w of BrowserWindow.getAllWindows()) {
    await closeWindow(w, { assertNotWindows: false });
  }
}
