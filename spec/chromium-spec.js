const { expect } = require('chai')
const fs = require('fs')
const http = require('http')
const path = require('path')
const ws = require('ws')
const url = require('url')
const ChildProcess = require('child_process')
const { ipcRenderer } = require('electron')
const { emittedOnce } = require('./events-helpers')
const { resolveGetters } = require('./expect-helpers')
const features = process.electronBinding('features')

/* Most of the APIs here don't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('chromium feature', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  let listener = null

  afterEach(() => {
    if (listener != null) {
      window.removeEventListener('message', listener)
    }
    listener = null
  })

  describe('heap snapshot', () => {
    it('does not crash', function () {
      process.electronBinding('v8_util').takeHeapSnapshot()
    })
  })

  describe('navigator.webkitGetUserMedia', () => {
    it('calls its callbacks', (done) => {
      navigator.webkitGetUserMedia({
        audio: true,
        video: false
      }, () => done(),
      () => done())
    })
  })

  describe('navigator.language', () => {
    it('should not be empty', () => {
      expect(navigator.language).to.not.equal('')
    })
  })

  describe('navigator.geolocation', () => {
    before(function () {
      if (!features.isFakeLocationProviderEnabled()) {
        return this.skip()
      }
    })

    it('returns position when permission is granted', (done) => {
      navigator.geolocation.getCurrentPosition((position) => {
        expect(position).to.have.a.property('coords')
        expect(position).to.have.a.property('timestamp')
        done()
      }, (error) => {
        done(error)
      })
    })
  })

  describe('window.open', () => {
    it('returns a BrowserWindowProxy object', () => {
      const b = window.open('about:blank', '', 'show=no')
      expect(b.closed).to.be.false()
      expect(b.constructor.name).to.equal('BrowserWindowProxy')
      b.close()
    })

    it('accepts "nodeIntegration" as feature', (done) => {
      let b = null
      listener = (event) => {
        expect(event.data.isProcessGlobalUndefined).to.be.true()
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-opener-node.html`, '', 'nodeIntegration=no,show=no')
    })

    it('inherit options of parent window', (done) => {
      let b = null
      listener = (event) => {
        const width = outerWidth
        const height = outerHeight
        expect(event.data).to.equal(`size: ${width} ${height}`)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no')
    })

    it('disables node integration when it is disabled on the parent window', (done) => {
      let b = null
      listener = (event) => {
        expect(event.data.isProcessGlobalUndefined).to.be.true()
        b.close()
        done()
      }
      window.addEventListener('message', listener)

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-node-integration.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-node.html`
        },
        slashes: true
      })
      b = window.open(windowUrl, '', 'nodeIntegration=no,show=no')
    })

    it('disables the <webview> tag when it is disabled on the parent window', (done) => {
      let b = null
      listener = (event) => {
        expect(event.data.isWebViewGlobalUndefined).to.be.true()
        b.close()
        done()
      }
      window.addEventListener('message', listener)

      const windowUrl = require('url').format({
        pathname: `${fixtures}/pages/window-opener-no-webview-tag.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-webview.html`
        },
        slashes: true
      })
      b = window.open(windowUrl, '', 'webviewTag=no,nodeIntegration=yes,show=no')
    })

    it('does not override child options', (done) => {
      let b = null
      const size = {
        width: 350,
        height: 450
      }
      listener = (event) => {
        expect(event.data).to.equal(`size: ${size.width} ${size.height}`)
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-open-size.html`, '', 'show=no,width=' + size.width + ',height=' + size.height)
    })

    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.open('', { toString: null })
      }).to.throw('Cannot convert object to primitive value')

      expect(() => {
        window.open('', '', { toString: 3 })
      }).to.throw('Cannot convert object to primitive value')
    })

    it('does not throw an exception when the features include webPreferences', () => {
      let b = null
      expect(() => {
        b = window.open('', '', 'webPreferences=')
      }).to.not.throw()
      b.close()
    })
  })

  describe('window.opener', () => {
    it('is not null for window opened by window.open', (done) => {
      let b = null
      listener = (event) => {
        expect(event.data).to.equal('object')
        b.close()
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-opener.html`, '', 'show=no')
    })
  })

  describe('window.postMessage', () => {
    it('throws an exception when the targetOrigin cannot be converted to a string', () => {
      const b = window.open('')
      expect(() => {
        b.postMessage('test', { toString: null })
      }).to.throw('Cannot convert object to primitive value')
      b.close()
    })
  })

  describe('window.opener.postMessage', () => {
    it('sets source and origin correctly', (done) => {
      let b = null
      listener = (event) => {
        window.removeEventListener('message', listener)
        b.close()
        expect(event.source).to.equal(b)
        expect(event.origin).to.equal('file://')
        done()
      }
      window.addEventListener('message', listener)
      b = window.open(`file://${fixtures}/pages/window-opener-postMessage.html`, '', 'show=no')
    })

    it('supports windows opened from a <webview>', (done) => {
      const webview = new WebView()
      webview.addEventListener('console-message', (e) => {
        webview.remove()
        expect(e.message).to.equal('message')
        done()
      })
      webview.allowpopups = true
      webview.src = url.format({
        pathname: `${fixtures}/pages/webview-opener-postMessage.html`,
        protocol: 'file',
        query: {
          p: `${fixtures}/pages/window-opener-postMessage.html`
        },
        slashes: true
      })
      document.body.appendChild(webview)
    })

    describe('targetOrigin argument', () => {
      let serverURL
      let server

      beforeEach((done) => {
        server = http.createServer((req, res) => {
          res.writeHead(200)
          const filePath = path.join(fixtures, 'pages', 'window-opener-targetOrigin.html')
          res.end(fs.readFileSync(filePath, 'utf8'))
        })
        server.listen(0, '127.0.0.1', () => {
          serverURL = `http://127.0.0.1:${server.address().port}`
          done()
        })
      })

      afterEach(() => {
        server.close()
      })

      it('delivers messages that match the origin', (done) => {
        let b = null
        listener = (event) => {
          window.removeEventListener('message', listener)
          b.close()
          expect(event.data).to.equal('deliver')
          done()
        }
        window.addEventListener('message', listener)
        b = window.open(serverURL, '', 'show=no')
      })
    })
  })

  describe('webgl', () => {
    before(function () {
      if (process.platform === 'win32') {
        this.skip()
      }
    })

    it('can be get as context in canvas', () => {
      if (process.platform === 'linux') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return
      }

      const webgl = document.createElement('canvas').getContext('webgl')
      expect(webgl).to.not.be.null()
    })
  })

  describe('web workers', () => {
    it('Worker can work', (done) => {
      const worker = new Worker('../fixtures/workers/worker.js')
      const message = 'ping'
      worker.onmessage = (event) => {
        expect(event.data).to.equal(message)
        worker.terminate()
        done()
      }
      worker.postMessage(message)
    })

    it('Worker has no node integration by default', (done) => {
      const worker = new Worker('../fixtures/workers/worker_node.js')
      worker.onmessage = (event) => {
        expect(event.data).to.equal('undefined undefined undefined undefined')
        worker.terminate()
        done()
      }
    })

    it('Worker has node integration with nodeIntegrationInWorker', (done) => {
      const webview = new WebView()
      webview.addEventListener('ipc-message', (e) => {
        expect(e.channel).to.equal('object function object function')
        webview.remove()
        done()
      })
      webview.src = `file://${fixtures}/pages/worker.html`
      webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker')
      document.body.appendChild(webview)
    })

    // FIXME: disabled during chromium update due to crash in content::WorkerScriptFetchInitiator::CreateScriptLoaderOnIO
    xdescribe('SharedWorker', () => {
      it('can work', (done) => {
        const worker = new SharedWorker('../fixtures/workers/shared_worker.js')
        const message = 'ping'
        worker.port.onmessage = (event) => {
          expect(event.data).to.equal(message)
          done()
        }
        worker.port.postMessage(message)
      })

      it('has no node integration by default', (done) => {
        const worker = new SharedWorker('../fixtures/workers/shared_worker_node.js')
        worker.port.onmessage = (event) => {
          expect(event.data).to.equal('undefined undefined undefined undefined')
          done()
        }
      })

      it('has node integration with nodeIntegrationInWorker', (done) => {
        const webview = new WebView()
        webview.addEventListener('console-message', (e) => {
          console.log(e)
        })
        webview.addEventListener('ipc-message', (e) => {
          expect(e.channel).to.equal('object function object function')
          webview.remove()
          done()
        })
        webview.src = `file://${fixtures}/pages/shared_worker.html`
        webview.setAttribute('webpreferences', 'nodeIntegration, nodeIntegrationInWorker')
        document.body.appendChild(webview)
      })
    })
  })

  describe('iframe', () => {
    let iframe = null

    beforeEach(() => {
      iframe = document.createElement('iframe')
    })

    afterEach(() => {
      document.body.removeChild(iframe)
    })

    it('does not have node integration', (done) => {
      iframe.src = `file://${fixtures}/pages/set-global.html`
      document.body.appendChild(iframe)
      iframe.onload = () => {
        expect(iframe.contentWindow.test).to.equal('undefined undefined undefined')
        done()
      }
    })
  })

  describe('storage', () => {
    describe('DOM storage quota increase', () => {
      ['localStorage', 'sessionStorage'].forEach((storageName) => {
        const storage = window[storageName]
        it(`allows saving at least 40MiB in ${storageName}`, (done) => {
          // Although JavaScript strings use UTF-16, the underlying
          // storage provider may encode strings differently, muddling the
          // translation between character and byte counts. However,
          // a string of 40 * 2^20 characters will require at least 40MiB
          // and presumably no more than 80MiB, a size guaranteed to
          // to exceed the original 10MiB quota yet stay within the
          // new 100MiB quota.
          // Note that both the key name and value affect the total size.
          const testKeyName = '_electronDOMStorageQuotaIncreasedTest'
          const length = 40 * Math.pow(2, 20) - testKeyName.length
          storage.setItem(testKeyName, 'X'.repeat(length))
          // Wait at least one turn of the event loop to help avoid false positives
          // Although not entirely necessary, the previous version of this test case
          // failed to detect a real problem (perhaps related to DOM storage data caching)
          // wherein calling `getItem` immediately after `setItem` would appear to work
          // but then later (e.g. next tick) it would not.
          setTimeout(() => {
            expect(storage.getItem(testKeyName)).to.have.lengthOf(length)
            storage.removeItem(testKeyName)
            done()
          }, 1)
        })
        it(`throws when attempting to use more than 128MiB in ${storageName}`, () => {
          expect(() => {
            const testKeyName = '_electronDOMStorageQuotaStillEnforcedTest'
            const length = 128 * Math.pow(2, 20) - testKeyName.length
            try {
              storage.setItem(testKeyName, 'X'.repeat(length))
            } finally {
              storage.removeItem(testKeyName)
            }
          }).to.throw()
        })
      })
    })

    it('requesting persitent quota works', (done) => {
      navigator.webkitPersistentStorage.requestQuota(1024 * 1024, (grantedBytes) => {
        expect(grantedBytes).to.equal(1048576)
        done()
      })
    })
  })

  describe('websockets', () => {
    let wss = null
    let server = null
    const WebSocketServer = ws.Server

    afterEach(() => {
      wss.close()
      server.close()
    })

    it('has user agent', (done) => {
      server = http.createServer()
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        wss = new WebSocketServer({ server: server })
        wss.on('error', done)
        wss.on('connection', (ws, upgradeReq) => {
          if (upgradeReq.headers['user-agent']) {
            done()
          } else {
            done('user agent is empty')
          }
        })
        const socket = new WebSocket(`ws://127.0.0.1:${port}`)
      })
    })
  })

  describe('Promise', () => {
    it('resolves correctly in Node.js calls', (done) => {
      class XElement extends HTMLElement {}
      customElements.define('x-element', XElement)
      setImmediate(() => {
        let called = false
        Promise.resolve().then(() => {
          done(called ? void 0 : new Error('wrong sequence'))
        })
        document.createElement('x-element')
        called = true
      })
    })

    it('resolves correctly in Electron calls', (done) => {
      class YElement extends HTMLElement {}
      customElements.define('y-element', YElement)
      ipcRenderer.invoke('ping').then(() => {
        let called = false
        Promise.resolve().then(() => {
          done(called ? void 0 : new Error('wrong sequence'))
        })
        document.createElement('y-element')
        called = true
      })
    })
  })

  describe('fetch', () => {
    it('does not crash', (done) => {
      const server = http.createServer((req, res) => {
        res.end('test')
        server.close()
      })
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        fetch(`http://127.0.0.1:${port}`).then((res) => res.body.getReader())
          .then((reader) => {
            reader.read().then((r) => {
              reader.cancel()
              done()
            })
          }).catch((e) => done(e))
      })
    })
  })

  describe('window.alert(message, title)', () => {
    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.alert({ toString: null })
      }).to.throw('Cannot convert object to primitive value')
    })
  })

  describe('window.confirm(message, title)', () => {
    it('throws an exception when the arguments cannot be converted to strings', () => {
      expect(() => {
        window.confirm({ toString: null }, 'title')
      }).to.throw('Cannot convert object to primitive value')
    })
  })

  describe('window.history', () => {
    describe('window.history.go(offset)', () => {
      it('throws an exception when the argumnet cannot be converted to a string', () => {
        expect(() => {
          window.history.go({ toString: null })
        }).to.throw('Cannot convert object to primitive value')
      })
    })
  })

  // TODO(nornagon): this is broken on CI, it triggers:
  // [FATAL:speech_synthesis.mojom-shared.h(237)] The outgoing message will
  // trigger VALIDATION_ERROR_UNEXPECTED_NULL_POINTER at the receiving side
  // (null text in SpeechSynthesisUtterance struct).
  describe.skip('SpeechSynthesis', () => {
    before(function () {
      if (!features.isTtsEnabled()) {
        this.skip()
      }
    })

    it('should emit lifecycle events', async () => {
      const sentence = `long sentence which will take at least a few seconds to
          utter so that it's possible to pause and resume before the end`
      const utter = new SpeechSynthesisUtterance(sentence)
      // Create a dummy utterence so that speech synthesis state
      // is initialized for later calls.
      speechSynthesis.speak(new SpeechSynthesisUtterance())
      speechSynthesis.cancel()
      speechSynthesis.speak(utter)
      // paused state after speak()
      expect(speechSynthesis.paused).to.be.false()
      await new Promise((resolve) => { utter.onstart = resolve })
      // paused state after start event
      expect(speechSynthesis.paused).to.be.false()

      speechSynthesis.pause()
      // paused state changes async, right before the pause event
      expect(speechSynthesis.paused).to.be.false()
      await new Promise((resolve) => { utter.onpause = resolve })
      expect(speechSynthesis.paused).to.be.true()

      speechSynthesis.resume()
      await new Promise((resolve) => { utter.onresume = resolve })
      // paused state after resume event
      expect(speechSynthesis.paused).to.be.false()

      await new Promise((resolve) => { utter.onend = resolve })
    })
  })
})

describe('console functions', () => {
  it('should exist', () => {
    expect(console.log, 'log').to.be.a('function')
    expect(console.error, 'error').to.be.a('function')
    expect(console.warn, 'warn').to.be.a('function')
    expect(console.info, 'info').to.be.a('function')
    expect(console.debug, 'debug').to.be.a('function')
    expect(console.trace, 'trace').to.be.a('function')
    expect(console.time, 'time').to.be.a('function')
    expect(console.timeEnd, 'timeEnd').to.be.a('function')
  })
})
