const assert = require('assert')
const desktopCapturer = require('electron').desktopCapturer

describe('desktopCapturer', function () {
  it('should return a non-empty array of sources', function (done) {
    desktopCapturer.getSources({
      types: ['window', 'screen']
    }, function (error, sources) {
      assert.equal(error, null)
      assert.notEqual(sources.length, 0)
      done()
    })
  })

  it('does not throw an error when called more than once (regression)', function (done) {
    var callCount = 0
    var callback = function (error, sources) {
      callCount++
      assert.equal(error, null)
      assert.notEqual(sources.length, 0)
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({types: ['window', 'screen']}, callback)
    desktopCapturer.getSources({types: ['window', 'screen']}, callback)
  })

  it('responds to subsequest calls of different options', function (done) {
    var callCount = 0
    var callback = function (error, sources) {
      callCount++
      assert.equal(error, null)
      if (callCount === 2) done()
    }

    desktopCapturer.getSources({types: ['window']}, callback)
    desktopCapturer.getSources({types: ['screen']}, callback)
  })
})
