import { BaseWindow, BrowserWindow, webContents } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

import { runCleanupFunctions } from './defer-helpers';

async function ensureWindowIsClosed(window: BaseWindow | null) {
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

export const closeWindow = async (window: BaseWindow | null = null) => {
  await ensureWindowIsClosed(window);
};

export async function assertNoWindowsLeaked() {
  const windows = BaseWindow.getAllWindows();
  try {
    expect(windows).to.have.lengthOf(
      0,
      `${windows.length} window(s) leaked across test boundary (ids: ${windows.map((w) => w.id).join(', ')})`
    );
  } finally {
    for (const win of windows) {
      await ensureWindowIsClosed(win);
    }
  }
}

export async function closeAllWindows() {
  // Under vitest, setupFiles-level hooks run after test-file afterEach hooks,
  // so defer()ed cleanups would see already-destroyed windows. Running them
  // here (the innermost afterEach in practice) preserves the mocha ordering.
  await runCleanupFunctions();
  let windowsClosed = 0;
  for (const w of BaseWindow.getAllWindows()) {
    await closeWindow(w);
    windowsClosed++;
  }
  return windowsClosed;
}

export async function cleanupWebContents() {
  let webContentsDestroyed = 0;
  const existingWCS = webContents.getAllWebContents();
  for (const contents of existingWCS) {
    if (contents.isDestroyed()) continue;
    const isDestroyed = once(contents, 'destroyed');
    contents.destroy();
    await isDestroyed;
    webContentsDestroyed++;
  }
  return webContentsDestroyed;
}
