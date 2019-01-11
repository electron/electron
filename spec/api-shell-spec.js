const assert = require('assert')
const fs = require('fs')
const path = require('path')
const os = require('os')
const { shell } = require('electron')

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

    beforeEach(function () {
      envVars = {
        display: process.env.DISPLAY,
        de: process.env.DE,
        browser: process.env.BROWSER
      }
    })

    afterEach(() => {
      // reset env vars to prevent side effects
      if (process.platform === 'linux') {
        process.env.DE = envVars.de
        process.env.BROWSER = envVars.browser
        process.env.DISPLAY = envVars.display
      }
    })

    it('opens an external link asynchronously', done => {
      const url = 'http://www.example.com'
      if (process.platform === 'linux') {
        process.env.BROWSER = '/bin/true'
        process.env.DE = 'generic'
        process.env.DISPLAY = ''
      }

      shell.openExternal(url).then(() => done())
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
