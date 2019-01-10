const assert = require('assert')
const Module = require('module')
const path = require('path')
const fs = require('fs')
const { remote } = require('electron')
const { BrowserWindow } = remote
const { closeWindow } = require('./window-helpers')

const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

describe('modules support', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('third-party module', () => {
    (nativeModulesEnabled ? describe : describe.skip)('runas', () => {
      it('can be required in renderer', () => {
        require('runas')
      })

      it('can be required in node binary', (done) => {
        const runas = path.join(fixtures, 'module', 'runas.js')
        const child = require('child_process').fork(runas)
        child.on('message', (msg) => {
          assert.strictEqual(msg, 'ok')
          done()
        })
      })

      if (process.platform === 'win32') {
        it('can be required if electron.exe is renamed', () => {
          const { execPath } = remote.process
          const testExecPath = path.join(path.dirname(execPath), 'test.exe')
          fs.copyFileSync(execPath, testExecPath)
          try {
            const runasFixture = path.join(fixtures, 'module', 'runas-renamed.js')
            assert.ok(fs.existsSync(runasFixture))
            const child = require('child_process').spawnSync(testExecPath, [runasFixture])
            assert.strictEqual(child.status, 0)
          } finally {
            fs.unlinkSync(testExecPath)
          }
        })
      }
    })

    // TODO(alexeykuzmin): Disabled during the Chromium 62 (Node.js 9) upgrade.
    // Enable it back when "ffi" module supports Node.js 9.
    // https://github.com/electron/electron/issues/11274
    xdescribe('ffi', () => {
      before(function () {
        if (!nativeModulesEnabled || process.platform === 'win32' ||
            process.arch === 'arm64') {
          this.skip()
        }
      })

      it('does not crash', () => {
        const ffi = require('ffi')
        const libm = ffi.Library('libm', {
          ceil: ['double', ['double']]
        })
        assert.strictEqual(libm.ceil(1.5), 2)
      })
    })

    describe('q', () => {
      const Q = require('q')
      describe('Q.when', () => {
        it('emits the fullfil callback', (done) => {
          Q(true).then((val) => {
            assert.strictEqual(val, true)
            done()
          })
        })
      })
    })

    describe('coffee-script', () => {
      it('can be registered and used to require .coffee files', () => {
        assert.doesNotThrow(() => {
          require('coffee-script').register()
        })
        assert.strictEqual(require('./fixtures/module/test.coffee'), true)
      })
    })
  })

  describe('global variables', () => {
    describe('process', () => {
      it('can be declared in a module', () => {
        assert.strictEqual(require('./fixtures/module/declare-process'), 'declared process')
      })
    })

    describe('global', () => {
      it('can be declared in a module', () => {
        assert.strictEqual(require('./fixtures/module/declare-global'), 'declared global')
      })
    })

    describe('Buffer', () => {
      it('can be declared in a module', () => {
        assert.strictEqual(require('./fixtures/module/declare-buffer'), 'declared Buffer')
      })
    })
  })

  describe('Module._nodeModulePaths', () => {
    describe('when the path is inside the resources path', () => {
      it('does not include paths outside of the resources path', () => {
        let modulePath = process.resourcesPath
        assert.deepStrictEqual(Module._nodeModulePaths(modulePath), [
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = process.resourcesPath + '-foo'
        const nodeModulePaths = Module._nodeModulePaths(modulePath)
        assert(nodeModulePaths.includes(path.join(modulePath, 'node_modules')))
        assert(nodeModulePaths.includes(path.join(modulePath, '..', 'node_modules')))

        modulePath = path.join(process.resourcesPath, 'foo')
        assert.deepStrictEqual(Module._nodeModulePaths(modulePath), [
          path.join(process.resourcesPath, 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo')
        assert.deepStrictEqual(Module._nodeModulePaths(modulePath), [
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'bar')
        assert.deepStrictEqual(Module._nodeModulePaths(modulePath), [
          path.join(process.resourcesPath, 'node_modules', 'foo', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar')
        assert.deepStrictEqual(Module._nodeModulePaths(modulePath), [
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])
      })
    })

    describe('when the path is outside the resources path', () => {
      it('includes paths outside of the resources path', () => {
        const modulePath = path.resolve('/foo')
        assert.deepStrictEqual(Module._nodeModulePaths(modulePath), [
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
          show: false,
          webPreferences: {
            nodeIntegration: true
          }
        })
      })

      afterEach(async () => {
        await closeWindow(w)
        w = null
      })

      it('searches for module under app directory', async () => {
        w.loadURL('about:blank')
        const result = await w.webContents.executeJavaScript('typeof require("q").when')
        assert.strictEqual(result, 'function')
      })
    })
  })
})
