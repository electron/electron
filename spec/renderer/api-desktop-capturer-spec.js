const assert = require('assert')
const {desktopCapturer, remote} = require('electron')

const isCI = remote.getGlobal('isCi')

describe('desktopCapturer', () => {
  before(function () {
    if (isCI && process.platform === 'win32') {
      this.skip()
    }
  })

  it('should return a non-empty array of sources', (done) => {
    desktopCapturer.getSources({
      types: ['window', 'screen']
    }, (error, sources) => {
      assert.equal(error, null)
      assert.notEqual(sources.length, 0)
      done()
    })
  })

  it('throws an error for invalid options', (done) => {
    desktopCapturer.getSources(['window', 'screen'], (error) => {
      assert.equal(error.message, 'Invalid options')
      done()
    })
  })

  it('does not throw an error when called more than once (regression)', (done) => {
    let callCount = 0
    const callback = (error, sources) => {
      callCount++
      assert.equal(error, null)
      assert.notEqual(sources.length, 0)
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({types: ['window', 'screen']}, callback)
    desktopCapturer.getSources({types: ['window', 'screen']}, callback)
  })

  it('responds to subsequest calls of different options', (done) => {
    let callCount = 0
    const callback = (error, sources) => {
      callCount++
      assert.equal(error, null)
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({types: ['window']}, callback)
    desktopCapturer.getSources({types: ['screen']}, callback)
  })
})
