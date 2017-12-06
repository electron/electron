const assert = require('assert')
const ChildProcess = require('child_process')
const https = require('https')
const net = require('net')
const fs = require('fs')
const path = require('path')
const {ipcRenderer, remote} = require('electron')
const {closeWindow} = require('./window-helpers')

const {app, BrowserWindow, ipcMain} = remote

const isCI = remote.getGlobal('isCi')

describe('electron module', () => {
  it('does not expose internal modules to require', () => {
    assert.throws(() => {
      require('clipboard')
    }, /Cannot find module 'clipboard'/)
  })

  describe('require("electron")', () => {
    let window = null

    beforeEach(() => {
      window = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
    })

    afterEach(() => {
      return closeWindow(window).then(() => { window = null })
    })

    it('always returns the internal electron module', (done) => {
      ipcMain.once('answer', () => done())
      window.loadURL(`file://${path.join(__dirname, 'fixtures', 'api', 'electron-module-app', 'index.html')}`)
    })
  })
})

describe('app module', () => {
  let server, secureUrl
  const certPath = path.join(__dirname, 'fixtures', 'certificates')

  before(() => {
    const options = {
      key: fs.readFileSync(path.join(certPath, 'server.key')),
      cert: fs.readFileSync(path.join(certPath, 'server.pem')),
      ca: [
        fs.readFileSync(path.join(certPath, 'rootCA.pem')),
        fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
      ],
      requestCert: true,
      rejectUnauthorized: false
    }

    server = https.createServer(options, (req, res) => {
      if (req.client.authorized) {
        res.writeHead(200)
        res.end('<title>authorized</title>')
      } else {
        res.writeHead(401)
        res.end('<title>denied</title>')
      }
    })

    server.listen(0, '127.0.0.1', () => {
      const port = server.address().port
      secureUrl = `https://127.0.0.1:${port}`
    })
  })

  after(() => {
    server.close()
  })

  describe('app.getVersion()', () => {
    it('returns the version field of package.json', () => {
      assert.equal(app.getVersion(), '0.1.0')
    })
  })

  describe('app.setVersion(version)', () => {
    it('overrides the version', () => {
      assert.equal(app.getVersion(), '0.1.0')
      app.setVersion('test-version')
      assert.equal(app.getVersion(), 'test-version')
      app.setVersion('0.1.0')
    })
  })

  describe('app.getName()', () => {
    it('returns the name field of package.json', () => {
      assert.equal(app.getName(), 'Electron Test')
    })
  })

  describe('app.setName(name)', () => {
    it('overrides the name', () => {
      assert.equal(app.getName(), 'Electron Test')
      app.setName('test-name')
      assert.equal(app.getName(), 'test-name')
      app.setName('Electron Test')
    })
  })

  describe('app.getLocale()', () => {
    it('should not be empty', () => {
      assert.notEqual(app.getLocale(), '')
    })
  })

  describe('app.isInApplicationsFolder()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('should be false during tests', () => {
      assert.equal(app.isInApplicationsFolder(), false)
    })
  })

  describe('app.exit(exitCode)', () => {
    let appProcess = null

    afterEach(() => {
      if (appProcess != null) appProcess.kill()
    })

    it('emits a process exit event with the code', (done) => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      const electronPath = remote.getGlobal('process').execPath
      let output = ''
      appProcess = ChildProcess.spawn(electronPath, [appPath])
      appProcess.stdout.on('data', (data) => {
        output += data
      })
      appProcess.on('close', (code) => {
        if (process.platform !== 'win32') {
          assert.notEqual(output.indexOf('Exit event with code: 123'), -1)
        }
        assert.equal(code, 123)
        done()
      })
    })

    it('closes all windows', (done) => {
      var appPath = path.join(__dirname, 'fixtures', 'api', 'exit-closes-all-windows-app')
      var electronPath = remote.getGlobal('process').execPath
      appProcess = ChildProcess.spawn(electronPath, [appPath])
      appProcess.on('close', function (code) {
        assert.equal(code, 123)
        done()
      })
    })
  })

  describe('app.makeSingleInstance', () => {
    it('prevents the second launch of app', function (done) {
      this.timeout(120000)
      const appPath = path.join(__dirname, 'fixtures', 'api', 'singleton')
      // First launch should exit with 0.
      const first = ChildProcess.spawn(remote.process.execPath, [appPath])
      first.once('exit', (code) => {
        assert.equal(code, 0)
      })
      // Start second app when received output.
      first.stdout.once('data', () => {
        // Second launch should exit with 1.
        const second = ChildProcess.spawn(remote.process.execPath, [appPath])
        second.once('exit', (code) => {
          assert.equal(code, 1)
          done()
        })
      })
    })
  })

  describe('app.relaunch', () => {
    let server = null
    const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch'

    beforeEach((done) => {
      fs.unlink(socketPath, () => {
        server = net.createServer()
        server.listen(socketPath)
        done()
      })
    })

    afterEach((done) => {
      server.close(() => {
        if (process.platform === 'win32') {
          done()
        } else {
          fs.unlink(socketPath, () => done())
        }
      })
    })

    it('relaunches the app', function (done) {
      this.timeout(120000)

      let state = 'none'
      server.once('error', (error) => done(error))
      server.on('connection', (client) => {
        client.once('data', (data) => {
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

  describe('app.setUserActivity(type, userInfo)', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('sets the current activity', () => {
      app.setUserActivity('com.electron.testActivity', {testData: '123'})
      assert.equal(app.getCurrentActivityType(), 'com.electron.testActivity')
    })
  })

  xdescribe('app.importCertificate', () => {
    var w = null

    before(function () {
      if (process.platform !== 'linux') {
        this.skip()
      }
    })

    afterEach(() => closeWindow(w).then(() => { w = null }))

    it('can import certificate into platform cert store', (done) => {
      let options = {
        certificate: path.join(certPath, 'client.p12'),
        password: 'electron'
      }

      w = new BrowserWindow({ show: false })

      w.webContents.on('did-finish-load', () => {
        assert.equal(w.webContents.getTitle(), 'authorized')
        done()
      })

      ipcRenderer.once('select-client-certificate', (event, webContentsId, list) => {
        assert.equal(webContentsId, w.webContents.id)
        assert.equal(list.length, 1)
        assert.equal(list[0].issuerName, 'Intermediate CA')
        assert.equal(list[0].subjectName, 'Client Cert')
        assert.equal(list[0].issuer.commonName, 'Intermediate CA')
        assert.equal(list[0].subject.commonName, 'Client Cert')
        event.sender.send('client-certificate-response', list[0])
      })

      app.importCertificate(options, (result) => {
        assert(!result)
        ipcRenderer.sendSync('set-client-certificate-option', false)
        w.loadURL(secureUrl)
      })
    })
  })

  describe('BrowserWindow events', () => {
    let w = null

    afterEach(() => closeWindow(w).then(() => { w = null }))

    it('should emit browser-window-focus event when window is focused', (done) => {
      app.once('browser-window-focus', (e, window) => {
        assert.equal(w.id, window.id)
        done()
      })
      w = new BrowserWindow({ show: false })
      w.emit('focus')
    })

    it('should emit browser-window-blur event when window is blured', (done) => {
      app.once('browser-window-blur', (e, window) => {
        assert.equal(w.id, window.id)
        done()
      })
      w = new BrowserWindow({ show: false })
      w.emit('blur')
    })

    it('should emit browser-window-created event when window is created', (done) => {
      app.once('browser-window-created', (e, window) => {
        setImmediate(() => {
          assert.equal(w.id, window.id)
          done()
        })
      })
      w = new BrowserWindow({ show: false })
    })

    it('should emit web-contents-created event when a webContents is created', (done) => {
      app.once('web-contents-created', (e, webContents) => {
        setImmediate(() => {
          assert.equal(w.webContents.id, webContents.id)
          done()
        })
      })
      w = new BrowserWindow({ show: false })
    })
  })

  describe('app.setBadgeCount', () => {
    const platformIsNotSupported =
        (process.platform === 'win32') ||
        (process.platform === 'linux' && !app.isUnityRunning())
    const platformIsSupported = !platformIsNotSupported

    const expectedBadgeCount = 42
    let returnValue = null

    beforeEach(() => {
      returnValue = app.setBadgeCount(expectedBadgeCount)
    })

    after(() => {
      // Remove the badge.
      app.setBadgeCount(0)
    })

    describe('on supported platform', () => {
      before(function () {
        if (platformIsNotSupported) {
          this.skip()
        }
      })

      it('returns true', () => {
        assert.equal(returnValue, true)
      })

      it('sets a badge count', () => {
        assert.equal(app.getBadgeCount(), expectedBadgeCount)
      })
    })

    describe('on unsupported platform', () => {
      before(function () {
        if (platformIsSupported) {
          this.skip()
        }
      })

      it('returns false', () => {
        assert.equal(returnValue, false)
      })

      it('does not set a badge count', () => {
        assert.equal(app.getBadgeCount(), 0)
      })
    })
  })

  describe('app.get/setLoginItemSettings API', () => {
    const updateExe = path.resolve(path.dirname(process.execPath), '..', 'Update.exe')
    const processStartArgs = [
      '--processStart', `"${path.basename(process.execPath)}"`,
      '--process-start-args', `"--hidden"`
    ]

    before(function () {
      if (process.platform === 'linux') {
        this.skip()
      }
    })

    beforeEach(() => {
      app.setLoginItemSettings({openAtLogin: false})
      app.setLoginItemSettings({openAtLogin: false, path: updateExe, args: processStartArgs})
    })

    afterEach(() => {
      app.setLoginItemSettings({openAtLogin: false})
      app.setLoginItemSettings({openAtLogin: false, path: updateExe, args: processStartArgs})
    })

    it('returns the login item status of the app', (done) => {
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
        openAsHidden: process.platform === 'darwin' && !process.mas, // Only available on macOS
        wasOpenedAtLogin: false,
        wasOpenedAsHidden: false,
        restoreState: false
      })

      app.setLoginItemSettings({})
      // Wait because login item settings are not applied immediately in MAS build
      const delay = process.mas ? 100 : 0
      setTimeout(() => {
        assert.deepEqual(app.getLoginItemSettings(), {
          openAtLogin: false,
          openAsHidden: false,
          wasOpenedAtLogin: false,
          wasOpenedAsHidden: false,
          restoreState: false
        })
        done()
      }, delay)
    })

    it('allows you to pass a custom executable and arguments', function () {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      app.setLoginItemSettings({openAtLogin: true, path: updateExe, args: processStartArgs})

      assert.equal(app.getLoginItemSettings().openAtLogin, false)
      assert.equal(app.getLoginItemSettings({path: updateExe, args: processStartArgs}).openAtLogin, true)
    })
  })

  describe('isAccessibilitySupportEnabled API', () => {
    it('returns whether the Chrome has accessibility APIs enabled', () => {
      assert.equal(typeof app.isAccessibilitySupportEnabled(), 'boolean')
    })
  })

  describe('getPath(name)', () => {
    it('returns paths that exist', () => {
      assert.equal(fs.existsSync(app.getPath('exe')), true)
      assert.equal(fs.existsSync(app.getPath('home')), true)
      assert.equal(fs.existsSync(app.getPath('temp')), true)
    })

    it('throws an error when the name is invalid', () => {
      assert.throws(() => {
        app.getPath('does-not-exist')
      }, /Failed to get 'does-not-exist' path/)
    })

    it('returns the overridden path', () => {
      app.setPath('music', __dirname)
      assert.equal(app.getPath('music'), __dirname)
    })
  })

  describe('select-client-certificate event', () => {
    let w = null

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: 'empty-certificate'
        }
      })
    })

    afterEach(() => closeWindow(w).then(() => { w = null }))

    it('can respond with empty certificate list', (done) => {
      w.webContents.on('did-finish-load', () => {
        assert.equal(w.webContents.getTitle(), 'denied')
        server.close()
        done()
      })

      ipcRenderer.sendSync('set-client-certificate-option', true)
      w.webContents.loadURL(secureUrl)
    })
  })

  describe('setAsDefaultProtocolClient(protocol, path, args)', () => {
    const protocol = 'electron-test'
    const updateExe = path.resolve(path.dirname(process.execPath), '..', 'Update.exe')
    const processStartArgs = [
      '--processStart', `"${path.basename(process.execPath)}"`,
      '--process-start-args', `"--hidden"`
    ]

    let Winreg
    let classesKey

    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      } else {
        Winreg = require('winreg')

        classesKey = new Winreg({
          hive: Winreg.HKCU,
          key: '\\Software\\Classes\\'
        })
      }
    })

    after(function (done) {
      if (process.platform !== 'win32') {
        done()
      } else {
        const protocolKey = new Winreg({
          hive: Winreg.HKCU,
          key: `\\Software\\Classes\\${protocol}`
        })

        // The last test leaves the registry dirty,
        // delete the protocol key for those of us who test at home
        protocolKey.destroy(() => done())
      }
    })

    beforeEach(() => {
      app.removeAsDefaultProtocolClient(protocol)
      app.removeAsDefaultProtocolClient(protocol, updateExe, processStartArgs)
    })

    afterEach(() => {
      app.removeAsDefaultProtocolClient(protocol)
      assert.equal(app.isDefaultProtocolClient(protocol), false)
      app.removeAsDefaultProtocolClient(protocol, updateExe, processStartArgs)
      assert.equal(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs), false)
    })

    it('sets the app as the default protocol client', () => {
      assert.equal(app.isDefaultProtocolClient(protocol), false)
      app.setAsDefaultProtocolClient(protocol)
      assert.equal(app.isDefaultProtocolClient(protocol), true)
    })

    it('allows a custom path and args to be specified', () => {
      assert.equal(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs), false)
      app.setAsDefaultProtocolClient(protocol, updateExe, processStartArgs)
      assert.equal(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs), true)
      assert.equal(app.isDefaultProtocolClient(protocol), false)
    })

    it('creates a registry entry for the protocol class', (done) => {
      app.setAsDefaultProtocolClient(protocol)

      classesKey.keys((error, keys) => {
        if (error) {
          throw error
        }

        const exists = !!keys.find((key) => key.key.includes(protocol))
        assert.equal(exists, true)

        done()
      })
    })

    it('completely removes a registry entry for the protocol class', (done) => {
      app.setAsDefaultProtocolClient(protocol)
      app.removeAsDefaultProtocolClient(protocol)

      classesKey.keys((error, keys) => {
        if (error) {
          throw error
        }

        const exists = !!keys.find((key) => key.key.includes(protocol))
        assert.equal(exists, false)

        done()
      })
    })

    it('only unsets a class registry key if it contains other data', (done) => {
      app.setAsDefaultProtocolClient(protocol)

      const protocolKey = new Winreg({
        hive: Winreg.HKCU,
        key: `\\Software\\Classes\\${protocol}`
      })

      protocolKey.set('test-value', 'REG_BINARY', '123', () => {
        app.removeAsDefaultProtocolClient(protocol)

        classesKey.keys((error, keys) => {
          if (error) {
            throw error
          }

          const exists = !!keys.find((key) => key.key.includes(protocol))
          assert.equal(exists, true)

          done()
        })
      })
    })
  })

  describe('getFileIcon() API', () => {
    const iconPath = path.join(__dirname, 'fixtures/assets/icon.ico')
    const sizes = {
      small: 16,
      normal: 32,
      large: process.platform === 'win32' ? 32 : 48
    }

    // (alexeykuzmin): `.skip()` called in `before`
    // doesn't affect nested `describe`s.
    beforeEach(function () {
      // FIXME Get these specs running on Linux CI
      if (process.platform === 'linux' && isCI) {
        this.skip()
      }
    })

    it('fetches a non-empty icon', (done) => {
      app.getFileIcon(iconPath, (err, icon) => {
        assert.equal(err, null)
        assert.equal(icon.isEmpty(), false)
        done()
      })
    })

    it('fetches normal icon size by default', (done) => {
      app.getFileIcon(iconPath, (err, icon) => {
        const size = icon.getSize()
        assert.equal(err, null)
        assert.equal(size.height, sizes.normal)
        assert.equal(size.width, sizes.normal)
        done()
      })
    })

    describe('size option', () => {
      it('fetches a small icon', (done) => {
        app.getFileIcon(iconPath, { size: 'small' }, (err, icon) => {
          const size = icon.getSize()
          assert.equal(err, null)
          assert.equal(size.height, sizes.small)
          assert.equal(size.width, sizes.small)
          done()
        })
      })

      it('fetches a normal icon', (done) => {
        app.getFileIcon(iconPath, { size: 'normal' }, function (err, icon) {
          const size = icon.getSize()
          assert.equal(err, null)
          assert.equal(size.height, sizes.normal)
          assert.equal(size.width, sizes.normal)
          done()
        })
      })

      it('fetches a large icon', function (done) {
        // macOS does not support large icons
        if (process.platform === 'darwin') {
          // FIXME(alexeykuzmin): Skip the test.
          // this.skip()
          return done()
        }

        app.getFileIcon(iconPath, { size: 'large' }, function (err, icon) {
          const size = icon.getSize()
          assert.equal(err, null)
          assert.equal(size.height, sizes.large)
          assert.equal(size.width, sizes.large)
          done()
        })
      })
    })
  })

  describe('getAppMetrics() API', () => {
    it('returns memory and cpu stats of all running electron processes', () => {
      const appMetrics = app.getAppMetrics()
      assert.ok(appMetrics.length > 0, 'App memory info object is not > 0')
      const types = []
      for (const {memory, pid, type, cpu} of appMetrics) {
        assert.ok(memory.workingSetSize > 0, 'working set size is not > 0')
        assert.ok(memory.privateBytes > 0, 'private bytes is not > 0')
        assert.ok(memory.sharedBytes > 0, 'shared bytes is not > 0')
        assert.ok(pid > 0, 'pid is not > 0')
        assert.ok(type.length > 0, 'process type is null')
        types.push(type)
        assert.equal(typeof cpu.percentCPUUsage, 'number')
        assert.equal(typeof cpu.idleWakeupsPerSecond, 'number')
      }

      if (process.platform === 'darwin') {
        assert.ok(types.includes('GPU'))
      }

      assert.ok(types.includes('Browser'))
      assert.ok(types.includes('Tab'))
    })
  })

  describe('getGPUFeatureStatus() API', () => {
    it('returns the graphic features statuses', () => {
      const features = app.getGPUFeatureStatus()
      assert.equal(typeof features.webgl, 'string')
      assert.equal(typeof features.gpu_compositing, 'string')
    })
  })

  describe('mixed sandbox option', () => {
    let appProcess = null
    let server = null
    const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-mixed-sandbox' : '/tmp/electron-mixed-sandbox'

    beforeEach(function (done) {
      // XXX(alexeykuzmin): Calling `.skip()` inside a `before` hook
      // doesn't affect nested `describe`s.
      // FIXME Get these specs running on Linux
      if (process.platform === 'linux') {
        this.skip()
      }

      fs.unlink(socketPath, () => {
        server = net.createServer()
        server.listen(socketPath)
        done()
      })
    })

    afterEach((done) => {
      if (appProcess != null) appProcess.kill()

      server.close(() => {
        if (process.platform === 'win32') {
          done()
        } else {
          fs.unlink(socketPath, () => done())
        }
      })
    })

    describe('when app.enableMixedSandbox() is called', () => {
      it('adds --enable-sandbox to render processes created with sandbox: true', (done) => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'mixed-sandbox-app')
        appProcess = ChildProcess.spawn(remote.process.execPath, [appPath])

        server.once('error', (error) => {
          done(error)
        })

        server.on('connection', (client) => {
          client.once('data', function (data) {
            const argv = JSON.parse(data)
            assert.equal(argv.sandbox.includes('--enable-sandbox'), true)
            assert.equal(argv.sandbox.includes('--no-sandbox'), false)

            assert.equal(argv.noSandbox.includes('--enable-sandbox'), false)
            assert.equal(argv.noSandbox.includes('--no-sandbox'), true)

            done()
          })
        })
      })
    })

    describe('when the app is launched with --enable-mixed-sandbox', () => {
      it('adds --enable-sandbox to render processes created with sandbox: true', (done) => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'mixed-sandbox-app')
        appProcess = ChildProcess.spawn(remote.process.execPath, [appPath, '--enable-mixed-sandbox'])

        server.once('error', (error) => {
          done(error)
        })

        server.on('connection', (client) => {
          client.once('data', function (data) {
            const argv = JSON.parse(data)
            assert.equal(argv.sandbox.includes('--enable-sandbox'), true)
            assert.equal(argv.sandbox.includes('--no-sandbox'), false)

            assert.equal(argv.noSandbox.includes('--enable-sandbox'), false)
            assert.equal(argv.noSandbox.includes('--no-sandbox'), true)

            assert.equal(argv.noSandboxDevtools, true)
            assert.equal(argv.sandboxDevtools, true)

            done()
          })
        })
      })
    })
  })

  describe('disableDomainBlockingFor3DAPIs() API', () => {
    it('throws when called after app is ready', () => {
      assert.throws(() => {
        app.disableDomainBlockingFor3DAPIs()
      }, /before app is ready/)
    })
  })
})
