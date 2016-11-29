const assert = require('assert')
const ChildProcess = require('child_process')
const https = require('https')
const net = require('net')
const fs = require('fs')
const path = require('path')
const {remote} = require('electron')
const {closeWindow} = require('./window-helpers')

const {app, BrowserWindow, ipcMain} = remote

describe('electron module', function () {
  it('does not expose internal modules to require', function () {
    assert.throws(function () {
      require('clipboard')
    }, /Cannot find module 'clipboard'/)
  })

  describe('require("electron")', function () {
    let window = null

    beforeEach(function () {
      window = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
    })

    afterEach(function () {
      return closeWindow(window).then(function () { window = null })
    })

    it('always returns the internal electron module', function (done) {
      ipcMain.once('answer', function () {
        done()
      })
      window.loadURL('file://' + path.join(__dirname, 'fixtures', 'api', 'electron-module-app', 'index.html'))
    })
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
      if (appProcess != null) appProcess.kill()
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

  describe('app.relaunch', function () {
    let server = null
    const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch'

    beforeEach(function (done) {
      fs.unlink(socketPath, () => {
        server = net.createServer()
        server.listen(socketPath)
        done()
      })
    })

    afterEach(function (done) {
      server.close(() => {
        if (process.platform === 'win32') {
          done()
        } else {
          fs.unlink(socketPath, () => {
            done()
          })
        }
      })
    })

    it('relaunches the app', function (done) {
      this.timeout(120000)

      let state = 'none'
      server.once('error', (error) => {
        done(error)
      })
      server.on('connection', (client) => {
        client.once('data', function (data) {
          if (String(data) === 'false' && state === 'none') {
            state = 'first-launch'
          } else if (String(data) === 'true' && state === 'first-launch') {
            done()
          } else {
            done(`Unexpected state: ${state}`)
          }
        })
      })

      const appPath = path.join(__dirname, 'fixtures', 'api', 'relaunch')
      ChildProcess.spawn(remote.process.execPath, [appPath])
    })
  })

  describe('app.setUserActivity(type, userInfo)', function () {
    if (process.platform !== 'darwin') {
      return
    }

    it('sets the current activity', function () {
      app.setUserActivity('com.electron.testActivity', {testData: '123'})
      assert.equal(app.getCurrentActivityType(), 'com.electron.testActivity')
    })
  })

  describe('app.importCertificate', function () {
    if (process.platform !== 'linux') return

    var w = null
    var certPath = path.join(__dirname, 'fixtures', 'certificates')
    var options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    }

    var server = https.createServer(options, function (req, res) {
      if (req.client.authorized) {
        res.writeHead(200)
        res.end('authorized')
      }
    })

    afterEach(function () {
      return closeWindow(w).then(function () { w = null })
    })

    it('can import certificate into platform cert store', function (done) {
      let options = {
        certificate: path.join(certPath, 'client.p12'),
        password: 'electron'
      }

      w = new BrowserWindow({
        show: false
      })

      w.webContents.on('did-finish-load', function () {
        server.close()
        done()
      })

      app.on('select-client-certificate', function (event, webContents, url, list, callback) {
        assert.equal(list.length, 1)
        assert.equal(list[0].issuerName, 'Intermediate CA')
        assert.equal(list[0].subjectName, 'Client Cert')
        assert.equal(list[0].issuer.commonName, 'Intermediate CA')
        assert.equal(list[0].subject.commonName, 'Client Cert')
        callback(list[0])
      })

      app.importCertificate(options, function (result) {
        assert(!result)
        server.listen(0, '127.0.0.1', function () {
          var port = server.address().port
          w.loadURL(`https://127.0.0.1:${port}`)
        })
      })
    })
  })

  describe('BrowserWindow events', function () {
    var w = null

    afterEach(function () {
      return closeWindow(w).then(function () { w = null })
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
    })

    it('should emit web-contents-created event when a webContents is created', function (done) {
      app.once('web-contents-created', function (e, webContents) {
        setImmediate(function () {
          assert.equal(w.webContents.id, webContents.id)
          done()
        })
      })
      w = new BrowserWindow({
        show: false
      })
    })
  })

  describe('app.setBadgeCount API', function () {
    const shouldFail = process.platform === 'win32' ||
                       (process.platform === 'linux' && !app.isUnityRunning())

    afterEach(function () {
      app.setBadgeCount(0)
    })

    it('returns false when failed', function () {
      assert.equal(app.setBadgeCount(42), !shouldFail)
    })

    it('should set a badge count', function () {
      app.setBadgeCount(42)
      assert.equal(app.getBadgeCount(), shouldFail ? 0 : 42)
    })
  })

  describe('app.get/setLoginItemSettings API', function () {
    if (process.platform === 'linux') return

    beforeEach(function () {
      app.setLoginItemSettings({openAtLogin: false})
    })

    afterEach(function () {
      app.setLoginItemSettings({openAtLogin: false})
    })

    it('returns the login item status of the app', function () {
      app.setLoginItemSettings({openAtLogin: true})
      assert.deepEqual(app.getLoginItemSettings(), {
        openAtLogin: true,
        openAsHidden: false,
        wasOpenedAtLogin: false,
        wasOpenedAsHidden: false,
        restoreState: false
      })

      app.setLoginItemSettings({openAtLogin: true, openAsHidden: true})
      assert.deepEqual(app.getLoginItemSettings(), {
        openAtLogin: true,
        openAsHidden: process.platform === 'darwin', // Only available on macOS
        wasOpenedAtLogin: false,
        wasOpenedAsHidden: false,
        restoreState: false
      })

      app.setLoginItemSettings({})
      assert.deepEqual(app.getLoginItemSettings(), {
        openAtLogin: false,
        openAsHidden: false,
        wasOpenedAtLogin: false,
        wasOpenedAsHidden: false,
        restoreState: false
      })
    })
  })

  describe('isAccessibilitySupportEnabled API', function () {
    it('returns whether the Chrome has accessibility APIs enabled', function () {
      assert.equal(typeof app.isAccessibilitySupportEnabled(), 'boolean')
    })
  })

  describe('getPath(name)', function () {
    it('returns paths that exist', function () {
      assert.equal(fs.existsSync(app.getPath('exe')), true)
      assert.equal(fs.existsSync(app.getPath('home')), true)
      assert.equal(fs.existsSync(app.getPath('temp')), true)
    })

    it('throws an error when the name is invalid', function () {
      assert.throws(function () {
        app.getPath('does-not-exist')
      }, /Failed to get 'does-not-exist' path/)
    })

    it('returns the overridden path', function () {
      app.setPath('music', __dirname)
      assert.equal(app.getPath('music'), __dirname)
    })
  })
})
