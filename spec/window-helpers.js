const { expect } = require('chai')
const { BrowserWindow } = require('electron').remote

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
    expect(BrowserWindow.getAllWindows()).to.have.lengthOf(1)
  }
}

exports.waitForWebContentsToLoad = async (webContents) => {
  const didFinishLoadPromise = emittedOnce(webContents, 'did-finish-load')
  if (webContents.isLoadingMainFrame()) {
    await didFinishLoadPromise
  }
}
