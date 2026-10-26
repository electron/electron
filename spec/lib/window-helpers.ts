import { app, BaseWindow, BrowserWindow, webContents } from 'electron/main';

import { expect } from 'vitest';

import { once } from 'node:events';

// BrowserWindows that have emitted 'enter-full-screen' but not yet
// 'leave-full-screen'.
const inFullScreen = new WeakSet<BaseWindow>();
if (process.platform === 'darwin') {
  app.on('browser-window-created', (_event, window) => {
    window.on('enter-full-screen', () => inFullScreen.add(window));
    window.on('leave-full-screen', () => inFullScreen.delete(window));
  });
}

// On macOS, destroying a fullscreen window animates it out of its Space, and
// AppKit ignores fullscreen and other window state requests from other
// windows until that finishes. Leave fullscreen first so the next test
// doesn't start inside the animation.
//
// isFullScreen() is not enough to tell: a window can already be on its way
// out, for example when another window in the same fullscreen Space left
// fullscreen and took it along. isFullScreen() is then false, but the window
// still emits 'leave-full-screen' once the animation ends, so wait for that.
async function leaveFullScreen(window: BaseWindow) {
  if (process.platform !== 'darwin') return;
  if (!window.isFullScreen() && !inFullScreen.has(window)) return;
  const left = once(window, 'leave-full-screen').then(() => true);
  if (window.isFullScreen()) window.setFullScreen(false);
  const timeout = new Promise<boolean>((resolve) => setTimeout(() => resolve(false), 5000));
  if (!(await Promise.race([left, timeout]))) {
    console.warn(`closeWindow(): window ${window.id} did not leave fullscreen within 5s, destroying it anyway`);
  }
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
    const windows = BaseWindow.getAllWindows();
    if (windows.length > 0) {
      setTimeout(async () => {
        // Wait until next tick to assert that all windows have been closed.
        // Only look at the windows that were open when closeWindow() was
        // called: by now the next test may already have created its own.
        const leftover = windows.filter((win) => !win.isDestroyed());
        try {
          expect(leftover).toHaveLength(0);
        } finally {
          for (const win of leftover) {
            await ensureWindowIsClosed(win);
          }
        }
      });
    }
  }
};

// Takes no parameters on purpose: it is registered directly as an afterEach /
// afterAll hook, and vitest hands hooks their context as the first argument.
export async function closeAllWindows() {
  let windowsClosed = 0;
  for (const w of BaseWindow.getAllWindows()) {
    await closeWindow(w, { assertNotWindows: false });
    windowsClosed++;
  }
  return windowsClosed;
}

export async function cleanupWebContents() {
  let webContentsDestroyed = 0;
  const existingWCS = webContents.getAllWebContents();
  for (const contents of existingWCS) {
    // Destroying one webContents can take others in the list down with it
    // (e.g. its guests or devtools), and calling destroy() on those throws.
    if (contents.isDestroyed()) continue;
    const isDestroyed = once(contents, 'destroyed');
    contents.destroy();
    await isDestroyed;
    webContentsDestroyed++;
  }
  return webContentsDestroyed;
}
