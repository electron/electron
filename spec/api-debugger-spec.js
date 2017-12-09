const assert = require('assert')
const http = require('http')
const path = require('path')
const {closeWindow} = require('./window-helpers')
const BrowserWindow = require('electron').remote.BrowserWindow

describe('debugger module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let w = null

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    })
  })

  afterEach(() => closeWindow(w).then(() => { w = null }))

  describe('debugger.attach', () => {
    it('fails when devtools is already open', (done) => {
      w.webContents.on('did-finish-load', () => {
        w.webContents.openDevTools()
        try {
          w.webContents.debugger.attach()
        } catch (err) {
          assert(w.webContents.debugger.isAttached())
          done()
        }
      })
      w.webContents.loadURL(`file://${path.join(fixtures, 'pages', 'a.html')}`)
    })

    it('fails when protocol version is not supported', (done) => {
      try {
        w.webContents.debugger.attach('2.0')
      } catch (err) {
        assert(!w.webContents.debugger.isAttached())
        done()
      }
    })

    it('attaches when no protocol version is specified', (done) => {
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }
      assert(w.webContents.debugger.isAttached())
      done()
    })
  })

  describe('debugger.detach', () => {
    it('fires detach event', (done) => {
      w.webContents.debugger.on('detach', (e, reason) => {
        assert.equal(reason, 'target closed')
        assert(!w.webContents.debugger.isAttached())
        done()
      })
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }
      w.webContents.debugger.detach()
    })
  })

  describe('debugger.sendCommand', () => {
    let server

    afterEach(() => {
      if (server != null) {
        server.close()
        server = null
      }
    })

    it('retuns response', (done) => {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        return done('unexpected error : ' + err)
      }
      var callback = function (err, res) {
        assert(!err.message)
        assert(!res.wasThrown)
        assert.equal(res.result.value, 6)
        w.webContents.debugger.detach()
        done()
      }
      const params = {
        'expression': '4+2'
      }
      w.webContents.debugger.sendCommand('Runtime.evaluate', params, callback)
    })

    it('fires message event', (done) => {
      const url = process.platform !== 'win32'
        ? `file://${path.join(fixtures, 'pages', 'a.html')}`
        : 'file:///' + path.join(fixtures, 'pages', 'a.html').replace(/\\/g, '/')
      w.webContents.loadURL(url)
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done('unexpected error : ' + err)
      }
      w.webContents.debugger.on('message', (e, method, params) => {
        if (method === 'Console.messageAdded') {
          assert.equal(params.message.level, 'log')
          assert.equal(params.message.url, url)
          assert.equal(params.message.text, 'a')
          w.webContents.debugger.detach()
          done()
        }
      })
      w.webContents.debugger.sendCommand('Console.enable')
    })

    it('returns error message when command fails', (done) => {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }
      w.webContents.debugger.sendCommand('Test', (err) => {
        assert.equal(err.message, "'Test' wasn't found")
        w.webContents.debugger.detach()
        done()
      })
    })

    // TODO(deepak1556): Find a way to enable this spec.
    xit('handles invalid unicode characters in message', (done) => {
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }

      w.webContents.debugger.on('message', (event, method, params) => {
        if (method === 'Network.loadingFinished') {
          w.webContents.debugger.sendCommand('Network.getResponseBody', {
            requestId: params.requestId
          }, () => {
            done()
          })
        }
      })

      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8')
        res.end('\uFFFF')
      })

      server.listen(0, '127.0.0.1', () => {
        w.webContents.debugger.sendCommand('Network.enable')
        w.loadURL(`http://127.0.0.1:${server.address().port}`)
      })
    })
  })
})
