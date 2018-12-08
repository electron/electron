const { autoUpdater } = require('electron').remote
const { ipcRenderer } = require('electron')
const { expect } = require('chai')

const { platformDescribe, platformIt } = require('./test-helpers')

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
    platformIt('emits an error on Windows when called the feed URL is not set', ['win32'], function (done) {
      ipcRenderer.once('auto-updater-error', (event, message) => {
        expect(message).to.equal('Update URL is not set')
        done()
      })
      autoUpdater.setFeedURL('')
      autoUpdater.checkForUpdates()
    })
  })

  describe('getFeedURL', () => {
    it('returns a falsey value by default', () => {
      expect(autoUpdater.getFeedURL()).to.equal('')
    })

    // Can't use the auto updater on non-win32 platforms without code signing
    platformIt('correctly fetches the previously set FeedURL', ['win32'], () => {
      const updateURL = 'https://fake-update.electron.io'
      autoUpdater.setFeedURL(updateURL)
      expect(autoUpdater.getFeedURL()).to.equal(updateURL)
    })
  })

  describe('setFeedURL', function () {
    platformDescribe('on Mac or Windows', ['win32', 'darwin'], () => {
      const noThrow = (fn) => {
        try { fn() } catch (err) {}
      }

      it('sets url successfully using old (url, headers) syntax', () => {
        const url = 'http://electronjs.org'
        noThrow(() => autoUpdater.setFeedURL(url, { header: 'val' }))
        expect(autoUpdater.getFeedURL()).to.equal(url)
      })

      it('throws if no url is provided when using the old style', () => {
        expect(() => autoUpdater.setFeedURL(),
          err => err.message.includes('Expected an options object with a \'url\' property to be provided') // eslint-disable-line
        ).to.throw()
      })

      it('sets url successfully using new ({ url }) syntax', () => {
        const url = 'http://mymagicurl.local'
        noThrow(() => autoUpdater.setFeedURL({ url }))
        expect(autoUpdater.getFeedURL()).to.equal(url)
      })

      it('throws if no url is provided when using the new style', () => {
        expect(() => autoUpdater.setFeedURL({ noUrl: 'lol' }),
          err => err.message.includes('Expected options object to contain a \'url\' string property in setFeedUrl call') // eslint-disable-line
        ).to.throw()
      })
    })

    platformDescribe('on Mac', ['darwin'], function () {
      const isServerTypeError = (err) => err.message.includes('Expected serverType to be \'default\' or \'json\'')

      it('emits an error when the application is unsigned', done => {
        ipcRenderer.once('auto-updater-error', (event, message) => {
          expect(message).equal('Could not get code signature for running application')
          done()
        })
        autoUpdater.setFeedURL('')
      })

      it('does not throw if default is the serverType', () => {
        expect(() => autoUpdater.setFeedURL({ url: '', serverType: 'default' }),
          isServerTypeError
        ).to.not.throw()
      })

      it('does not throw if json is the serverType', () => {
        expect(() => autoUpdater.setFeedURL({ url: '', serverType: 'default' }),
          isServerTypeError
        ).to.not.throw()
      })

      it('does throw if an unknown string is the serverType', () => {
        expect(() => autoUpdater.setFeedURL({ url: '', serverType: 'weow' }),
          isServerTypeError
        ).to.throw()
      })
    })
  })

  describe('quitAndInstall', () => {
    platformIt('emits an error on Windows when no update is available', ['win32'], (done) => {
      ipcRenderer.once('auto-updater-error', (event, message) => {
        expect(message).to.equal('No update available, can\'t quit and install')
        done()
      })
      autoUpdater.quitAndInstall()
    })
  })

  describe('error event', () => {
    platformIt('serializes correctly over the remote module', ['win32', 'darwin'], (done) => {
      autoUpdater.once('error', error => {
        expect(error).to.be.an.instanceof(Error)
        expect(Object.getOwnPropertyNames(error)).to.deep.equal(['stack', 'message', 'name'])
        done()
      })

      // setFeedURL emits an error on darwin
      autoUpdater.setFeedURL('')

      // checkForUpdates emits an error on win32
      if (process.platform === 'win32') {
        autoUpdater.checkForUpdates()
      }
    })
  })
})
