const assert = require('assert')
const {autoUpdater} = require('electron').remote
const {ipcRenderer} = require('electron')

describe.only('autoUpdater module', () => {
  beforeEach(function () {
    if (process.mas) {
      this.skip()
    }
  })

  describe('checkForUpdates', () => {
    it('emits an error on Windows when called the feed URL is not set', (done) => {
      if (process.platform !== 'win32') return done()

      ipcRenderer.once('auto-updater-error', (event, message) => {
        assert.equal(message, 'Update URL is not set')
        done()
      })

      autoUpdater.setFeedURL('')
      autoUpdater.checkForUpdates()
    })
  })

  describe('setFeedURL', () => {
    describe('on Mac', () => {
      before(function () {
        if (process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('emits an error when the application is unsigned', (done) => {
        ipcRenderer.once('auto-updater-error', (event, message) => {
          assert.equal(message, 'Could not get code signature for running application')
          done()
        })

        autoUpdater.setFeedURL('')
      })
    })
  })

  describe('getFeedURL', () => {
    it('returns a falsey value by default', () => {
      assert.ok(!autoUpdater.getFeedURL())
    })

    it('correctly fetches the previously set FeedURL', (done) => {
      if (process.platform !== 'win32') return done()

      const updateURL = 'https://fake-update.electron.io'
      autoUpdater.setFeedURL(updateURL)
      assert.equal(autoUpdater.getFeedURL(), updateURL)
      done()
    })
  })

  describe('quitAndInstall', () => {
    it('emits an error on Windows when no update is available', (done) => {
      if (process.platform !== 'win32') return done()

      ipcRenderer.once('auto-updater-error', (event, message) => {
        assert.equal(message, 'No update available, can\'t quit and install')
        done()
      })

      autoUpdater.quitAndInstall()
    })
  })

  describe('error event', () => {
    it('serializes correctly over the remote module', (done) => {
      if (process.platform === 'linux') return done()

      autoUpdater.once('error', (error) => {
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
