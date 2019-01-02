const chai = require('chai')
const chaiAsPromised = require('chai-as-promised')
const dirtyChai = require('dirty-chai')
const ChildProcess = require('child_process')
const https = require('https')
const net = require('net')
const fs = require('fs')
const path = require('path')
const { ipcRenderer, remote } = require('electron')
const { emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')

const { expect } = chai
const { app, BrowserWindow, Menu, ipcMain } = remote

const isCI = remote.getGlobal('isCi')

chai.use(chaiAsPromised)
chai.use(dirtyChai)

describe('electron module', () => {
  it('does not expose internal modules to require', () => {
    expect(() => {
      require('clipboard')
    }).to.throw(/Cannot find module 'clipboard'/)
  })

  describe('require("electron")', () => {
    let window = null

    beforeEach(() => {
      window = new BrowserWindow({
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          nodeIntegration: true
        }
      })
    })

    afterEach(() => {
      return closeWindow(window).then(() => { window = null })
    })

    it('always returns the internal electron module', (done) => {
      ipcMain.once('answer', () => done())
      window.loadFile(path.join(__dirname, 'fixtures', 'api', 'electron-module-app', 'index.html'))
    })
  })
})

describe('app module', () => {
  let server, secureUrl
  const certPath = path.join(__dirname, 'fixtures', 'certificates')

  before((done) => {
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
      done()
    })
  })

  after(done => {
    server.close(() => done())
  })

  describe('app.getVersion()', () => {
    it('returns the version field of package.json', () => {
      expect(app.getVersion()).to.equal('0.1.0')
    })
  })

  describe('app.setVersion(version)', () => {
    it('overrides the version', () => {
      expect(app.getVersion()).to.equal('0.1.0')
      app.setVersion('test-version')

      expect(app.getVersion()).to.equal('test-version')
      app.setVersion('0.1.0')
    })
  })

  describe('app.getName()', () => {
    it('returns the name field of package.json', () => {
      expect(app.getName()).to.equal('Electron Test')
    })
  })

  describe('app.setName(name)', () => {
    it('overrides the name', () => {
      expect(app.getName()).to.equal('Electron Test')
      app.setName('test-name')

      expect(app.getName()).to.equal('test-name')
      app.setName('Electron Test')
    })
  })

  describe('app.getLocale()', () => {
    it('should not be empty', () => {
      expect(app.getLocale()).to.not.be.empty()
    })
  })

  describe('app.getLocaleCountryCode()', () => {
    it('should be empty or have length of two', () => {
      let expectedLength = 2
      if (isCI && process.platform === 'linux') {
        // Linux CI machines have no locale.
        expectedLength = 0
      }
      expect(app.getLocaleCountryCode()).to.be.a('string').and.have.lengthOf(expectedLength)
    })
  })

  describe('app.isPackaged', () => {
    it('should be false durings tests', () => {
      expect(app.isPackaged).to.be.false()
    })
  })

  describe('app.isInApplicationsFolder()', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('should be false during tests', () => {
      expect(app.isInApplicationsFolder()).to.be.false()
    })
  })

  describe('app.exit(exitCode)', () => {
    let appProcess = null

    afterEach(() => {
      if (appProcess != null) appProcess.kill()
    })

    it('emits a process exit event with the code', async () => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      const electronPath = remote.getGlobal('process').execPath
      let output = ''

      appProcess = ChildProcess.spawn(electronPath, [appPath])
      appProcess.stdout.on('data', data => { output += data })
      const [code] = await emittedOnce(appProcess, 'close')

      if (process.platform !== 'win32') {
        expect(output).to.include('Exit event with code: 123')
      }
      expect(code).to.equal(123)
    })

    it('closes all windows', async function () {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'exit-closes-all-windows-app')
      const electronPath = remote.getGlobal('process').execPath

      appProcess = ChildProcess.spawn(electronPath, [appPath])
      const [code, signal] = await emittedOnce(appProcess, 'close')

      expect(signal).to.equal(null, 'exit signal should be null, if you see this please tag @MarshallOfSound')
      expect(code).to.equal(123, 'exit code should be 123, if you see this please tag @MarshallOfSound')
    })

    it('exits gracefully', async function () {
      if (!['darwin', 'linux'].includes(process.platform)) {
        this.skip()
        return
      }

      const electronPath = remote.getGlobal('process').execPath
      const appPath = path.join(__dirname, 'fixtures', 'api', 'singleton')
      appProcess = ChildProcess.spawn(electronPath, [appPath])

      // Singleton will send us greeting data to let us know it's running.
      // After that, ask it to exit gracefully and confirm that it does.
      appProcess.stdout.on('data', data => appProcess.kill())
      const [code, signal] = await emittedOnce(appProcess, 'close')

      const message = `code:\n${code}\nsignal:\n${signal}`
      expect(code).to.equal(0, message)
      expect(signal).to.be.null(message)
    })
  })

  describe('app.requestSingleInstanceLock', () => {
    it('prevents the second launch of app', function (done) {
      this.timeout(120000)
      const appPath = path.join(__dirname, 'fixtures', 'api', 'singleton')
      const first = ChildProcess.spawn(remote.process.execPath, [appPath])
      first.once('exit', code => {
        expect(code).to.equal(0)
      })
      // Start second app when received output.
      first.stdout.once('data', () => {
        const second = ChildProcess.spawn(remote.process.execPath, [appPath])
        second.once('exit', code => {
          expect(code).to.equal(1)
          done()
        })
      })
    })
  })

  describe('app.relaunch', () => {
    let server = null
    const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch'

    beforeEach(done => {
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
      server.once('error', error => done(error))
      server.on('connection', client => {
        client.once('data', data => {
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
      app.setUserActivity('com.electron.testActivity', { testData: '123' })
      expect(app.getCurrentActivityType()).to.equal('com.electron.testActivity')
    })
  })

  xdescribe('app.importCertificate', () => {
    let w = null

    before(function () {
      if (process.platform !== 'linux') {
        this.skip()
      }
    })

    afterEach(() => closeWindow(w).then(() => { w = null }))

    it('can import certificate into platform cert store', done => {
      const options = {
        certificate: path.join(certPath, 'client.p12'),
        password: 'electron'
      }

      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })

      w.webContents.on('did-finish-load', () => {
        expect(w.webContents.getTitle()).to.equal('authorized')
        done()
      })

      ipcRenderer.once('select-client-certificate', (event, webContentsId, list) => {
        expect(webContentsId).to.equal(w.webContents.id)
        expect(list).to.have.lengthOf(1)

        expect(list[0]).to.deep.equal({
          issuerName: 'Intermediate CA',
          subjectName: 'Client Cert',
          issuer: { commonName: 'Intermediate CA' },
          subject: { commonName: 'Client Cert' }
        })

        event.sender.send('client-certificate-response', list[0])
      })

      app.importCertificate(options, result => {
        expect(result).toNotExist()
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
        expect(w.id).to.equal(window.id)
        done()
      })
      w = new BrowserWindow({ show: false })
      w.emit('focus')
    })

    it('should emit browser-window-blur event when window is blured', (done) => {
      app.once('browser-window-blur', (e, window) => {
        expect(w.id).to.equal(window.id)
        done()
      })
      w = new BrowserWindow({ show: false })
      w.emit('blur')
    })

    it('should emit browser-window-created event when window is created', (done) => {
      app.once('browser-window-created', (e, window) => {
        setImmediate(() => {
          expect(w.id).to.equal(window.id)
          done()
        })
      })
      w = new BrowserWindow({ show: false })
    })

    it('should emit web-contents-created event when a webContents is created', (done) => {
      app.once('web-contents-created', (e, webContents) => {
        setImmediate(() => {
          expect(w.webContents.id).to.equal(webContents.id)
          done()
        })
      })
      w = new BrowserWindow({ show: false })
    })

    it('should emit desktop-capturer-get-sources event when desktopCapturer.getSources() is invoked', (done) => {
      app.once('desktop-capturer-get-sources', (event, webContents) => {
        expect(webContents).to.equal(w.webContents)
        done()
      })
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`require('electron').desktopCapturer.getSources({ types: ['screen'] }, () => {})`)
    })

    it('should emit remote-require event when remote.require() is invoked', (done) => {
      app.once('remote-require', (event, webContents, moduleName) => {
        expect(webContents).to.equal(w.webContents)
        expect(moduleName).to.equal('test')
        done()
      })
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`require('electron').remote.require('test')`)
    })

    it('should emit remote-get-global event when remote.getGlobal() is invoked', (done) => {
      app.once('remote-get-global', (event, webContents, globalName) => {
        expect(webContents).to.equal(w.webContents)
        expect(globalName).to.equal('test')
        done()
      })
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      w.loadURL('about:blank')
      w.webContents.executeJavaScript(`require('electron').remote.getGlobal('test')`)
    })
  })

  describe('app.setBadgeCount', () => {
    const platformIsNotSupported =
        (process.platform === 'win32') ||
        (process.platform === 'linux' && !app.isUnityRunning())
    const platformIsSupported = !platformIsNotSupported

    const expectedBadgeCount = 42
    let returnValue = null

    beforeEach(() => { returnValue = app.setBadgeCount(expectedBadgeCount) })

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
        expect(returnValue).to.be.true()
      })

      it('sets a badge count', () => {
        expect(app.getBadgeCount()).to.equal(expectedBadgeCount)
      })
    })

    describe('on unsupported platform', () => {
      before(function () {
        if (platformIsSupported) {
          this.skip()
        }
      })

      it('returns false', () => {
        expect(returnValue).to.be.false()
      })

      it('does not set a badge count', () => {
        expect(app.getBadgeCount()).to.equal(0)
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
      app.setLoginItemSettings({ openAtLogin: false })
      app.setLoginItemSettings({ openAtLogin: false, path: updateExe, args: processStartArgs })
    })

    afterEach(() => {
      app.setLoginItemSettings({ openAtLogin: false })
      app.setLoginItemSettings({ openAtLogin: false, path: updateExe, args: processStartArgs })
    })

    it('sets and returns the app as a login item', done => {
      app.setLoginItemSettings({ openAtLogin: true })
      // Wait because login item settings are not applied immediately in MAS build
      const delay = process.mas ? 250 : 0
      setTimeout(() => {
        expect(app.getLoginItemSettings()).to.deep.equal({
          openAtLogin: true,
          openAsHidden: false,
          wasOpenedAtLogin: false,
          wasOpenedAsHidden: false,
          restoreState: false
        })
        done()
      }, delay)
    })

    it('adds a login item that loads in hidden mode', done => {
      app.setLoginItemSettings({ openAtLogin: true, openAsHidden: true })
      // Wait because login item settings are not applied immediately in MAS build
      const delay = process.mas ? 250 : 0
      setTimeout(() => {
        expect(app.getLoginItemSettings()).to.deep.equal({
          openAtLogin: true,
          openAsHidden: process.platform === 'darwin' && !process.mas, // Only available on macOS
          wasOpenedAtLogin: false,
          wasOpenedAsHidden: false,
          restoreState: false
        })
        done()
      }, delay)
    })

    it('correctly sets and unsets the LoginItem', function () {
      expect(app.getLoginItemSettings().openAtLogin).to.be.false()

      app.setLoginItemSettings({ openAtLogin: true })
      expect(app.getLoginItemSettings().openAtLogin).to.be.true()

      app.setLoginItemSettings({ openAtLogin: false })
      expect(app.getLoginItemSettings().openAtLogin).to.be.false()
    })

    it('correctly sets and unsets the LoginItem as hidden', function () {
      if (process.platform !== 'darwin' || process.mas) this.skip()

      expect(app.getLoginItemSettings().openAtLogin).to.be.false()
      expect(app.getLoginItemSettings().openAsHidden).to.be.false()

      app.setLoginItemSettings({ openAtLogin: true, openAsHidden: true })
      expect(app.getLoginItemSettings().openAtLogin).to.be.true()
      expect(app.getLoginItemSettings().openAsHidden).to.be.true()

      app.setLoginItemSettings({ openAtLogin: true, openAsHidden: false })
      expect(app.getLoginItemSettings().openAtLogin).to.be.true()
      expect(app.getLoginItemSettings().openAsHidden).to.be.false()
    })

    it('allows you to pass a custom executable and arguments', function () {
      if (process.platform !== 'win32') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      app.setLoginItemSettings({ openAtLogin: true, path: updateExe, args: processStartArgs })

      expect(app.getLoginItemSettings().openAtLogin).to.be.false()
      expect(app.getLoginItemSettings({
        path: updateExe,
        args: processStartArgs
      }).openAtLogin).to.be.true()
    })
  })

  describe('isAccessibilitySupportEnabled API', () => {
    it('returns whether the Chrome has accessibility APIs enabled', () => {
      expect(app.isAccessibilitySupportEnabled()).to.be.a('boolean')
    })
  })

  describe('getPath(name)', () => {
    it('returns paths that exist', () => {
      const paths = [
        fs.existsSync(app.getPath('exe')),
        fs.existsSync(app.getPath('home')),
        fs.existsSync(app.getPath('temp'))
      ]
      expect(paths).to.deep.equal([true, true, true])
    })

    it('throws an error when the name is invalid', () => {
      expect(() => {
        app.getPath('does-not-exist')
      }).to.throw(/Failed to get 'does-not-exist' path/)
    })

    it('returns the overridden path', () => {
      app.setPath('music', __dirname)
      expect(app.getPath('music')).to.equal(__dirname)
    })
  })

  describe('select-client-certificate event', () => {
    let w = null

    before(function () {
      if (process.platform === 'linux') {
        this.skip()
      }
    })

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true,
          partition: 'empty-certificate'
        }
      })
    })

    afterEach(() => closeWindow(w).then(() => { w = null }))

    it('can respond with empty certificate list', done => {
      w.webContents.on('did-finish-load', () => {
        expect(w.webContents.getTitle()).to.equal('denied')
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
      expect(app.isDefaultProtocolClient(protocol)).to.be.false()

      app.removeAsDefaultProtocolClient(protocol, updateExe, processStartArgs)
      expect(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs)).to.be.false()
    })

    it('sets the app as the default protocol client', () => {
      expect(app.isDefaultProtocolClient(protocol)).to.be.false()
      app.setAsDefaultProtocolClient(protocol)
      expect(app.isDefaultProtocolClient(protocol)).to.be.true()
    })

    it('allows a custom path and args to be specified', () => {
      expect(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs)).to.be.false()
      app.setAsDefaultProtocolClient(protocol, updateExe, processStartArgs)

      expect(app.isDefaultProtocolClient(protocol, updateExe, processStartArgs)).to.be.true()
      expect(app.isDefaultProtocolClient(protocol)).to.be.false()
    })

    it('creates a registry entry for the protocol class', (done) => {
      app.setAsDefaultProtocolClient(protocol)

      classesKey.keys((error, keys) => {
        if (error) throw error

        const exists = !!keys.find(key => key.key.includes(protocol))
        expect(exists).to.be.true()

        done()
      })
    })

    it('completely removes a registry entry for the protocol class', (done) => {
      app.setAsDefaultProtocolClient(protocol)
      app.removeAsDefaultProtocolClient(protocol)

      classesKey.keys((error, keys) => {
        if (error) throw error

        const exists = !!keys.find(key => key.key.includes(protocol))
        expect(exists).to.be.false()

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
          if (error) throw error

          const exists = !!keys.find(key => key.key.includes(protocol))
          expect(exists).to.be.true()

          done()
        })
      })
    })
  })

  describe('app launch through uri', () => {
    before(function () {
      if (process.platform !== 'win32') {
        this.skip()
      }
    })

    it('does not launch for argument following a URL', done => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      // App should exit with non 123 code.
      const first = ChildProcess.spawn(remote.process.execPath, [appPath, 'electron-test:?', 'abc'])
      first.once('exit', code => {
        expect(code).to.not.equal(123)
        done()
      })
    })

    it('launches successfully for argument following a file path', done => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      // App should exit with code 123.
      const first = ChildProcess.spawn(remote.process.execPath, [appPath, 'e:\\abc', 'abc'])
      first.once('exit', code => {
        expect(code).to.equal(123)
        done()
      })
    })

    it('launches successfully for multiple URIs following --', done => {
      const appPath = path.join(__dirname, 'fixtures', 'api', 'quit-app')
      // App should exit with code 123.
      const first = ChildProcess.spawn(remote.process.execPath, [appPath, '--', 'http://electronjs.org', 'electron-test://testdata'])
      first.once('exit', code => {
        expect(code).to.equal(123)
        done()
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

    it('fetches a non-empty icon', async () => {
      const icon = await app.getFileIcon(iconPath)
      expect(icon.isEmpty()).to.be.false()
    })

    it('fetches normal icon size by default', async () => {
      const icon = await app.getFileIcon(iconPath)
      const size = icon.getSize()

      expect(size.height).to.equal(sizes.normal)
      expect(size.width).to.equal(sizes.normal)
    })

    describe('size option', () => {
      it('fetches a small icon', async () => {
        const icon = await app.getFileIcon(iconPath, { size: 'small' })
        const size = icon.getSize()

        expect(size.height).to.equal(sizes.small)
        expect(size.width).to.equal(sizes.small)
      })

      it('fetches a normal icon', async () => {
        const icon = await app.getFileIcon(iconPath, { size: 'normal' })
        const size = icon.getSize()

        expect(size.height).to.equal(sizes.normal)
        expect(size.width).to.equal(sizes.normal)
      })

      it('fetches a large icon', async () => {
        // macOS does not support large icons
        if (process.platform === 'darwin') return

        const icon = await app.getFileIcon(iconPath, { size: 'large' })
        const size = icon.getSize()

        expect(size.height).to.equal(sizes.large)
        expect(size.width).to.equal(sizes.large)
      })
    })
  })

  describe('getAppMetrics() API', () => {
    it('returns memory and cpu stats of all running electron processes', () => {
      const appMetrics = app.getAppMetrics()
      expect(appMetrics).to.be.an('array').and.have.lengthOf.at.least(1, 'App memory info object is not > 0')

      const types = []
      for (const { pid, type, cpu } of appMetrics) {
        expect(pid).to.be.above(0, 'pid is not > 0')
        expect(type).to.be.a('string').that.is.not.empty()

        types.push(type)
        expect(cpu).to.have.own.property('percentCPUUsage').that.is.a('number')
        expect(cpu).to.have.own.property('idleWakeupsPerSecond').that.is.a('number')
      }

      if (process.platform === 'darwin') {
        expect(types).to.include('GPU')
      }

      expect(types).to.include('Browser')
      expect(types).to.include('Tab')
    })
  })

  describe('getGPUFeatureStatus() API', () => {
    it('returns the graphic features statuses', () => {
      const features = app.getGPUFeatureStatus()
      expect(features).to.have.own.property('webgl').that.is.a('string')
      expect(features).to.have.own.property('gpu_compositing').that.is.a('string')
    })
  })

  describe('getGPUInfo() API', () => {
    const appPath = path.join(__dirname, 'fixtures', 'api', 'gpu-info.js')

    const getGPUInfo = async (type) => {
      const appProcess = ChildProcess.spawn(remote.process.execPath, [appPath, type])
      let gpuInfoData = ''
      let errorData = ''
      appProcess.stdout.on('data', (data) => {
        gpuInfoData += data
      })
      appProcess.stderr.on('data', (data) => {
        errorData += data
      })
      const [exitCode] = await emittedOnce(appProcess, 'exit')
      if (exitCode === 0) {
        // return info data on successful exit
        return JSON.parse(gpuInfoData)
      } else {
        // return error if not clean exit
        return Promise.reject(new Error(errorData))
      }
    }
    const verifyBasicGPUInfo = async (gpuInfo) => {
      // Devices information is always present in the available info.
      expect(gpuInfo).to.have.own.property('gpuDevice')
        .that.is.an('array')
        .and.is.not.empty()

      const device = gpuInfo.gpuDevice[0]
      expect(device).to.be.an('object')
        .and.to.have.property('deviceId')
        .that.is.a('number')
        .not.lessThan(0)
    }

    it('succeeds with basic GPUInfo', async () => {
      const gpuInfo = await getGPUInfo('basic')
      await verifyBasicGPUInfo(gpuInfo)
    })

    it('succeeds with complete GPUInfo', async () => {
      const completeInfo = await getGPUInfo('complete')
      if (process.platform === 'linux') {
        // For linux and macOS complete info is same as basic info
        await verifyBasicGPUInfo(completeInfo)
        const basicInfo = await getGPUInfo('basic')
        expect(completeInfo).to.deep.equal(basicInfo)
      } else {
        // Gl version is present in the complete info.
        expect(completeInfo).to.have.own.property('auxAttributes')
          .that.is.an('object')
        expect(completeInfo.auxAttributes).to.have.own.property('glVersion')
          .that.is.a('string')
          .and.not.empty()
      }
    })

    it('fails for invalid info_type', () => {
      const invalidType = 'invalid'
      const expectedErrorMessage = "Invalid info type. Use 'basic' or 'complete'"
      return expect(app.getGPUInfo(invalidType)).to.eventually.be.rejectedWith(expectedErrorMessage)
    })
  })

  describe('sandbox options', () => {
    let appProcess = null
    let server = null
    const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-mixed-sandbox' : '/tmp/electron-mixed-sandbox'

    beforeEach(function (done) {
      if (process.platform === 'linux' && (process.arch === 'arm64' || process.arch === 'arm')) {
        // Our ARM tests are run on VSTS rather than CircleCI, and the Docker
        // setup on VSTS disallows syscalls that Chrome requires for setting up
        // sandboxing.
        // See:
        // - https://docs.docker.com/engine/security/seccomp/#significant-syscalls-blocked-by-the-default-profile
        // - https://chromium.googlesource.com/chromium/src/+/70.0.3538.124/sandbox/linux/services/credentials.cc#292
        // - https://github.com/docker/docker-ce/blob/ba7dfc59ccfe97c79ee0d1379894b35417b40bca/components/engine/profiles/seccomp/seccomp_default.go#L497
        // - https://blog.jessfraz.com/post/how-to-use-new-docker-seccomp-profiles/
        //
        // Adding `--cap-add SYS_ADMIN` or `--security-opt seccomp=unconfined`
        // to the Docker invocation allows the syscalls that Chrome needs, but
        // are probably more permissive than we'd like.
        this.skip()
      }
      fs.unlink(socketPath, () => {
        server = net.createServer()
        server.listen(socketPath)
        done()
      })
    })

    afterEach(done => {
      if (appProcess != null) appProcess.kill()

      server.close(() => {
        if (process.platform === 'win32') {
          done()
        } else {
          fs.unlink(socketPath, () => done())
        }
      })
    })

    describe('when app.enableSandbox() is called', () => {
      it('adds --enable-sandbox to all renderer processes', done => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'mixed-sandbox-app')
        appProcess = ChildProcess.spawn(remote.process.execPath, [appPath, '--app-enable-sandbox'])

        server.once('error', error => { done(error) })

        server.on('connection', client => {
          client.once('data', data => {
            const argv = JSON.parse(data)
            expect(argv.sandbox).to.include('--enable-sandbox')
            expect(argv.sandbox).to.not.include('--no-sandbox')

            expect(argv.noSandbox).to.include('--enable-sandbox')
            expect(argv.noSandbox).to.not.include('--no-sandbox')

            expect(argv.noSandboxDevtools).to.be.true()
            expect(argv.sandboxDevtools).to.be.true()

            done()
          })
        })
      })
    })

    describe('when the app is launched with --enable-sandbox', () => {
      it('adds --enable-sandbox to all renderer processes', done => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'mixed-sandbox-app')
        appProcess = ChildProcess.spawn(remote.process.execPath, [appPath, '--enable-sandbox'])

        server.once('error', error => { done(error) })

        server.on('connection', client => {
          client.once('data', data => {
            const argv = JSON.parse(data)
            expect(argv.sandbox).to.include('--enable-sandbox')
            expect(argv.sandbox).to.not.include('--no-sandbox')

            expect(argv.noSandbox).to.include('--enable-sandbox')
            expect(argv.noSandbox).to.not.include('--no-sandbox')

            expect(argv.noSandboxDevtools).to.be.true()
            expect(argv.sandboxDevtools).to.be.true()

            done()
          })
        })
      })
    })

    describe('when app.enableMixedSandbox() is called', () => {
      it('adds --enable-sandbox to renderer processes created with sandbox: true', done => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'mixed-sandbox-app')
        appProcess = ChildProcess.spawn(remote.process.execPath, [appPath, '--app-enable-mixed-sandbox'])

        server.once('error', error => { done(error) })

        server.on('connection', client => {
          client.once('data', data => {
            const argv = JSON.parse(data)
            expect(argv.sandbox).to.include('--enable-sandbox')
            expect(argv.sandbox).to.not.include('--no-sandbox')

            expect(argv.noSandbox).to.not.include('--enable-sandbox')
            expect(argv.noSandbox).to.include('--no-sandbox')

            expect(argv.noSandboxDevtools).to.be.true()
            expect(argv.sandboxDevtools).to.be.true()

            done()
          })
        })
      })
    })

    describe('when the app is launched with --enable-mixed-sandbox', () => {
      it('adds --enable-sandbox to renderer processes created with sandbox: true', done => {
        const appPath = path.join(__dirname, 'fixtures', 'api', 'mixed-sandbox-app')
        appProcess = ChildProcess.spawn(remote.process.execPath, [appPath, '--enable-mixed-sandbox'])

        server.once('error', error => { done(error) })

        server.on('connection', client => {
          client.once('data', data => {
            const argv = JSON.parse(data)
            expect(argv.sandbox).to.include('--enable-sandbox')
            expect(argv.sandbox).to.not.include('--no-sandbox')

            expect(argv.noSandbox).to.not.include('--enable-sandbox')
            expect(argv.noSandbox).to.include('--no-sandbox')

            expect(argv.noSandboxDevtools).to.be.true()
            expect(argv.sandboxDevtools).to.be.true()

            done()
          })
        })
      })
    })
  })

  describe('disableDomainBlockingFor3DAPIs() API', () => {
    it('throws when called after app is ready', () => {
      expect(() => {
        app.disableDomainBlockingFor3DAPIs()
      }).to.throw(/before app is ready/)
    })
  })

  describe('dock.setMenu', () => {
    before(function () {
      if (process.platform !== 'darwin') {
        this.skip()
      }
    })

    it('keeps references to the menu', () => {
      app.dock.setMenu(new Menu())
      const v8Util = process.atomBinding('v8_util')
      v8Util.requestGarbageCollectionForTesting()
    })
  })

  describe('whenReady', () => {
    it('returns a Promise', () => {
      expect(app.whenReady()).to.be.a('promise')
    })

    it('becomes fulfilled if the app is already ready', () => {
      expect(app.isReady()).to.be.true()
      return expect(app.whenReady()).to.be.eventually.fulfilled
    })
  })
})
