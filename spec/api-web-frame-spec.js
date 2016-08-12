const assert = require('assert')
const path = require('path')
const {closeWindow} = require('./window-helpers')
const {remote, webFrame} = require('electron')
const {BrowserWindow, protocol, ipcMain} = remote

describe('webFrame module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  describe('webFrame.registerURLSchemeAsPrivileged', function () {
    it('supports fetch api', function (done) {
      webFrame.registerURLSchemeAsPrivileged('file')
      var url = 'file://' + fixtures + '/assets/logo.png'
      window.fetch(url).then(function (response) {
        assert(response.ok)
        done()
      }).catch(function (err) {
        done('unexpected error : ' + err)
      })
    })

    it('allows CORS requests', function (done) {
      const standardScheme = remote.getGlobal('standardScheme')
      const url = standardScheme + '://fake-host'
      const content = `<html>
                       <script>
                        const {ipcRenderer, webFrame} = require('electron')
                        webFrame.registerURLSchemeAsPrivileged('cors')
                        fetch('cors://myhost').then(function (response) {
                          ipcRenderer.send('response', response.status)
                        })
                       </script>
                       </html>`
      var w = new BrowserWindow({show: false})
      after(function (done) {
        protocol.unregisterProtocol('cors', function () {
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

      protocol.registerStringProtocol('cors', function (request, callback) {
        callback('')
      }, function (error) {
        if (error) return done(error)
        ipcMain.once('response', function (event, status) {
          assert.equal(status, 200)
          done()
        })
        w.loadURL(url)
      })
    })
  })
})
