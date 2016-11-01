const assert = require('assert')
const path = require('path')
const {closeWindow} = require('./window-helpers')
const {remote, webFrame} = require('electron')
const {BrowserWindow, protocol, ipcMain} = remote

describe('webFrame module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  describe('webFrame.registerURLSchemeAsPrivileged', function () {
    it('supports fetch api by default', function (done) {
      webFrame.registerURLSchemeAsPrivileged('file')
      var url = 'file://' + fixtures + '/assets/logo.png'
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

    var runNumber = 1
    function allowsCORSRequests (expected, content, done) {
      const standardScheme = remote.getGlobal('standardScheme') + runNumber
      const corsScheme = 'cors' + runNumber
      runNumber++

      const url = standardScheme + '://fake-host'
      var w = new BrowserWindow({show: false})
      after(function (done) {
        protocol.unregisterProtocol(corsScheme, function () {
          protocol.unregisterProtocol(standardScheme, function () {
            closeWindow(w).then(function () {
              w = null
              done()
            })
          })
        })
      })

      const handler = function (request, callback) {
        callback({data: content, mimeType: 'text/html'})
      }
      protocol.registerStringProtocol(standardScheme, handler, function (error) {
        if (error) return done(error)
      })

      protocol.registerStringProtocol(corsScheme, function (request, callback) {
        callback('')
      }, function (error) {
        if (error) return done(error)
        ipcMain.once('response', function (event, status) {
          assert.equal(status, expected)
          done()
        })
        w.loadURL(url)
      })
    }
  })
})
