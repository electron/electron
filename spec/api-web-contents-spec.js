'use strict'

const assert = require('assert')
const path = require('path')

const {remote} = require('electron')
const {BrowserWindow, webContents} = remote

var isCi = remote.getGlobal('isCi')

describe('webContents module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  var w = null

  beforeEach(function () {
    if (w != null) {
      w.destroy()
    }
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
    if (w != null) {
      w.destroy()
    }
    w = null
  })

  describe('getAllWebContents() API', function () {
    it('returns an array of web contents', function (done) {
      w.webContents.on('devtools-opened', function () {
        assert.equal(webContents.getAllWebContents().length, 4)

        assert.equal(webContents.getAllWebContents()[0].getType(), 'remote')
        assert.equal(webContents.getAllWebContents()[1].getType(), 'webview')
        assert.equal(webContents.getAllWebContents()[2].getType(), 'window')
        assert.equal(webContents.getAllWebContents()[3].getType(), 'window')

        done()
      })

      w.loadURL('file://' + path.join(fixtures, 'pages', 'webview-zoom-factor.html'))
      w.webContents.openDevTools()
    })
  })

  describe('getFocusedWebContents() API', function () {
    if (isCi) {
      return
    }

    it('returns the focused web contents', function (done) {
      var specWebContents = remote.getCurrentWebContents()
      assert.equal(specWebContents.getId(), webContents.getFocusedWebContents().getId())

      specWebContents.on('devtools-opened', function () {
        assert.equal(specWebContents.devToolsWebContents.getId(), webContents.getFocusedWebContents().getId())
        specWebContents.closeDevTools()
      })

      specWebContents.on('devtools-closed', function () {
        assert.equal(specWebContents.getId(), webContents.getFocusedWebContents().getId())
        done()
      })

      specWebContents.openDevTools()
    })
  })
})
