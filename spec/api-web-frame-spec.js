const assert = require('assert')
const chai = require('chai')
const dirtyChai = require('dirty-chai')
const path = require('path')
const { closeWindow } = require('./window-helpers')
const { remote, webFrame } = require('electron')
const { BrowserWindow, protocol, ipcMain } = remote

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

  // FIXME: Disabled with C70.
  xdescribe('webFrame.registerURLSchemeAsPrivileged', function () {
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
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true
      }
    })
    await w.loadFile(path.join(fixtures, 'pages', 'webframe-spell-check.html'))
    w.focus()
    await w.webContents.executeJavaScript('document.querySelector("input").focus()', true)

    const spellCheckerFeedback =
      new Promise(resolve => {
        ipcMain.on('spec-spell-check', (e, words, callback) => {
          if (words.length === 2) {
            // The promise is resolved only after this event is received twice
            // Array contains only 1 word first time and 2 the next time
            resolve([words, callback])
          }
        })
      })
    const inputText = 'spleling test '
    for (const keyCode of inputText) {
      w.webContents.sendInputEvent({ type: 'char', keyCode })
    }
    const [words, callback] = await spellCheckerFeedback
    expect(words).to.deep.equal(['spleling', 'test'])
    expect(callback).to.be.true()
  })
})
