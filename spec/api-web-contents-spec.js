'use strict'

const assert = require('assert')
const path = require('path')
const {closeWindow} = require('./window-helpers')

const {ipcRenderer, remote} = require('electron')
const {BrowserWindow, webContents, ipcMain} = remote

const isCi = remote.getGlobal('isCi')

describe('webContents module', function () {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w

  beforeEach(function () {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        backgroundThrottling: false
      }
    })
  })

  afterEach(function () {
    return closeWindow(w).then(function () { w = null })
  })

  describe('getAllWebContents() API', function () {
    it('returns an array of web contents', function (done) {
      w.webContents.on('devtools-opened', function () {
        const all = webContents.getAllWebContents().sort(function (a, b) {
          return a.getId() - b.getId()
        })

        assert.ok(all.length >= 4)
        assert.equal(all[0].getType(), 'window')
        assert.equal(all[all.length - 2].getType(), 'remote')
        assert.equal(all[all.length - 1].getType(), 'webview')

        done()
      })

      w.loadURL('file://' + path.join(fixtures, 'pages', 'webview-zoom-factor.html'))
      w.webContents.openDevTools()
    })
  })

  describe('getFocusedWebContents() API', function () {
    it('returns the focused web contents', function (done) {
      if (isCi) return done()

      const specWebContents = remote.getCurrentWebContents()
      assert.equal(specWebContents.getId(), webContents.getFocusedWebContents().getId())

      specWebContents.once('devtools-opened', function () {
        assert.equal(specWebContents.devToolsWebContents.getId(), webContents.getFocusedWebContents().getId())
        specWebContents.closeDevTools()
      })

      specWebContents.once('devtools-closed', function () {
        assert.equal(specWebContents.getId(), webContents.getFocusedWebContents().getId())
        done()
      })

      specWebContents.openDevTools()
    })

    it('does not crash when called on a detached dev tools window', function (done) {
      const specWebContents = w.webContents

      specWebContents.once('devtools-opened', function () {
        assert.doesNotThrow(function () {
          webContents.getFocusedWebContents()
        })
        specWebContents.closeDevTools()
      })

      specWebContents.once('devtools-closed', function () {
        assert.doesNotThrow(function () {
          webContents.getFocusedWebContents()
        })
        done()
      })

      specWebContents.openDevTools({mode: 'detach'})
      w.inspectElement(100, 100)
    })
  })

  describe('isFocused() API', function () {
    it('returns false when the window is hidden', function () {
      BrowserWindow.getAllWindows().forEach(function (window) {
        assert.equal(!window.isVisible() && window.webContents.isFocused(), false)
      })
    })
  })

  describe('before-input-event event', () => {
    it('can prevent document keyboard events', (done) => {
      w.loadURL('file://' + path.join(__dirname, 'fixtures', 'pages', 'key-events.html'))
      w.webContents.once('did-finish-load', () => {
        ipcMain.once('keydown', (event, key) => {
          assert.equal(key, 'b')
          done()
        })

        ipcRenderer.send('prevent-next-input-event', 'a', w.webContents.id)
        w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'a'})
        w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'b'})
      })
    })

    it('has the correct properties', (done) => {
      w.loadURL('file://' + path.join(__dirname, 'fixtures', 'pages', 'base-page.html'))
      w.webContents.once('did-finish-load', () => {
        const testBeforeInput = (opts) => {
          return new Promise((resolve, reject) => {
            w.webContents.once('before-input-event', (event, input) => {
              assert.equal(input.type, opts.type)
              assert.equal(input.key, opts.key)
              assert.equal(input.code, opts.code)
              assert.equal(input.isAutoRepeat, opts.isAutoRepeat)
              assert.equal(input.shift, opts.shift)
              assert.equal(input.control, opts.control)
              assert.equal(input.alt, opts.alt)
              assert.equal(input.meta, opts.meta)
              resolve()
            })

            const modifiers = []
            if (opts.shift) modifiers.push('shift')
            if (opts.control) modifiers.push('control')
            if (opts.alt) modifiers.push('alt')
            if (opts.meta) modifiers.push('meta')
            if (opts.isAutoRepeat) modifiers.push('isAutoRepeat')

            w.webContents.sendInputEvent({
              type: opts.type,
              keyCode: opts.keyCode,
              modifiers: modifiers
            })
          })
        }

        Promise.resolve().then(() => {
          return testBeforeInput({
            type: 'keyDown',
            key: 'A',
            code: 'KeyA',
            keyCode: 'a',
            shift: true,
            control: true,
            alt: true,
            meta: true,
            isAutoRepeat: true
          })
        }).then(() => {
          return testBeforeInput({
            type: 'keyUp',
            key: '.',
            code: 'Period',
            keyCode: '.',
            shift: false,
            control: true,
            alt: true,
            meta: false,
            isAutoRepeat: false
          })
        }).then(() => {
          return testBeforeInput({
            type: 'keyUp',
            key: '!',
            code: 'Digit1',
            keyCode: '1',
            shift: true,
            control: false,
            alt: false,
            meta: true,
            isAutoRepeat: false
          })
        }).then(() => {
          return testBeforeInput({
            type: 'keyUp',
            key: 'Tab',
            code: 'Tab',
            keyCode: 'Tab',
            shift: false,
            control: true,
            alt: false,
            meta: false,
            isAutoRepeat: true
          })
        }).then(done).catch(done)
      })
    })
  })

  describe('sendInputEvent(event)', function () {
    beforeEach(function (done) {
      w.loadURL('file://' + path.join(__dirname, 'fixtures', 'pages', 'key-events.html'))
      w.webContents.once('did-finish-load', function () {
        done()
      })
    })

    it('can send keydown events', function (done) {
      ipcMain.once('keydown', function (event, key, code, keyCode, shiftKey, ctrlKey, altKey) {
        assert.equal(key, 'a')
        assert.equal(code, 'KeyA')
        assert.equal(keyCode, 65)
        assert.equal(shiftKey, false)
        assert.equal(ctrlKey, false)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'A'})
    })

    it('can send keydown events with modifiers', function (done) {
      ipcMain.once('keydown', function (event, key, code, keyCode, shiftKey, ctrlKey, altKey) {
        assert.equal(key, 'Z')
        assert.equal(code, 'KeyZ')
        assert.equal(keyCode, 90)
        assert.equal(shiftKey, true)
        assert.equal(ctrlKey, true)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'Z', modifiers: ['shift', 'ctrl']})
    })

    it('can send keydown events with special keys', function (done) {
      ipcMain.once('keydown', function (event, key, code, keyCode, shiftKey, ctrlKey, altKey) {
        assert.equal(key, 'Tab')
        assert.equal(code, 'Tab')
        assert.equal(keyCode, 9)
        assert.equal(shiftKey, false)
        assert.equal(ctrlKey, false)
        assert.equal(altKey, true)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'Tab', modifiers: ['alt']})
    })

    it('can send char events', function (done) {
      ipcMain.once('keypress', function (event, key, code, keyCode, shiftKey, ctrlKey, altKey) {
        assert.equal(key, 'a')
        assert.equal(code, 'KeyA')
        assert.equal(keyCode, 65)
        assert.equal(shiftKey, false)
        assert.equal(ctrlKey, false)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'A'})
      w.webContents.sendInputEvent({type: 'char', keyCode: 'A'})
    })

    it('can send char events with modifiers', function (done) {
      ipcMain.once('keypress', function (event, key, code, keyCode, shiftKey, ctrlKey, altKey) {
        assert.equal(key, 'Z')
        assert.equal(code, 'KeyZ')
        assert.equal(keyCode, 90)
        assert.equal(shiftKey, true)
        assert.equal(ctrlKey, true)
        assert.equal(altKey, false)
        done()
      })
      w.webContents.sendInputEvent({type: 'keyDown', keyCode: 'Z'})
      w.webContents.sendInputEvent({type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl']})
    })
  })

  it('supports inserting CSS', function (done) {
    w.loadURL('about:blank')
    w.webContents.insertCSS('body { background-repeat: round; }')
    w.webContents.executeJavaScript('window.getComputedStyle(document.body).getPropertyValue("background-repeat")', (result) => {
      assert.equal(result, 'round')
      done()
    })
  })

  it('supports inspecting an element in the devtools', function (done) {
    w.loadURL('about:blank')
    w.webContents.once('devtools-opened', function () {
      done()
    })
    w.webContents.inspectElement(10, 10)
  })

  describe('startDrag({file, icon})', () => {
    it('throws errors for a missing file or a missing/empty icon', () => {
      assert.throws(() => {
        w.webContents.startDrag({icon: path.join(__dirname, 'fixtures', 'assets', 'logo.png')})
      }, /Must specify either 'file' or 'files' option/)

      assert.throws(() => {
        w.webContents.startDrag({file: __filename})
      }, /Must specify 'icon' option/)

      if (process.platform === 'darwin') {
        assert.throws(() => {
          w.webContents.startDrag({file: __filename, icon: __filename})
        }, /Must specify non-empty 'icon' option/)
      }
    })
  })

  describe('focus()', function () {
    it('focuses the parent window', function (done) {
      ipcMain.once('answer', (event, visible, focused) => {
        assert.equal(visible, true)
        assert.equal(focused, true)
        done()
      })
      w.show()
      w.loadURL('file://' + path.join(__dirname, 'fixtures', 'pages', 'focus-web-contents.html'))
    })
  })
})
