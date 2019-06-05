const chai = require('chai')
const dirtyChai = require('dirty-chai')

const Module = require('module')
const path = require('path')
const fs = require('fs')
const { remote } = require('electron')
const { BrowserWindow } = remote
const { closeWindow } = require('./window-helpers')
const features = process.electronBinding('features')

const { expect } = chai
chai.use(dirtyChai)

const nativeModulesEnabled = remote.getGlobal('nativeModulesEnabled')

describe('modules support', () => {
  const fixtures = path.join(__dirname, 'fixtures')

  describe('third-party module', () => {
    (nativeModulesEnabled ? describe : describe.skip)('echo', () => {
      it('can be required in renderer', () => {
        require('echo')
      })

      it('can be required in node binary', function (done) {
        if (!features.isRunAsNodeEnabled()) {
          this.skip()
          done()
        }

        const echo = path.join(fixtures, 'module', 'echo.js')
        const child = require('child_process').fork(echo)
        child.on('message', (msg) => {
          expect(msg).to.equal('ok')
          done()
        })
      })

      if (process.platform === 'win32') {
        it('can be required if electron.exe is renamed', () => {
          const { execPath } = remote.process
          const testExecPath = path.join(path.dirname(execPath), 'test.exe')
          fs.copyFileSync(execPath, testExecPath)
          try {
            const fixture = path.join(fixtures, 'module', 'echo-renamed.js')
            expect(fs.existsSync(fixture)).to.be.true()
            const child = require('child_process').spawnSync(testExecPath, [fixture])
            expect(child.status).to.equal(0)
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
        expect(libm.ceil(1.5)).to.equal(2)
      })
    })

    describe('q', () => {
      const Q = require('q')
      describe('Q.when', () => {
        it('emits the fullfil callback', (done) => {
          Q(true).then((val) => {
            expect(val).to.be.true()
            done()
          })
        })
      })
    })

    describe('coffeescript', () => {
      it('can be registered and used to require .coffee files', () => {
        expect(() => {
          require('coffeescript').register()
        }).to.not.throw()
        expect(require('./fixtures/module/test.coffee')).to.be.true()
      })
    })
  })

  describe('global variables', () => {
    describe('process', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-process')).to.equal('declared process')
      })
    })

    describe('global', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-global')).to.equal('declared global')
      })
    })

    describe('Buffer', () => {
      it('can be declared in a module', () => {
        expect(require('./fixtures/module/declare-buffer')).to.equal('declared Buffer')
      })
    })
  })

  describe('Module._nodeModulePaths', () => {
    describe('when the path is inside the resources path', () => {
      it('does not include paths outside of the resources path', () => {
        let modulePath = process.resourcesPath
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = process.resourcesPath + '-foo'
        const nodeModulePaths = Module._nodeModulePaths(modulePath)
        expect(nodeModulePaths).to.include(path.join(modulePath, 'node_modules'))
        expect(nodeModulePaths).to.include(path.join(modulePath, '..', 'node_modules'))

        modulePath = path.join(process.resourcesPath, 'foo')
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo')
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'bar')
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])

        modulePath = path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar')
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules', 'bar', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules', 'foo', 'node_modules'),
          path.join(process.resourcesPath, 'node_modules')
        ])
      })
    })

    describe('when the path is outside the resources path', () => {
      it('includes paths outside of the resources path', () => {
        const modulePath = path.resolve('/foo')
        expect(Module._nodeModulePaths(modulePath)).to.deep.equal([
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
        expect(result).to.equal('function')
      })
    })
  })
})
