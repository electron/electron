const assert = require('assert')
const ChildProcess = require('child_process')
const https = require('https')
const fs = require('fs')
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

  describe('app.importCertificate', function () {
    if (process.platform !== 'linux')
      return

    this.timeout(5000)

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
        res.writeHead(200);
        res.end('authorized');
      }
    })

    afterEach(function () {
      if (w != null) {
        w.destroy()
      }
      w = null
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
