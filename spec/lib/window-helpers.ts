import { BaseWindow, BrowserWindow, webContents } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

// On macOS, destroying a fullscreen window animates it out of its Space, and
// AppKit ignores fullscreen requests from other windows until that finishes.
// Leave fullscreen first so the next test doesn't start inside the animation.
async function leaveFullScreen(window: BaseWindow) {
  if (process.platform !== 'darwin' || !window.isFullScreen()) return;
  const left = once(window, 'leave-full-screen');
  window.setFullScreen(false);
  await Promise.race([left, new Promise((resolve) => setTimeout(resolve, 5000))]);
}

async function ensureWindowIsClosed(window: BaseWindow | null) {
  if (window && !window.isDestroyed()) {
    await leaveFullScreen(window);
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

export async function closeAllWindows(assertNotWindows = false) {
  let windowsClosed = 0;
  for (const w of BaseWindow.getAllWindows()) {
    await closeWindow(w, { assertNotWindows });
    windowsClosed++;
  }
  return windowsClosed;
}

export async function cleanupWebContents() {
  let webContentsDestroyed = 0;
  const existingWCS = webContents.getAllWebContents();
  for (const contents of existingWCS) {
    const isDestroyed = once(contents, 'destroyed');
    contents.destroy();
    await isDestroyed;
    webContentsDestroyed++;
  }
  return webContentsDestroyed;
}
