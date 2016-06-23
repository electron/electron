const assert = require('assert')
const Module = require('module')
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
      if (process.platform === 'win32') return

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

describe('Module._nodeModulePaths', function () {
  describe('when the path is inside the resources path', function () {
    it('does not include paths outside of the resources path', function () {
      let modulePath = process.resourcesPath
      assert.deepEqual(Module._nodeModulePaths(modulePath), [
        path.join(process.resourcesPath, 'node_modules')
      ])

      modulePath = path.join(process.resourcesPath, 'foo')
      assert.deepEqual(Module._nodeModulePaths(modulePath), [
        path.join(process.resourcesPath, 'foo', 'node_modules'),
        path.join(process.resourcesPath, 'node_modules')
      ])

      modulePath = path.join(process.resourcesPath, 'node_modules', 'foo')
      assert.deepEqual(Module._nodeModulePaths(modulePath), [
        path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
        path.join(process.resourcesPath, 'node_modules')
      ])

      modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'bar')
      assert.deepEqual(Module._nodeModulePaths(modulePath), [
        path.join(process.resourcesPath, 'node_modules', 'foo', 'bar', 'node_modules'),
        path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
        path.join(process.resourcesPath, 'node_modules')
      ])

      modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar')
      assert.deepEqual(Module._nodeModulePaths(modulePath), [
        path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar', 'node_modules'),
        path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
        path.join(process.resourcesPath, 'node_modules')
      ])
    })
  })

  describe('when the path is outside the resources path', function () {
    it('includes paths outside of the resources path', function () {
      let modulePath = path.resolve('/foo')
      assert.deepEqual(Module._nodeModulePaths(modulePath), [
        path.join(modulePath, 'node_modules')
      ])
    })
  })
})
