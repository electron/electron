const { expect } = require('chai')
const { remote } = require('electron')
const { BrowserWindow } = remote

const { emittedOnce } = require('./events-helpers')

async function ensureWindowIsClosed (window) {
  if (window && !window.isDestroyed()) {
    if (window.webContents && !window.webContents.isDestroyed()) {
      // If a window isn't destroyed already, and it has non-destroyed WebContents,
      // then calling destroy() won't immediately destroy it, as it may have
      // <webview> children which need to be destroyed first. In that case, we
      // await the 'closed' event which signals the complete shutdown of the
      // window.
      const isClosed = emittedOnce(window, 'closed')
      window.destroy()
      await isClosed
    } else {
      // If there's no WebContents or if the WebContents is already destroyed,
      // then the 'closed' event has already been emitted so there's nothing to
      // wait for.
      window.destroy()
    }
  }
}

exports.closeWindow = async (window = null,
  { assertSingleWindow } = { assertSingleWindow: true }) => {
  await ensureWindowIsClosed(window)

  if (assertSingleWindow) {
    // Although we want to assert that no windows were left handing around
    // we also want to clean up the left over windows so that no future
    // tests fail as a side effect
    const currentId = remote.getCurrentWindow().id
    const windows = BrowserWindow.getAllWindows()
    for (const win of windows) {
      if (win.id !== currentId) {
        await ensureWindowIsClosed(win)
      }
    }
    expect(windows).to.have.lengthOf(1)
  }
}

exports.waitForWebContentsToLoad = async (webContents) => {
  const didFinishLoadPromise = emittedOnce(webContents, 'did-finish-load')
  if (webContents.isLoadingMainFrame()) {
    await didFinishLoadPromise
  }
}
