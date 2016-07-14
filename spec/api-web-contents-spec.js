'use strict'

const assert = require('assert')
const path = require('path')

const {BrowserWindow, webContents} = require('electron').remote

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
})
