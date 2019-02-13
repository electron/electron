const chai = require('chai')
const dirtyChai = require('dirty-chai')
const http = require('http')
const path = require('path')
const { emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')
const { BrowserWindow } = require('electron').remote

const { expect } = chai
chai.use(dirtyChai)

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
    it('succeeds when devtools is already open', done => {
      w.webContents.on('did-finish-load', () => {
        w.webContents.openDevTools()
        try {
          w.webContents.debugger.attach()
        } catch (err) {
          done(`unexpected error : ${err}`)
        }
        expect(w.webContents.debugger.isAttached()).to.be.true()
        done()
      })
      w.webContents.loadFile(path.join(fixtures, 'pages', 'a.html'))
    })

    it('fails when protocol version is not supported', done => {
      try {
        w.webContents.debugger.attach('2.0')
      } catch (err) {
        expect(w.webContents.debugger.isAttached()).to.be.false()
        done()
      }
    })

    it('attaches when no protocol version is specified', done => {
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }
      expect(w.webContents.debugger.isAttached()).to.be.true()
      done()
    })
  })

  describe('debugger.detach', () => {
    it('fires detach event', (done) => {
      w.webContents.debugger.on('detach', (e, reason) => {
        expect(reason).to.equal('target closed')
        expect(w.webContents.debugger.isAttached()).to.be.false()
        done()
      })

      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }
      w.webContents.debugger.detach()
    })

    it('doesn\'t disconnect an active devtools session', done => {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        return done(`unexpected error : ${err}`)
      }
      w.webContents.openDevTools()
      w.webContents.once('devtools-opened', () => {
        w.webContents.debugger.detach()
      })
      w.webContents.debugger.on('detach', (e, reason) => {
        expect(w.webContents.debugger.isAttached()).to.be.false()
        expect(w.devToolsWebContents.isDestroyed()).to.be.false()
        done()
      })
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

    it('returns response', async () => {
      w.webContents.loadURL('about:blank')
      w.webContents.debugger.attach()

      const params = { 'expression': '4+2' }
      const res = await w.webContents.debugger.sendCommand('Runtime.evaluate', params)

      expect(res.wasThrown).to.be.undefined()
      expect(res.result.value).to.equal(6)

      w.webContents.debugger.detach()
    })

    // TODO(miniak): remove when promisification is complete
    it('returns response (callback)', done => {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        return done(`unexpected error : ${err}`)
      }

      const callback = (err, res) => {
        expect(err).to.be.null()
        expect(res.wasThrown).to.be.undefined()
        expect(res.result.value).to.equal(6)

        w.webContents.debugger.detach()
        done()
      }

      const params = { 'expression': '4+2' }
      w.webContents.debugger.sendCommand('Runtime.evaluate', params, callback)
    })

    it('returns response when devtools is opened', async () => {
      w.webContents.loadURL('about:blank')
      w.webContents.debugger.attach()

      w.webContents.openDevTools()
      await emittedOnce(w.webContents, 'devtools-opened')

      const params = { 'expression': '4+2' }
      const res = await w.webContents.debugger.sendCommand('Runtime.evaluate', params)

      expect(res.wasThrown).to.be.undefined()
      expect(res.result.value).to.equal(6)

      w.webContents.debugger.detach()
    })

    // TODO(miniak): remove when promisification is complete
    it('returns response when devtools is opened (callback)', done => {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        return done(`unexpected error : ${err}`)
      }
      const callback = (err, res) => {
        expect(err).to.be.null()
        expect(res.wasThrown).to.be.undefined()
        expect(res.result.value).to.equal(6)
        w.webContents.debugger.detach()
        done()
      }
      w.webContents.openDevTools()
      w.webContents.once('devtools-opened', () => {
        const params = { 'expression': '4+2' }
        w.webContents.debugger.sendCommand('Runtime.evaluate', params, callback)
      })
    })

    it('fires message event', done => {
      const url = process.platform !== 'win32'
        ? `file://${path.join(fixtures, 'pages', 'a.html')}`
        : `file:///${path.join(fixtures, 'pages', 'a.html').replace(/\\/g, '/')}`
      w.webContents.loadFile(path.join(fixtures, 'pages', 'a.html'))

      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }

      w.webContents.debugger.on('message', (e, method, params) => {
        if (method === 'Console.messageAdded') {
          expect(params.message.level).to.equal('log')
          expect(params.message.url).to.equal(url)
          expect(params.message.text).to.equal('a')

          w.webContents.debugger.detach()
          done()
        }
      })
      w.webContents.debugger.sendCommand('Console.enable')
    })

    it('returns error message when command fails', async () => {
      w.webContents.loadURL('about:blank')
      w.webContents.debugger.attach()

      const promise = w.webContents.debugger.sendCommand('Test')
      await expect(promise).to.be.eventually.rejectedWith(Error, "'Test' wasn't found")

      w.webContents.debugger.detach()
    })

    // TODO(miniak): remove when promisification is complete
    it('returns error message when command fails (callback)', done => {
      w.webContents.loadURL('about:blank')
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }

      w.webContents.debugger.sendCommand('Test', (err, res) => {
        expect(err).to.be.an.instanceOf(Error).with.property('message', "'Test' wasn't found")
        w.webContents.debugger.detach()
        done()
      })
    })

    it('handles valid unicode characters in message', (done) => {
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }

      w.webContents.debugger.on('message', (event, method, params) => {
        if (method === 'Network.loadingFinished') {
          w.webContents.debugger.sendCommand('Network.getResponseBody', {
            requestId: params.requestId
          }, (_, data) => {
            expect(data.body).to.equal('\u0024')
            done()
          })
        }
      })

      server = http.createServer((req, res) => {
        res.setHeader('Content-Type', 'text/plain; charset=utf-8')
        res.end('\u0024')
      })

      server.listen(0, '127.0.0.1', () => {
        w.webContents.debugger.sendCommand('Network.enable')
        w.loadURL(`http://127.0.0.1:${server.address().port}`)
      })
    })

    it('does not crash for invalid unicode characters in message', (done) => {
      try {
        w.webContents.debugger.attach()
      } catch (err) {
        done(`unexpected error : ${err}`)
      }

      w.webContents.debugger.on('message', (event, method, params) => {
        // loadingFinished indicates that page has been loaded and it did not
        // crash because of invalid UTF-8 data
        if (method === 'Network.loadingFinished') {
          done()
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
