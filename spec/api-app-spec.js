const assert = require('assert')
const ChildProcess = require('child_process')
const path = require('path')
const remote = require('electron').remote

const app = remote.require('electron').app
const BrowserWindow = remote.require('electron').BrowserWindow

describe('electron module', function () {
  it('allows old style require by default', function () {
    require('shell')
  })

  it('can prevent exposing internal modules to require', function (done) {
    const electron = require('electron')
    const clipboard = require('clipboard')
    assert.equal(typeof clipboard, 'object')
    electron.hideInternalModules()
    try {
      require('clipboard')
    } catch (err) {
      assert.equal(err.message, "Cannot find module 'clipboard'")
      done()
    }
  })
})

describe('app module', function () {
  describe('app.getVersion()', function () {
    it('returns the version field of package.json', function () {
      assert.equal(app.getVersion(), '0.1.0')
    })
  })

  describe('app.setVersion(version)', function () {
    it('overrides the version', function () {
      assert.equal(app.getVersion(), '0.1.0')
      app.setVersion('test-version')
      assert.equal(app.getVersion(), 'test-version')
      app.setVersion('0.1.0')
    })
  })

  describe('app.getName()', function () {
    it('returns the name field of package.json', function () {
      assert.equal(app.getName(), 'Electron Test')
    })
  })

  describe('app.setName(name)', function () {
    it('overrides the name', function () {
      assert.equal(app.getName(), 'Electron Test')
      app.setName('test-name')
      assert.equal(app.getName(), 'test-name')
      app.setName('Electron Test')
    })
  })

  describe('app.getLocale()', function () {
    it('should not be empty', function () {
      assert.notEqual(app.getLocale(), '')
    })
  })

  describe('app.exit(exitCode)', function () {
    var appProcess = null

    afterEach(function () {
      appProcess != null ? appProcess.kill() : void 0
    })

    it('emits a process exit event with the code', function (done) {
      var appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      var electronPath = remote.getGlobal('process').execPath
      var output = ''
      appProcess = ChildProcess.spawn(electronPath, [appPath])
      appProcess.stdout.on('data', function (data) {
        output += data
      })
      appProcess.on('close', function (code) {
        if (process.platform !== 'win32') {
          assert.notEqual(output.indexOf('Exit event with code: 123'), -1)
        }
        assert.equal(code, 123)
        done()
      })
    })
  })

  describe('BrowserWindow events', function () {
    var w = null

    afterEach(function () {
      if (w != null) {
        w.destroy()
      }
      w = null
    })

    it('should emit browser-window-focus event when window is focused', function (done) {
      app.once('browser-window-focus', function (e, window) {
        assert.equal(w.id, window.id)
        done()
      })
      w = new BrowserWindow({
        show: false
      })
      w.emit('focus')
    })

    it('should emit browser-window-blur event when window is blured', function (done) {
      app.once('browser-window-blur', function (e, window) {
        assert.equal(w.id, window.id)
        done()
      })
      w = new BrowserWindow({
        show: false
      })
      w.emit('blur')
    })

    it('should emit browser-window-created event when window is created', function (done) {
      app.once('browser-window-created', function (e, window) {
        setImmediate(function () {
          assert.equal(w.id, window.id)
          done()
        })
      })
      w = new BrowserWindow({
        show: false
      })
      w.emit('blur')
    })
  })
})
