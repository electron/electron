import { expect } from 'chai'
import { BrowserWindow, WebContents } from 'electron'
import { emittedOnce } from './events-helpers';

export const closeWindow = async (
  window: BrowserWindow | null = null,
  { assertNotWindows } = { assertNotWindows: true }
) => {
  if (window && !window.isDestroyed()) {
    window.destroy()
  }

  if (assertNotWindows) {
    const windows = BrowserWindow.getAllWindows()
    for (const win of windows) {
      win.destroy()
    }
    expect(windows).to.have.lengthOf(0)
  }
}

exports.waitForWebContentsToLoad = async (webContents: WebContents) => {
  const didFinishLoadPromise = emittedOnce(webContents, 'did-finish-load')
  if (webContents.isLoadingMainFrame()) {
    await didFinishLoadPromise
  }
}
