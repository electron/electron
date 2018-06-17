const chai = require('chai')
const dirtyChai = require('dirty-chai')
const {desktopCapturer, remote, screen} = require('electron')
const features = process.atomBinding('features')

const {expect} = chai
chai.use(dirtyChai)

const isCI = remote.getGlobal('isCi')

describe('desktopCapturer', () => {
  before(function () {
    if (!features.isDesktopCapturerEnabled()) {
      // It's been disabled during build time.
      this.skip()
      return
    }

    if (isCI && process.platform === 'win32') {
      this.skip()
    }
  })

  it('should return a non-empty array of sources', done => {
    desktopCapturer.getSources({
      types: ['window', 'screen']
    }, (error, sources) => {
      expect(error).to.be.null()
      expect(sources.length).to.not.equal(0)
      done()
    })
  })

  it('throws an error for invalid options', done => {
    desktopCapturer.getSources(['window', 'screen'], error => {
      expect(error.message).to.equal('Invalid options')
      done()
    })
  })

  it('does not throw an error when called more than once (regression)', (done) => {
    let callCount = 0
    const callback = (error, sources) => {
      callCount++
      expect(error).to.be.null()
      expect(sources.length).to.not.equal(0)
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({types: ['window', 'screen']}, callback)
    desktopCapturer.getSources({types: ['window', 'screen']}, callback)
  })

  it('responds to subsequent calls of different options', done => {
    let callCount = 0
    const callback = (error, sources) => {
      callCount++
      expect(error).to.be.null()
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({types: ['window']}, callback)
    desktopCapturer.getSources({types: ['screen']}, callback)
  })

  it('returns an empty display_id for window sources on Windows and Mac', done => {
    // Linux doesn't return any window sources.
    if (process.platform !== 'win32' && process.platform !== 'darwin') {
      return done()
    }

    desktopCapturer.getSources({types: ['window']}, (error, sources) => {
      expect(error).to.be.null()
      expect(sources.length).to.not.equal(0)
      sources.forEach((source) => {
        expect(source.display_id.length).to.equal(0)
      })
      done()
    })
  })

  it('returns display_ids matching the Screen API on Windows and Mac', done => {
    if (process.platform !== 'win32' && process.platform !== 'darwin') {
      return done()
    }

    const displays = screen.getAllDisplays()
    desktopCapturer.getSources({types: ['screen']}, (error, sources) => {
      expect(error).to.be.null()
      expect(sources.length).to.not.equal(0)
      expect(sources.length).to.equal(displays.length)

      for (let i = 0; i < sources.length; i++) {
        expect(sources[i].display_id).to.equal(displays[i].id.toString())
      }
      done()
    })
  })
})
