const assert = require('assert')
const fs = require('fs')
const path = require('path')
const os = require('os')
const http = require('http')
const { shell, remote } = require('electron')
const { BrowserWindow } = remote

const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')

describe('shell module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  const shortcutOptions = {
    target: 'C:\\target',
    description: 'description',
    cwd: 'cwd',
    args: 'args',
    appUserModelId: 'appUserModelId',
    icon: 'icon',
    iconIndex: 1
  }

  describe('shell.openExternal()', () => {
    let envVars = {}
    let w

    beforeEach(function () {
      envVars = {
        display: process.env.DISPLAY,
        de: process.env.DE,
        browser: process.env.BROWSER
      }
    })

    afterEach(async () => {
      await closeWindow(w)
      w = null
      // reset env vars to prevent side effects
      if (process.platform === 'linux') {
        process.env.DE = envVars.de
        process.env.BROWSER = envVars.browser
        process.env.DISPLAY = envVars.display
      }
    })

    it('opens an external link asynchronously', async () => {
      let url = 'http://127.0.0.1'
      let requestReceived
      if (process.platform === 'linux') {
        process.env.BROWSER = '/bin/true'
        process.env.DE = 'generic'
        process.env.DISPLAY = ''
        requestReceived = Promise.resolve()
      } else if (process.platform === 'darwin') {
        // On the Mac CI machines, Safari tries to ask for a password to the
        // code signing keychain we set up to test code signing (see
        // https://github.com/electron/electron/pull/19969#issuecomment-526278890),
        // so use a blur event as a crude proxy.
        w = new BrowserWindow({ show: true })
        requestReceived = emittedOnce(w, 'blur')
      } else {
        const server = http.createServer((req, res) => {
          res.end()
        })
        await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
        requestReceived = new Promise(resolve => server.on('connection', () => resolve()))
        url = `http://127.0.0.1:${server.address().port}`
      }

      await Promise.all([
        shell.openExternal(url),
        requestReceived
      ])
    })

    it('opens an external link synchronously', () => {
      const url = 'http://www.example.com'
      if (process.platform === 'linux') {
        process.env.DE = 'generic'
        process.env.DE = '/bin/true'
        process.env.DISPLAY = ''
      }

      const success = shell.openExternalSync(url)
      assert.strictEqual(true, success)
    })
  })

  describe('shell.readShortcutLink(shortcutPath)', () => {
    beforeEach(function () {
      if (process.platform !== 'win32') this.skip()
    })

    it('throws when failed', () => {
      assert.throws(() => {
        shell.readShortcutLink('not-exist')
      }, /Failed to read shortcut link/)
    })

    it('reads all properties of a shortcut', () => {
      const shortcut = shell.readShortcutLink(path.join(fixtures, 'assets', 'shortcut.lnk'))
      assert.deepStrictEqual(shortcut, shortcutOptions)
    })
  })

  describe('shell.writeShortcutLink(shortcutPath[, operation], options)', () => {
    beforeEach(function () {
      if (process.platform !== 'win32') this.skip()
    })

    const tmpShortcut = path.join(os.tmpdir(), `${Date.now()}.lnk`)

    afterEach(() => {
      fs.unlinkSync(tmpShortcut)
    })

    it('writes the shortcut', () => {
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, { target: 'C:\\' }), true)
      assert.strictEqual(fs.existsSync(tmpShortcut), true)
    })

    it('correctly sets the fields', () => {
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, shortcutOptions), true)
      assert.deepStrictEqual(shell.readShortcutLink(tmpShortcut), shortcutOptions)
    })

    it('updates the shortcut', () => {
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, 'update', shortcutOptions), false)
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, 'create', shortcutOptions), true)
      assert.deepStrictEqual(shell.readShortcutLink(tmpShortcut), shortcutOptions)
      const change = { target: 'D:\\' }
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, 'update', change), true)
      assert.deepStrictEqual(shell.readShortcutLink(tmpShortcut), Object.assign(shortcutOptions, change))
    })

    it('replaces the shortcut', () => {
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, 'replace', shortcutOptions), false)
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, 'create', shortcutOptions), true)
      assert.deepStrictEqual(shell.readShortcutLink(tmpShortcut), shortcutOptions)
      const change = {
        target: 'D:\\',
        description: 'description2',
        cwd: 'cwd2',
        args: 'args2',
        appUserModelId: 'appUserModelId2',
        icon: 'icon2',
        iconIndex: 2
      }
      assert.strictEqual(shell.writeShortcutLink(tmpShortcut, 'replace', change), true)
      assert.deepStrictEqual(shell.readShortcutLink(tmpShortcut), change)
    })
  })
})
