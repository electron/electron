'use strict'

const assert = require('assert')
const path = require('path')
const {closeWindow} = require('./window-helpers')

const {remote} = require('electron')
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
      w.webContents.sendInputEvent({type: 'char', keyCode: 'Z', modifiers: ['shift', 'ctrl']})
    })
  })
})
