const assert = require('assert')
const Module = require('module')
const path = require('path')
const {remote} = require('electron')
const {BrowserWindow} = remote
const {closeWindow} = require('./window-helpers')

const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

describe('modules support', function () {
  var fixtures = path.join(__dirname, 'fixtures')

  describe('third-party module', function () {
    describe('runas', function () {
      if (!nativeModulesEnabled) return

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
      if (!nativeModulesEnabled) return
      if (process.platform === 'win32') return

      it('does not crash', function () {
        var ffi = require('ffi')
        var libm = ffi.Library('libm', {
          ceil: ['double', ['double']]
        })
        assert.equal(libm.ceil(1.5), 2)
      })
    })

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

    describe('coffee-script', function () {
      it('can be registered and used to require .coffee files', function () {
        assert.doesNotThrow(function () {
          require('coffee-script').register()
        })
        assert.strictEqual(require('./fixtures/module/test.coffee'), true)
      })
    })
  })

  describe('global variables', function () {
    describe('process', function () {
      it('can be declared in a module', function () {
        assert.strictEqual(require('./fixtures/module/declare-process'), 'declared process')
      })
    })

    describe('global', function () {
      it('can be declared in a module', function () {
        assert.strictEqual(require('./fixtures/module/declare-global'), 'declared global')
      })
    })

    describe('Buffer', function () {
      it('can be declared in a module', function () {
        assert.strictEqual(require('./fixtures/module/declare-buffer'), 'declared Buffer')
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

        modulePath = process.resourcesPath + '-foo'
        let nodeModulePaths = Module._nodeModulePaths(modulePath)
        assert(nodeModulePaths.includes(path.join(modulePath, 'node_modules')))
        assert(nodeModulePaths.includes(path.join(modulePath, '..', 'node_modules')))

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
          path.join(modulePath, 'node_modules'),
          path.resolve('/node_modules')
        ])
      })
    })
  })

  describe('require', () => {
    describe('when loaded URL is not file: protocol', () => {
      let w

      beforeEach(() => {
        w = new BrowserWindow({
          show: false
        })
      })

      afterEach(async () => {
        await closeWindow(w)
        w = null
      })

      it('searches for module under app directory', async () => {
        w.loadURL('about:blank')
        const result = await w.webContents.executeJavaScript('typeof require("q").when')
        assert.equal(result, 'function')
      })
    })
  })
})
