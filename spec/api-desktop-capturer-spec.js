const chai = require('chai')
const dirtyChai = require('dirty-chai')
const chaiAsPromised = require('chai-as-promised')
const { desktopCapturer, ipcRenderer, remote } = require('electron')
const { screen } = remote
const features = process.atomBinding('features')

const { expect } = chai
chai.use(dirtyChai)
chai.use(chaiAsPromised)

const isCI = remote.getGlobal('isCi')

describe('desktopCapturer', () => {
  before(function () {
    if (!features.isDesktopCapturerEnabled() || process.arch.indexOf('arm') === 0) {
      // It's been disabled during build time.
      this.skip()
      return
    }

    if (isCI && process.platform === 'win32') {
      this.skip()
    }
  })

  it('should return a non-empty array of sources', async () => {
    const sources = await desktopCapturer.getSources({ types: ['window', 'screen'] })
    expect(sources).to.be.an('array').that.is.not.empty()
  })

  // TODO(codebytere): remove when promisification is complete
  it('should return a non-empty array of sources (callback)', (done) => {
    desktopCapturer.getSources({ types: ['window', 'screen'] }, (err, sources) => {
      expect(sources).to.be.an('array').that.is.not.empty()
      expect(err).to.be.null()
      done()
    })
  })

  it('throws an error for invalid options', async () => {
    const promise = desktopCapturer.getSources(['window', 'screen'])
    expect(promise).to.be.eventually.rejectedWith(Error, 'Invalid options')
  })

  it('does not throw an error when called more than once (regression)', async () => {
    const sources1 = await desktopCapturer.getSources({ types: ['window', 'screen'] })
    expect(sources1).to.be.an('array').that.is.not.empty()

    const sources2 = await desktopCapturer.getSources({ types: ['window', 'screen'] })
    expect(sources2).to.be.an('array').that.is.not.empty()
  })

  it('responds to subsequent calls of different options', async () => {
    const promise1 = desktopCapturer.getSources({ types: ['window'] })
    expect(promise1).to.not.eventually.be.rejected()

    const promise2 = desktopCapturer.getSources({ types: ['screen'] })
    expect(promise2).to.not.eventually.be.rejected()
  })

  // TODO(codebytere): remove when promisification is complete
  it('responds to subsequent calls of different options (callback)', (done) => {
    let callCount = 0
    const callback = (error, sources) => {
      callCount++
      expect(error).to.be.null()
      expect(sources).to.not.be.null()
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({ types: ['window'] }, callback)
    desktopCapturer.getSources({ types: ['screen'] }, callback)
  })

  it('returns an empty display_id for window sources on Windows and Mac', async () => {
    // Linux doesn't return any window sources.
    if (process.platform !== 'win32' && process.platform !== 'darwin') return

    const { BrowserWindow } = remote
    const w = new BrowserWindow({ width: 200, height: 200 })

    const sources = await desktopCapturer.getSources({ types: ['window'] })
    w.destroy()
    expect(sources).to.be.an('array').that.is.not.empty()
    for (const { display_id: displayId } of sources) {
      expect(displayId).to.be.a('string').and.be.empty()
    }
  })

  it('returns display_ids matching the Screen API on Windows and Mac', async () => {
    if (process.platform !== 'win32' && process.platform !== 'darwin') return

    const displays = screen.getAllDisplays()
    const sources = await desktopCapturer.getSources({ types: ['screen'] })
    expect(sources).to.be.an('array').of.length(displays.length)

    for (let i = 0; i < sources.length; i++) {
      expect(sources[i].display_id).to.equal(displays[i].id.toString())
    }

    it('returns empty sources when blocked', async () => {
      ipcRenderer.send('handle-next-desktop-capturer-get-sources')
      const sources = await desktopCapturer.getSources({ types: ['screen'] })
      expect(sources).to.be.empty()
    })
  })
})
