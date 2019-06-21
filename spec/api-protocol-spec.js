const chai = require('chai')
const dirtyChai = require('dirty-chai')
const chaiAsPromised = require('chai-as-promised')

const http = require('http')
const path = require('path')
const qs = require('querystring')
const { promisify } = require('util')
const { emittedOnce } = require('./events-helpers')
const { closeWindow } = require('./window-helpers')
const { remote } = require('electron')
const { BrowserWindow, ipcMain, protocol, session, webContents } = remote
// The RPC API doesn't seem to support calling methods on remote objects very
// well. In order to test stream protocol, we must work around this limitation
// and use Stream instances created in the browser process.
const stream = remote.require('stream')

const { expect } = chai
chai.use(dirtyChai)
chai.use(chaiAsPromised)

/* The whole protocol API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('protocol module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures')
  const protocolName = 'sp'
  const text = 'valar morghulis'
  const postData = {
    name: 'post test',
    type: 'string'
  }

  const registerStringProtocol = promisify(protocol.registerStringProtocol)
  const registerBufferProtocol = promisify(protocol.registerBufferProtocol)
  const registerStreamProtocol = promisify(protocol.registerStreamProtocol)
  const registerFileProtocol = promisify(protocol.registerFileProtocol)
  const registerHttpProtocol = promisify(protocol.registerHttpProtocol)
  const unregisterProtocol = promisify(protocol.unregisterProtocol)
  const interceptStringProtocol = promisify(protocol.interceptStringProtocol)
  const interceptBufferProtocol = promisify(protocol.interceptBufferProtocol)
  const interceptStreamProtocol = promisify(protocol.interceptStreamProtocol)
  const interceptFileProtocol = promisify(protocol.interceptFileProtocol)
  const interceptHttpProtocol = promisify(protocol.interceptHttpProtocol)
  const uninterceptProtocol = promisify(protocol.uninterceptProtocol)

  const contents = webContents.create({})
  after(() => contents.destroy())

  async function ajax (url, options = {}) {
    // Note that we need to do navigation every time after a protocol is
    // registered or unregistered, otherwise the new protocol won't be
    // recognized by current page when NetworkService is used.
    await contents.loadFile(path.join(fixtures, 'pages', 'jquery.html'))
    return contents.executeJavaScript(`ajax("${url}", ${JSON.stringify(options)})`)
  }

  function delay (ms) {
    return new Promise((resolve) => {
      setTimeout(resolve, ms)
    })
  }

  function getStream (chunkSize = text.length, data = text) {
    const body = stream.PassThrough()

    async function sendChunks () {
      let buf = Buffer.from(data)
      for (;;) {
        body.push(buf.slice(0, chunkSize))
        buf = buf.slice(chunkSize)
        if (!buf.length) {
          break
        }
        // emulate network delay
        await delay(50)
      }
      body.push(null)
    }

    sendChunks()
    return body
  }

  afterEach((done) => {
    protocol.unregisterProtocol(protocolName, () => {
      protocol.uninterceptProtocol('http', () => done())
    })
  })

  describe('protocol.register(Any)Protocol', () => {
    const emptyHandler = (request, callback) => callback()

    it('throws error when scheme is already registered', async () => {
      await registerStringProtocol(protocolName, emptyHandler)
      await expect(registerBufferProtocol(protocolName, emptyHandler)).to.be.eventually.rejectedWith(Error)
    })

    it('does not crash when handler is called twice', async () => {
      const doubleHandler = (request, callback) => {
        try {
          callback(text)
          callback()
        } catch (error) {
          // Ignore error
        }
      }
      await registerStringProtocol(protocolName, doubleHandler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sends error when callback is called with nothing', async () => {
      await registerBufferProtocol(protocolName, emptyHandler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('does not crash when callback is called in next tick', async () => {
      const handler = (request, callback) => {
        setImmediate(() => callback(text))
      }
      await registerStringProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })
  })

  describe('protocol.unregisterProtocol', () => {
    it('returns error when scheme does not exist', async () => {
      await expect(unregisterProtocol('not-exist')).to.be.eventually.rejectedWith(Error)
    })
  })

  describe('protocol.registerStringProtocol', () => {
    it('sends string as response', async () => {
      const handler = (request, callback) => callback(text)
      await registerStringProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sets Access-Control-Allow-Origin', async () => {
      const handler = (request, callback) => callback(text)
      await registerStringProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.headers).to.include('access-control-allow-origin: *')
    })

    it('sends object as response', async () => {
      const handler = (request, callback) => {
        callback({
          data: text,
          mimeType: 'text/html'
        })
      }
      await registerStringProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('fails when sending object other than string', async () => {
      const notAString = () => {}
      const handler = (request, callback) => callback(notAString)
      await registerStringProtocol(protocolName, handler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.registerBufferProtocol', () => {
    const buffer = Buffer.from(text)
    it('sends Buffer as response', async () => {
      const handler = (request, callback) => callback(buffer)
      await registerBufferProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sets Access-Control-Allow-Origin', async () => {
      const handler = (request, callback) => callback(buffer)
      await registerBufferProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.headers).to.include('access-control-allow-origin: *')
    })

    it('sends object as response', async () => {
      const handler = (request, callback) => {
        callback({
          data: buffer,
          mimeType: 'text/html'
        })
      }
      await registerBufferProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('fails when sending string', async () => {
      const handler = (request, callback) => callback(text)
      await registerBufferProtocol(protocolName, handler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.registerFileProtocol', () => {
    const filePath = path.join(fixtures, 'asar', 'a.asar', 'file1')
    const fileContent = require('fs').readFileSync(filePath)
    const normalPath = path.join(fixtures, 'pages', 'a.html')
    const normalContent = require('fs').readFileSync(normalPath)

    it('sends file path as response', async () => {
      const handler = (request, callback) => callback(filePath)
      await registerFileProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
    })

    it('sets Access-Control-Allow-Origin', async () => {
      const handler = (request, callback) => callback(filePath)
      await registerFileProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
      expect(r.headers).to.include('access-control-allow-origin: *')
    })

    it('sets custom headers', async () => {
      const handler = (request, callback) => callback({
        path: filePath,
        headers: { 'X-Great-Header': 'sogreat' }
      })
      await registerFileProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
      expect(r.headers).to.include('x-great-header: sogreat')
    })

    it('throws an error when custom headers are invalid', (done) => {
      const handler = (request, callback) => {
        expect(() => callback({
          path: filePath,
          headers: { 'X-Great-Header': 42 }
        })).to.throw(Error, 'Value of \'X-Great-Header\' header has to be a string')
        done()
      }
      registerFileProtocol(protocolName, handler).then(() => {
        ajax(protocolName + '://fake-host')
      })
    })

    it('sends object as response', async () => {
      const handler = (request, callback) => callback({ path: filePath })
      await registerFileProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
    })

    it('can send normal file', async () => {
      const handler = (request, callback) => callback(normalPath)
      await registerFileProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(normalContent))
    })

    it('fails when sending unexist-file', async () => {
      const fakeFilePath = path.join(fixtures, 'asar', 'a.asar', 'not-exist')
      const handler = (request, callback) => callback(fakeFilePath)
      await registerFileProtocol(protocolName, handler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('fails when sending unsupported content', async () => {
      const handler = (request, callback) => callback(new Date())
      await registerFileProtocol(protocolName, handler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.registerHttpProtocol', () => {
    it('sends url as response', async () => {
      const server = http.createServer((req, res) => {
        expect(req.headers.accept).to.not.equal('')
        res.end(text)
        server.close()
      })
      await server.listen(0, '127.0.0.1')

      const port = server.address().port
      const url = 'http://127.0.0.1:' + port
      const handler = (request, callback) => callback({ url })
      await registerHttpProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('fails when sending invalid url', async () => {
      const handler = (request, callback) => callback({ url: 'url' })
      await registerHttpProtocol(protocolName, handler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('fails when sending unsupported content', async () => {
      const handler = (request, callback) => callback(new Date())
      await registerHttpProtocol(protocolName, handler)
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('works when target URL redirects', async () => {
      const server = http.createServer((req, res) => {
        if (req.url === '/serverRedirect') {
          res.statusCode = 301
          res.setHeader('Location', `http://${req.rawHeaders[1]}`)
          res.end()
        } else {
          res.end(text)
        }
      })
      after(() => server.close())
      await server.listen(0, '127.0.0.1')

      const port = server.address().port
      const url = `${protocolName}://fake-host`
      const redirectURL = `http://127.0.0.1:${port}/serverRedirect`
      const handler = (request, callback) => callback({ url: redirectURL })
      await registerHttpProtocol(protocolName, handler)

      const r = await ajax(url)
      expect(r.data).to.equal(text)
    })

    it('can access request headers', (done) => {
      const handler = (request) => {
        expect(request).to.have.a.property('headers')
        done()
      }
      registerHttpProtocol(protocolName, handler, () => {
        ajax(protocolName + '://fake-host')
      })
    })
  })

  describe('protocol.registerStreamProtocol', () => {
    it('sends Stream as response', async () => {
      const handler = (request, callback) => callback(getStream())
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sends object as response', async () => {
      const handler = (request, callback) => callback({ data: getStream() })
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.status).to.equal(200)
    })

    it('sends custom response headers', async () => {
      const handler = (request, callback) => callback({
        data: getStream(3),
        headers: {
          'x-electron': ['a', 'b']
        }
      })
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.status).to.equal(200)
      expect(r.headers).to.include('x-electron: a, b')
    })

    it('sends custom status code', async () => {
      const handler = (request, callback) => callback({
        statusCode: 204,
        data: null
      })
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.be.undefined()
      expect(r.status).to.equal(204)
    })

    it('receives request headers', async () => {
      const handler = (request, callback) => {
        callback({
          headers: {
            'content-type': 'application/json'
          },
          data: getStream(5, JSON.stringify(Object.assign({}, request.headers)))
        })
      }
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host', { headers: { 'x-return-headers': 'yes' } })
      expect(r.data['x-return-headers']).to.equal('yes')
    })

    it('returns response multiple response headers with the same name', async () => {
      const handler = (request, callback) => {
        callback({
          headers: {
            'header1': ['value1', 'value2'],
            'header2': 'value3'
          },
          data: getStream()
        })
      }
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      // SUBTLE: when the response headers have multiple values it
      // separates values by ", ". When the response headers are incorrectly
      // converting an array to a string it separates values by ",".
      expect(r.headers).to.equal('header1: value1, value2\r\nheader2: value3\r\n')
    })

    it('can handle large responses', async () => {
      const data = Buffer.alloc(128 * 1024)
      const handler = (request, callback) => {
        callback(getStream(data.length, data))
      }
      await registerStreamProtocol(protocolName, handler)
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.have.lengthOf(data.length)
    })
  })

  describe('protocol.isProtocolHandled', () => {
    it('returns true for about:', async () => {
      const result = await protocol.isProtocolHandled('about')
      expect(result).to.be.true()
    })

    it('returns true for file:', async () => {
      const result = await protocol.isProtocolHandled('file')
      expect(result).to.be.true()
    })

    it('returns true for http:', async () => {
      const result = await protocol.isProtocolHandled('http')
      expect(result).to.be.true()
    })

    it('returns true for https:', async () => {
      const result = await protocol.isProtocolHandled('https')
      expect(result).to.be.true()
    })

    it('returns false when scheme is not registered', async () => {
      const result = await protocol.isProtocolHandled('no-exist')
      expect(result).to.be.false()
    })

    it('returns true for custom protocol', async () => {
      const emptyHandler = (request, callback) => callback()
      await registerStringProtocol(protocolName, emptyHandler)
      const result = await protocol.isProtocolHandled(protocolName)
      expect(result).to.be.true()
    })

    it('returns true for intercepted protocol', async () => {
      const emptyHandler = (request, callback) => callback()
      await interceptStringProtocol('http', emptyHandler)
      const result = await protocol.isProtocolHandled('http')
      expect(result).to.be.true()
    })
  })

  describe('protocol.intercept(Any)Protocol', () => {
    const emptyHandler = (request, callback) => callback()
    it('throws error when scheme is already intercepted', (done) => {
      protocol.interceptStringProtocol('http', emptyHandler, (error) => {
        expect(error).to.be.null()
        protocol.interceptBufferProtocol('http', emptyHandler, (error) => {
          expect(error).to.not.be.null()
          done()
        })
      })
    })

    it('does not crash when handler is called twice', async () => {
      const doubleHandler = (request, callback) => {
        try {
          callback(text)
          callback()
        } catch (error) {
          // Ignore error
        }
      }
      await interceptStringProtocol('http', doubleHandler)
      const r = await ajax('http://fake-host')
      expect(r.data).to.be.equal(text)
    })

    it('sends error when callback is called with nothing', async () => {
      await interceptStringProtocol('http', emptyHandler)
      await expect(ajax('http://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.interceptStringProtocol', () => {
    it('can intercept http protocol', async () => {
      const handler = (request, callback) => callback(text)
      await interceptStringProtocol('http', handler)
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal(text)
    })

    it('can set content-type', async () => {
      const handler = (request, callback) => {
        callback({
          mimeType: 'application/json',
          data: '{"value": 1}'
        })
      }
      await interceptStringProtocol('http', handler)
      const r = await ajax('http://fake-host')
      expect(r.data).to.be.an('object')
      expect(r.data).to.have.a.property('value').that.is.equal(1)
    })

    it('can receive post data', async () => {
      const handler = (request, callback) => {
        const uploadData = request.uploadData[0].bytes.toString()
        callback({ data: uploadData })
      }
      await interceptStringProtocol('http', handler)
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData)
    })
  })

  describe('protocol.interceptBufferProtocol', () => {
    it('can intercept http protocol', async () => {
      const handler = (request, callback) => callback(Buffer.from(text))
      await interceptBufferProtocol('http', handler)
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal(text)
    })

    it('can receive post data', async () => {
      const handler = (request, callback) => {
        const uploadData = request.uploadData[0].bytes
        callback(uploadData)
      }
      await interceptBufferProtocol('http', handler)
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect(r.data).to.equal($.param(postData))
    })
  })

  describe('protocol.interceptHttpProtocol', () => {
    // FIXME(zcbenz): This test was passing because the test itself was wrong,
    // I don't know whether it ever passed before and we should take a look at
    // it in future.
    xit('can send POST request', async () => {
      const server = http.createServer((req, res) => {
        let body = ''
        req.on('data', (chunk) => {
          body += chunk
        })
        req.on('end', () => {
          res.end(body)
        })
        server.close()
      })
      after(() => server.close())
      await server.listen(0, '127.0.0.1')

      const port = server.address().port
      const url = `http://127.0.0.1:${port}`
      const handler = (request, callback) => {
        const data = {
          url: url,
          method: 'POST',
          uploadData: {
            contentType: 'application/x-www-form-urlencoded',
            data: request.uploadData[0].bytes.toString()
          },
          session: null
        }
        callback(data)
      }
      await interceptHttpProtocol('http', handler)
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData)
    })

    it('can use custom session', async () => {
      const customSession = session.fromPartition('custom-ses', { cache: false })
      customSession.webRequest.onBeforeRequest((details, callback) => {
        expect(details.url).to.equal('http://fake-host/')
        callback({ cancel: true })
      })
      after(() => customSession.webRequest.onBeforeRequest(null))

      const handler = (request, callback) => {
        callback({
          url: request.url,
          session: customSession
        })
      }
      await interceptHttpProtocol('http', handler)
      await expect(fetch('http://fake-host')).to.be.eventually.rejectedWith(Error)
    })

    it('can access request headers', (done) => {
      const handler = (request) => {
        expect(request).to.have.a.property('headers')
        done()
      }
      protocol.interceptHttpProtocol('http', handler, () => {
        fetch('http://fake-host')
      })
    })
  })

  describe('protocol.interceptStreamProtocol', () => {
    it('can intercept http protocol', async () => {
      const handler = (request, callback) => callback(getStream())
      await interceptStreamProtocol('http', handler)
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal(text)
    })

    it('can receive post data', async () => {
      const handler = (request, callback) => {
        callback(getStream(3, request.uploadData[0].bytes.toString()))
      }
      await interceptStreamProtocol('http', handler)
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData)
    })

    it('can execute redirects', async () => {
      const handler = (request, callback) => {
        if (request.url.indexOf('http://fake-host') === 0) {
          setTimeout(() => {
            callback({
              data: null,
              statusCode: 302,
              headers: {
                Location: 'http://fake-redirect'
              }
            })
          }, 300)
        } else {
          expect(request.url.indexOf('http://fake-redirect')).to.equal(0)
          callback(getStream(1, 'redirect'))
        }
      }
      await interceptStreamProtocol('http', handler)
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal('redirect')
    })
  })

  describe('protocol.uninterceptProtocol', () => {
    it('returns error when scheme does not exist', async () => {
      await expect(uninterceptProtocol('not-exist')).to.be.eventually.rejectedWith(Error)
    })

    it('returns error when scheme is not intercepted', async () => {
      await expect(uninterceptProtocol('http')).to.be.eventually.rejectedWith(Error)
    })
  })

  describe('protocol.registerSchemesAsPrivileged standard', () => {
    const standardScheme = remote.getGlobal('standardScheme')
    const origin = `${standardScheme}://fake-host`
    const imageURL = `${origin}/test.png`
    const filePath = path.join(fixtures, 'pages', 'b.html')
    const fileContent = '<img src="/test.png" />'
    let w = null
    let success = null

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
      success = false
    })

    afterEach((done) => {
      protocol.unregisterProtocol(standardScheme, () => {
        closeWindow(w).then(() => {
          w = null
          done()
        })
      })
    })

    it('resolves relative resources', async () => {
      const handler = (request, callback) => {
        if (request.url === imageURL) {
          success = true
          callback()
        } else {
          callback(filePath)
        }
      }
      await registerFileProtocol(standardScheme, handler)
      await w.loadURL(origin)
    })

    it('resolves absolute resources', async () => {
      const handler = (request, callback) => {
        if (request.url === imageURL) {
          success = true
          callback()
        } else {
          callback({
            data: fileContent,
            mimeType: 'text/html'
          })
        }
      }
      await registerStringProtocol(standardScheme, handler)
      await w.loadURL(origin)
    })

    it('can have fetch working in it', async () => {
      const content = '<html><script>fetch("http://github.com")</script></html>'
      const handler = (request, callback) => callback({ data: content, mimeType: 'text/html' })
      await registerStringProtocol(standardScheme, handler)
      await w.loadURL(origin)
    })

    it('can access files through the FileSystem API', (done) => {
      const filePath = path.join(fixtures, 'pages', 'filesystem.html')
      const handler = (request, callback) => callback({ path: filePath })
      protocol.registerFileProtocol(standardScheme, handler, (error) => {
        if (error) return done(error)
        w.loadURL(origin)
      })
      ipcMain.once('file-system-error', (event, err) => done(err))
      ipcMain.once('file-system-write-end', () => done())
    })

    it('registers secure, when {secure: true}', (done) => {
      const filePath = path.join(fixtures, 'pages', 'cache-storage.html')
      const handler = (request, callback) => callback({ path: filePath })
      ipcMain.once('success', () => done())
      ipcMain.once('failure', (event, err) => done(err))
      protocol.registerFileProtocol(standardScheme, handler, (error) => {
        if (error) return done(error)
        w.loadURL(origin)
      })
    })
  })

  describe('protocol.registerSchemesAsPrivileged cors-fetch', function () {
    const standardScheme = remote.getGlobal('standardScheme')
    let w = null

    beforeEach((done) => {
      protocol.unregisterProtocol(standardScheme, () => done())
    })

    afterEach((done) => {
      closeWindow(w).then(() => {
        w = null
        done()
      })
    })

    it('supports fetch api by default', async () => {
      const url = 'file://' + fixtures + '/assets/logo.png'
      const response = await window.fetch(url)
      expect(response.ok).to.be.true()
    })

    it('allows CORS requests by default', async () => {
      await allowsCORSRequests('cors', 200, `<html>
        <script>
        const {ipcRenderer} = require('electron')
        fetch('cors://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
        </html>`)
    })

    it('disallows CORS, but allows fetch requests, when specified', async () => {
      await allowsCORSRequests('no-cors', 'failed', `<html>
        <script>
        const {ipcRenderer} = require('electron')
        fetch('no-cors://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
        </html>`)
    })

    it('allows CORS, but disallows fetch requests, when specified', async () => {
      await allowsCORSRequests('no-fetch', 'failed', `<html>
        <script>
        const {ipcRenderer} = require('electron')
        fetch('no-fetch://myhost').then(function
        (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function (response) {
          ipcRenderer.send('response', 'failed')
        })
        </script>
        </html>`)
    })

    async function allowsCORSRequests (corsScheme, expected, content) {
      await registerStringProtocol(standardScheme, (request, callback) => {
        callback({ data: content, mimeType: 'text/html' })
      })
      await registerStringProtocol(corsScheme, (request, callback) => {
        callback('')
      })
      after(async () => {
        try {
          await unregisterProtocol(corsScheme)
        } catch {
          // Ignore error.
        }
      })

      const newContents = webContents.create({ nodeIntegration: true })
      after(() => newContents.destroy())

      const event = emittedOnce(ipcMain, 'response')
      newContents.loadURL(standardScheme + '://fake-host')
      const [, response] = await event
      expect(response).to.equal(expected)
    }
  })
})
