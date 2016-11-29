const assert = require('assert')
const {BrowserWindow} = require('electron').remote

exports.closeWindow = (window, {assertSingleWindow} = {assertSingleWindow: true}) => {
  if (window == null || window.isDestroyed()) {
    if (assertSingleWindow) {
      assert.equal(BrowserWindow.getAllWindows().length, 1)
    }
    return Promise.resolve()
  } else {
    return new Promise((resolve, reject) => {
      window.once('closed', () => {
        if (assertSingleWindow) {
          assert.equal(BrowserWindow.getAllWindows().length, 1)
        }
        resolve()
      })
      window.setClosable(true)
      window.close()
    })
  }
}
