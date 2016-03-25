const assert = require('assert')
const autoUpdater = require('electron').remote.autoUpdater
const ipcRenderer = require('electron').ipcRenderer

// Skip autoUpdater tests in MAS build.
if (!process.mas) {
  describe('autoUpdater module', function () {
    describe('checkForUpdates', function () {
      it('emits an error on Windows when called the feed URL is not set', function (done) {
        if (process.platform !== 'win32') {
          return done()
        }

        ipcRenderer.once('auto-updater-error', function (event, message) {
          assert.equal(message, 'Update URL is not set')
          done()
        })
        autoUpdater.setFeedURL('')
        autoUpdater.checkForUpdates()
      })
    })

    describe('setFeedURL', function () {
      it('emits an error on Mac OS X when the application is unsigned', function (done) {
        if (process.platform !== 'darwin') {
          return done()
        }

        ipcRenderer.once('auto-updater-error', function (event, message) {
          assert.equal(message, 'Could not get code signature for running application')
          done()
        })
        autoUpdater.setFeedURL('')
      })
    })
  })
}
