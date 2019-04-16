const { expect } = require('chai')
const { remote } = require('electron')
const { BrowserWindow } = remote

const { emittedOnce } = require('./events-helpers')

exports.closeWindow = async (window = null,
  { assertSingleWindow } = { assertSingleWindow: true }) => {
  const windowExists = (window !== null) && !window.isDestroyed()
  if (windowExists) {
    const isClosed = emittedOnce(window, 'closed')
    window.setClosable(true)
    window.close()
    await isClosed
  }

  if (assertSingleWindow) {
    // Although we want to assert that no windows were left handing around
    // we also want to clean up the left over windows so that no future
    // tests fail as a side effect
    const currentId = remote.getCurrentWindow().id
    const windows = BrowserWindow.getAllWindows()
    for (const win of windows) {
      if (win.id !== currentId) {
        const closePromise = emittedOnce(win, 'closed')
        win.close()
        await closePromise
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
