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
    describe('on Mac or Windows', () => {
      const noThrow = (fn) => {
        try { fn() } catch (err) {}
      }

      before(function () {
        if (process.platform !== 'win32' && process.platform !== 'darwin') {
          this.skip()
        }
      })

      it('sets url successfully using old (url, headers) syntax', () => {
        noThrow(() => autoUpdater.setFeedURL('http://electronjs.org', { header: 'val' }))
        assert.equal(autoUpdater.getFeedURL(), 'http://electronjs.org')
      })

      it('throws if no url is provided when using the old style', () => {
        assert.throws(
          () => autoUpdater.setFeedURL(),
          err => err.message.includes('Expected an options object with a \'url\' property to be provided') // eslint-disable-line
        )
      })

      it('sets url successfully using new ({ url }) syntax', () => {
        noThrow(() => autoUpdater.setFeedURL({ url: 'http://mymagicurl.local' }))
        assert.equal(autoUpdater.getFeedURL(), 'http://mymagicurl.local')
      })

      it('throws if no url is provided when using the new style', () => {
        assert.throws(
          () => autoUpdater.setFeedURL({ noUrl: 'lol' }),
          err => err.message.includes('Expected options object to contain a \'url\' string property in setFeedUrl call') // eslint-disable-line
        )
      })
    })

    describe('on Mac', function () {
      const isServerTypeError = (err) => err.message.includes('Expected serverType to be \'default\' or \'json\'')

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

      it('does not throw if default is the serverType', () => {
        assert.doesNotThrow(
          () => autoUpdater.setFeedURL({ url: '', serverType: 'default' }),
          isServerTypeError
        )
      })

      it('does not throw if json is the serverType', () => {
        assert.doesNotThrow(
          () => autoUpdater.setFeedURL({ url: '', serverType: 'default' }),
          isServerTypeError
        )
      })

      it('does throw if an unknown string is the serverType', () => {
        assert.throws(
          () => autoUpdater.setFeedURL({ url: '', serverType: 'weow' }),
          isServerTypeError
        )
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
