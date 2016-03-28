const assert = require('assert')
const path = require('path')
const BrowserWindow = require('electron').remote.BrowserWindow

describe('debugger module', function () {
  var fixtures = path.resolve(__dirname, 'fixtures')
  var w = null

  beforeEach(function () {
    if (w != null) {
      w.destroy()
    }
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400
    })
  })

  afterEach(function () {
    if (w != null) {
      w.destroy()
    }
    w = null
  })

  describe('debugger.attach', function () {
    it('fails when devtools is already open', function (done) {
      w.webContents.on('did-finish-load', function () {
        w.webContents.openDevTools()
        try {
          w.webContents.debugger.attach()
        } catch (err) {
          assert(w.webContents.debugger.isAttached())
          done()
        }
      })
      w.webContents.loadURL('file://' + path.join(fixtures, 'pages', 'a.html'))
    })

    it('fails when protocol version is not supported', function (done) {
      try {
        w.webContents.debugger.attach('2.0')
      } catch (err) {
        assert(!w.webContents.debugger.isAttached())
        done()
      }
    })

    it('attaches when no protocol version is specified', function (done) {
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done('unexpected error : ' + err)
      }
      assert(w.webContents.debugger.isAttached())
      done()
    })
  })

  describe('debugger.detach', function () {
    it('fires detach event', function (done) {
      w.webContents.debugger.on('detach', function (e, reason) {
        assert.equal(reason, 'target closed')
        assert(!w.webContents.debugger.isAttached())
        done()
      })
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done('unexpected error : ' + err)
      }
      w.webContents.debugger.detach()
    })
  })

  describe('debugger.sendCommand', function () {
    it('retuns response', function (done) {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done('unexpected error : ' + err)
      }
      var callback = function (err, res) {
        assert(!res.wasThrown)
        assert.equal(res.result.value, 6)
        w.webContents.debugger.detach()
        done()
      }
      const params = {
        'expression': '4+2',
      }
      w.webContents.debugger.sendCommand('Runtime.evaluate', params, callback)
    })

    it('fires message event', function (done) {
      var url = process.platform != 'win32' ?
        'file://' + path.join(fixtures, 'pages', 'a.html') :
        'file:///' + path.join(fixtures, 'pages', 'a.html').replace(/\\/g, '/')
      w.webContents.loadURL(url)
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done('unexpected error : ' + err)
      }
      w.webContents.debugger.on('message', function (e, method, params) {
        if (method == 'Console.messageAdded') {
          assert.equal(params.message.type, 'log')
          assert.equal(params.message.url, url)
          assert.equal(params.message.text, 'a')
          w.webContents.debugger.detach()
          done()
        }
      })
      w.webContents.debugger.sendCommand('Console.enable')
    })

    it('returns error message when command fails', function (done) {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done('unexpected error : ' + err)
      }
      w.webContents.debugger.sendCommand('Test', function (err) {
        assert.equal(err.message, "'Test' wasn't found")
        w.webContents.debugger.detach()
        done()
      })
    })
  })
})
