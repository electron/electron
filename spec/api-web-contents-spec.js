'use strict'

const assert = require('assert')
const path = require('path')

const {remote} = require('electron')
const {BrowserWindow, webContents} = remote

const isCi = remote.getGlobal('isCi')

describe('webContents module', function () {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w

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
        const all = webContents.getAllWebContents().sort(function (a, b) {
          return a.getId() - b.getId()
        })

        assert.equal(all.length, 5)
        assert.equal(all[0].getType(), 'window')
        assert.equal(all[1].getType(), 'offscreen')
        assert.equal(all[2].getType(), 'window')
        assert.equal(all[3].getType(), 'remote')
        assert.equal(all[4].getType(), 'webview')

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
  })
})
