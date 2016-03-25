const assert = require('assert')
const path = require('path')
const temp = require('temp')

describe('third-party module', function () {
  var fixtures = path.join(__dirname, 'fixtures')
  temp.track()

  if (process.platform !== 'win32' || process.execPath.toLowerCase().indexOf('\\out\\d\\') === -1) {
    describe('runas', function () {
      it('can be required in renderer', function () {
        require('runas')
      })

      it('can be required in node binary', function (done) {
        var runas = path.join(fixtures, 'module', 'runas.js')
        var child = require('child_process').fork(runas)
        child.on('message', function (msg) {
          assert.equal(msg, 'ok')
          done()
        })
      })
    })

    describe('ffi', function () {
      it('does not crash', function () {
        var ffi = require('ffi')
        var libm = ffi.Library('libm', {
          ceil: ['double', ['double']]
        })
        assert.equal(libm.ceil(1.5), 2)
      })
    })
  }

  describe('q', function () {
    var Q = require('q')
    describe('Q.when', function () {
      it('emits the fullfil callback', function (done) {
        Q(true).then(function (val) {
          assert.equal(val, true)
          done()
        })
      })
    })
  })
})
