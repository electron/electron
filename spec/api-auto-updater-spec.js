const assert = require('assert')
const {autoUpdater} = require('electron').remote
const {ipcRenderer} = require('electron')

describe('autoUpdater module', function () {
  // XXX(alexeykuzmin): Calling `.skip()` in a 'before' hook
  // doesn't affect nested 'describe's
  beforeEach(function () {
    // Skip autoUpdater tests in MAS build.
    if (process.mas) {
      this.skip()
    }
  })

  describe('checkForUpdates', function () {
    it('emits an error on Windows when called the feed URL is not set', function (done) {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
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
    describe('on Mac', function () {
      before(function () {
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('emits an error when the application is unsigned', function (done) {
        ipcRenderer.once('auto-updater-error', function (event, message) {
          assert.equal(message, 'Could not get code signature for running application')
          done()
        })
        autoUpdater.setFeedURL('')
      })
    })
  })

  describe('getFeedURL', function () {
    it('returns a falsey value by default', function () {
      assert.ok(!autoUpdater.getFeedURL())
    })

    it('correctly fetches the previously set FeedURL', function (done) {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      const updateURL = 'https://fake-update.electron.io'
      autoUpdater.setFeedURL(updateURL)
      assert.equal(autoUpdater.getFeedURL(), updateURL)
      done()
    })
  })

  describe('quitAndInstall', function () {
    it('emits an error on Windows when no update is available', function (done) {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      ipcRenderer.once('auto-updater-error', function (event, message) {
        assert.equal(message, 'No update available, can\'t quit and install')
        done()
      })
      autoUpdater.quitAndInstall()
    })
  })

  describe('error event', function () {
    it('serializes correctly over the remote module', function (done) {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      autoUpdater.once('error', function (error) {
        assert.equal(error instanceof Error, true)
        assert.deepEqual(Object.getOwnPropertyNames(error), ['stack', 'message', 'name'])
        done()
      })

      autoUpdater.setFeedURL('')

      if (process.platform === 'win32') {
        autoUpdater.checkForUpdates()
      }
    })
  })
})
