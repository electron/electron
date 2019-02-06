const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const { closeWindow } = require('./window-helpers')
const { remote, webFrame } = require('electron')
const { BrowserWindow, protocol, ipcMain } = remote
const { emittedOnce } = require('./events-helpers')

const { expect } = chai
chai.use(dirtyChai)

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('webFrame module', function () {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w = null

  afterEach(function () {
    return closeWindow(w).then(function () { w = null })
  })

  describe('webFrame.registerURLSchemeAsPrivileged', function () {
    it('supports fetch api by default', function (done) {
      const url = 'file://' + fixtures + '/assets/logo.png'
      window.fetch(url).then(function (response) {
        assert(response.ok)
        done()
      }).catch(function (err) {
        done('unexpected error : ' + err)
      })
    })

    it('allows CORS requests by default', function (done) {
      allowsCORSRequests(200, `<html>
        <script>
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.registerURLSchemeAsPrivileged('cors1')
        fetch('cors1://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
      </html>`, done)
    })

    it('allows CORS and fetch requests when specified', function (done) {
      allowsCORSRequests(200, `<html>
        <script>
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.registerURLSchemeAsPrivileged('cors2', { supportFetchAPI: true, corsEnabled: true })
        fetch('cors2://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
      </html>`, done)
    })

    it('allows CORS and fetch requests when half-specified', function (done) {
      allowsCORSRequests(200, `<html>
        <script>
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.registerURLSchemeAsPrivileged('cors3', { supportFetchAPI: true })
        fetch('cors3://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
      </html>`, done)
    })

    it('disallows CORS, but allows fetch requests, when specified', function (done) {
      allowsCORSRequests('failed', `<html>
        <script>
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.registerURLSchemeAsPrivileged('cors4', { supportFetchAPI: true, corsEnabled: false })
        fetch('cors4://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
      </html>`, done)
    })

    it('allows CORS, but disallows fetch requests, when specified', function (done) {
      allowsCORSRequests('failed', `<html>
        <script>
        const {ipcRenderer, webFrame} = require('electron')
        webFrame.registerURLSchemeAsPrivileged('cors5', { supportFetchAPI: false, corsEnabled: true })
        fetch('cors5://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
      </html>`, done)
    })

    let runNumber = 1
    function allowsCORSRequests (expected, content, done) {
      const standardScheme = remote.getGlobal('standardScheme') + runNumber
      const corsScheme = 'cors' + runNumber
      runNumber++

      const url = standardScheme + '://fake-host'
      w = new BrowserWindow({ show: false })
      after(function (done) {
        protocol.unregisterProtocol(corsScheme, function () {
          protocol.unregisterProtocol(standardScheme, function () {
            done()
          })
        })
      })

      const handler = function (request, callback) {
        callback({ data: content, mimeType: 'text/html' })
      }
      protocol.registerStringProtocol(standardScheme, handler, function (error) {
        if (error) return done(error)
      })

      protocol.registerStringProtocol(corsScheme, function (request, callback) {
        callback('')
      }, function (error) {
        if (error) return done(error)
        ipcMain.once('response', function (event, status) {
          assert.strictEqual(status, expected)
          done()
        })
        w.loadURL(url)
      })
    }
  })

  it('supports setting the visual and layout zoom level limits', function () {
    assert.doesNotThrow(function () {
      webFrame.setVisualZoomLevelLimits(1, 50)
      webFrame.setLayoutZoomLevelLimits(0, 25)
    })
  })

  it('calls a spellcheck provider', async () => {
    w = new BrowserWindow({ show: false })
    w.loadFile(path.join(fixtures, 'pages', 'webframe-spell-check.html'))
    await emittedOnce(w.webContents, 'did-finish-load')
    w.focus()
    await w.webContents.executeJavaScript('document.querySelector("input").focus()', true)

    const spellCheckerFeedback = emittedOnce(ipcMain, 'spec-spell-check')
    const misspelledWord = 'spleling'
    for (const keyCode of [...misspelledWord, ' ']) {
      w.webContents.sendInputEvent({ type: 'char', keyCode })
    }
    const [, text] = await spellCheckerFeedback
    expect(text).to.equal(misspelledWord)
  })

  it('top is self for top frame', () => {
    expect(webFrame.top.context).to.equal(webFrame.context)
  })

  it('opener is null for top frame', () => {
    expect(webFrame.opener).to.be.null()
  })

  it('firstChild is null for top frame', () => {
    expect(webFrame.firstChild).to.be.null()
  })

  it('getFrameForSelector() does not crash when not found', () => {
    expect(webFrame.getFrameForSelector('unexist-selector')).to.be.null()
  })

  it('findFrameByName() does not crash when not found', () => {
    expect(webFrame.findFrameByName('unexist-name')).to.be.null()
  })

  it('findFrameByRoutingId() does not crash when not found', () => {
    expect(webFrame.findFrameByRoutingId(-1)).to.be.null()
  })
})
