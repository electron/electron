import { expect } from 'chai'
import { protocol, webContents, WebContents, session, BrowserWindow, ipcMain } from 'electron'
import { promisify } from 'util'
import { AddressInfo } from 'net'
import * as path from 'path'
import * as http from 'http'
import * as fs from 'fs'
import * as qs from 'querystring'
import * as stream from 'stream'
import { closeWindow } from './window-helpers'
import { emittedOnce } from './events-helpers'

const fixturesPath = path.resolve(__dirname, '..', 'spec', 'fixtures')

const registerStringProtocol = promisify(protocol.registerStringProtocol)
const registerBufferProtocol = promisify(protocol.registerBufferProtocol)
const registerFileProtocol = promisify(protocol.registerFileProtocol)
const registerHttpProtocol = promisify(protocol.registerHttpProtocol)
const registerStreamProtocol = promisify(protocol.registerStreamProtocol)
const interceptStringProtocol = promisify(protocol.interceptStringProtocol)
const interceptBufferProtocol = promisify(protocol.interceptBufferProtocol)
const interceptHttpProtocol = promisify(protocol.interceptHttpProtocol)
const interceptStreamProtocol = promisify(protocol.interceptStreamProtocol)
const unregisterProtocol = promisify(protocol.unregisterProtocol)
const uninterceptProtocol = promisify(protocol.uninterceptProtocol)

const text = 'valar morghulis'
const protocolName = 'sp'
const postData = {
  name: 'post test',
  type: 'string'
}

function delay (ms: number) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms)
  })
}

function getStream (chunkSize = text.length, data: Buffer | string = text) {
  const body = new stream.PassThrough()

  async function sendChunks () {
    await delay(0) // the stream protocol API breaks if you send data immediately.
    let buf = Buffer.from(data as any) // nodejs typings are wrong, Buffer.from can take a Buffer
    for (;;) {
      body.push(buf.slice(0, chunkSize))
      buf = buf.slice(chunkSize)
      if (!buf.length) {
        break
      }
      // emulate some network delay
      await delay(10)
    }
    body.push(null)
  }

  sendChunks()
  return body
}

// A promise that can be resolved externally.
function defer (): Promise<any> & {resolve: Function, reject: Function} {
  let promiseResolve: Function = null as unknown as Function
  let promiseReject: Function = null as unknown as Function
  const promise: any = new Promise((resolve, reject) => {
    promiseResolve = resolve
    promiseReject = reject
  })
  promise.resolve = promiseResolve
  promise.reject = promiseReject
  return promise
}

describe('protocol module', () => {
  let contents: WebContents = null as unknown as WebContents
  // NB. sandbox: true is used because it makes navigations much (~8x) faster.
  before(() => { contents = (webContents as any).create({ sandbox: true }) })
  after(() => (contents as any).destroy())

  async function ajax (url: string, options = {}) {
    // Note that we need to do navigation every time after a protocol is
    // registered or unregistered, otherwise the new protocol won't be
    // recognized by current page when NetworkService is used.
    await contents.loadFile(path.join(__dirname, 'fixtures', 'pages', 'jquery.html'))
    return contents.executeJavaScript(`ajax("${url}", ${JSON.stringify(options)})`)
  }

  afterEach(async () => {
    await new Promise(resolve => protocol.unregisterProtocol(protocolName, (/* ignore error */) => resolve()))
    await new Promise(resolve => protocol.uninterceptProtocol('http', () => resolve()))
  })

  describe('protocol.register(Any)Protocol', () => {
    it('throws error when scheme is already registered', async () => {
      await registerStringProtocol(protocolName, (req, cb) => cb())
      await expect(registerBufferProtocol(protocolName, (req, cb) => cb())).to.be.eventually.rejectedWith(Error)
    })

    it('does not crash when handler is called twice', async () => {
      await registerStringProtocol(protocolName, (request, callback) => {
        try {
          callback(text)
          callback()
        } catch (error) {
          // Ignore error
        }
      })
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sends error when callback is called with nothing', async () => {
      await registerBufferProtocol(protocolName, (req, cb) => cb())
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('does not crash when callback is called in next tick', async () => {
      await registerStringProtocol(protocolName, (request, callback) => {
        setImmediate(() => callback(text))
      })
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
      await registerStringProtocol(protocolName, (request, callback) => callback(text))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sets Access-Control-Allow-Origin', async () => {
      await registerStringProtocol(protocolName, (request, callback) => callback(text))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.headers).to.include('access-control-allow-origin: *')
    })

    it('sends object as response', async () => {
      await registerStringProtocol(protocolName, (request, callback) => {
        callback({
          data: text,
          mimeType: 'text/html'
        })
      })
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('fails when sending object other than string', async () => {
      const notAString = () => {}
      await registerStringProtocol(protocolName, (request, callback) => callback(notAString as any))
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.registerBufferProtocol', () => {
    const buffer = Buffer.from(text)
    it('sends Buffer as response', async () => {
      await registerBufferProtocol(protocolName, (request, callback) => callback(buffer))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sets Access-Control-Allow-Origin', async () => {
      await registerBufferProtocol(protocolName, (request, callback) => callback(buffer))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.headers).to.include('access-control-allow-origin: *')
    })

    it('sends object as response', async () => {
      await registerBufferProtocol(protocolName, (request, callback) => {
        callback({
          data: buffer,
          mimeType: 'text/html'
        })
      })
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('fails when sending string', async () => {
      await registerBufferProtocol(protocolName, (request, callback) => callback(text as any))
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.registerFileProtocol', () => {
    const filePath = path.join(fixturesPath, 'test.asar', 'a.asar', 'file1')
    const fileContent = fs.readFileSync(filePath)
    const normalPath = path.join(fixturesPath, 'pages', 'a.html')
    const normalContent = fs.readFileSync(normalPath)

    it('sends file path as response', async () => {
      await registerFileProtocol(protocolName, (request, callback) => callback(filePath))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
    })

    it('sets Access-Control-Allow-Origin', async () => {
      await registerFileProtocol(protocolName, (request, callback) => callback(filePath))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
      expect(r.headers).to.include('access-control-allow-origin: *')
    })

    it('sets custom headers', async () => {
      await registerFileProtocol(protocolName, (request, callback) => callback({
        path: filePath,
        headers: { 'X-Great-Header': 'sogreat' }
      }))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
      expect(r.headers).to.include('x-great-header: sogreat')
    })

    it.skip('throws an error when custom headers are invalid', (done) => {
      registerFileProtocol(protocolName, (request, callback) => {
        expect(() => callback({
          path: filePath,
          headers: { 'X-Great-Header': (42 as any) }
        })).to.throw(Error, `Value of 'X-Great-Header' header has to be a string`)
        done()
      }).then(() => {
        ajax(protocolName + '://fake-host')
      })
    })

    it('sends object as response', async () => {
      await registerFileProtocol(protocolName, (request, callback) => callback({ path: filePath }))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(fileContent))
    })

    it('can send normal file', async () => {
      await registerFileProtocol(protocolName, (request, callback) => callback(normalPath))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(String(normalContent))
    })

    it('fails when sending unexist-file', async () => {
      const fakeFilePath = path.join(fixturesPath, 'test.asar', 'a.asar', 'not-exist')
      await registerFileProtocol(protocolName, (request, callback) => callback(fakeFilePath))
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('fails when sending unsupported content', async () => {
      await registerFileProtocol(protocolName, (request, callback) => callback(new Date() as any))
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

      const port = (server.address() as AddressInfo).port
      const url = 'http://127.0.0.1:' + port
      await registerHttpProtocol(protocolName, (request, callback) => callback({ url }))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('fails when sending invalid url', async () => {
      await registerHttpProtocol(protocolName, (request, callback) => callback({ url: 'url' }))
      await expect(ajax(protocolName + '://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })

    it('fails when sending unsupported content', async () => {
      await registerHttpProtocol(protocolName, (request, callback) => callback(new Date() as any))
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

      const port = (server.address() as AddressInfo).port
      const url = `${protocolName}://fake-host`
      const redirectURL = `http://127.0.0.1:${port}/serverRedirect`
      await registerHttpProtocol(protocolName, (request, callback) => callback({ url: redirectURL }))

      const r = await ajax(url)
      expect(r.data).to.equal(text)
    })

    it('can access request headers', (done) => {
      protocol.registerHttpProtocol(protocolName, (request) => {
        expect(request).to.have.property('headers')
        done()
      }, () => {
        ajax(protocolName + '://fake-host')
      })
    })
  })

  describe('protocol.registerStreamProtocol', () => {
    it('sends Stream as response', async () => {
      await registerStreamProtocol(protocolName, (request, callback) => callback(getStream()))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
    })

    it('sends object as response', async () => {
      await registerStreamProtocol(protocolName, (request, callback) => callback({ data: getStream() }))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.status).to.equal(200)
    })

    it('sends custom response headers', async () => {
      await registerStreamProtocol(protocolName, (request, callback) => callback({
        data: getStream(3),
        headers: {
          'x-electron': ['a', 'b']
        }
      }))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.equal(text)
      expect(r.status).to.equal(200)
      expect(r.headers).to.include('x-electron: a, b')
    })

    it('sends custom status code', async () => {
      await registerStreamProtocol(protocolName, (request, callback) => callback({
        statusCode: 204,
        data: null
      }))
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.be.undefined('data')
      expect(r.status).to.equal(204)
    })

    it('receives request headers', async () => {
      await registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          headers: {
            'content-type': 'application/json'
          },
          data: getStream(5, JSON.stringify(Object.assign({}, request.headers)))
        })
      })
      const r = await ajax(protocolName + '://fake-host', { headers: { 'x-return-headers': 'yes' } })
      expect(r.data['x-return-headers']).to.equal('yes')
    })

    it('returns response multiple response headers with the same name', async () => {
      await registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          headers: {
            'header1': ['value1', 'value2'],
            'header2': 'value3'
          },
          data: getStream()
        })
      })
      const r = await ajax(protocolName + '://fake-host')
      // SUBTLE: when the response headers have multiple values it
      // separates values by ", ". When the response headers are incorrectly
      // converting an array to a string it separates values by ",".
      expect(r.headers).to.equal('header1: value1, value2\r\nheader2: value3\r\n')
    })

    it('can handle large responses', async () => {
      const data = Buffer.alloc(128 * 1024)
      await registerStreamProtocol(protocolName, (request, callback) => {
        callback(getStream(data.length, data))
      })
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.have.lengthOf(data.length)
    })

    it('can handle a stream completing while writing', async () => {
      function dumbPassthrough () {
        return new stream.Transform({
          async transform (chunk, encoding, cb) {
            cb(null, chunk)
          }
        })
      }
      await registerStreamProtocol(protocolName, (request, callback) => {
        callback({
          statusCode: 200,
          headers: { 'Content-Type': 'text/plain' },
          data: getStream(1024 * 1024, Buffer.alloc(1024 * 1024 * 2)).pipe(dumbPassthrough())
        })
      })
      const r = await ajax(protocolName + '://fake-host')
      expect(r.data).to.have.lengthOf(1024 * 1024 * 2)
    })
  })

  describe('protocol.isProtocolHandled', () => {
    it('returns true for built-in protocols', async () => {
      for (const p of ['about', 'file', 'http', 'https']) {
        const handled = await protocol.isProtocolHandled(p)
        expect(handled).to.be.true(`${p}: is handled`)
      }
    })

    it('returns false when scheme is not registered', async () => {
      const result = await protocol.isProtocolHandled('no-exist')
      expect(result).to.be.false('no-exist: is handled')
    })

    it('returns true for custom protocol', async () => {
      await registerStringProtocol(protocolName, (request, callback) => callback())
      const result = await protocol.isProtocolHandled(protocolName)
      expect(result).to.be.true('custom protocol is handled')
    })

    it('returns true for intercepted protocol', async () => {
      await interceptStringProtocol('http', (request, callback) => callback())
      const result = await protocol.isProtocolHandled('http')
      expect(result).to.be.true('intercepted protocol is handled')
    })
  })

  describe('protocol.intercept(Any)Protocol', () => {
    it('throws error when scheme is already intercepted', (done) => {
      protocol.interceptStringProtocol('http', (request, callback) => callback(), (error) => {
        expect(error).to.be.null('error')
        protocol.interceptBufferProtocol('http', (request, callback) => callback(), (error) => {
          expect(error).to.not.be.null('error')
          done()
        })
      })
    })

    it('does not crash when handler is called twice', async () => {
      await interceptStringProtocol('http', (request, callback) => {
        try {
          callback(text)
          callback()
        } catch (error) {
          // Ignore error
        }
      })
      const r = await ajax('http://fake-host')
      expect(r.data).to.be.equal(text)
    })

    it('sends error when callback is called with nothing', async () => {
      await interceptStringProtocol('http', (request, callback) => callback())
      await expect(ajax('http://fake-host')).to.be.eventually.rejectedWith(Error, '404')
    })
  })

  describe('protocol.interceptStringProtocol', () => {
    it('can intercept http protocol', async () => {
      await interceptStringProtocol('http', (request, callback) => callback(text))
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal(text)
    })

    it('can set content-type', async () => {
      await interceptStringProtocol('http', (request, callback) => {
        callback({
          mimeType: 'application/json',
          data: '{"value": 1}'
        })
      })
      const r = await ajax('http://fake-host')
      expect(r.data).to.be.an('object')
      expect(r.data).to.have.property('value').that.is.equal(1)
    })

    it('can set content-type with charset', async () => {
      await interceptStringProtocol('http', (request, callback) => {
        callback({
          mimeType: 'application/json; charset=UTF-8',
          data: '{"value": 1}'
        })
      })
      const r = await ajax('http://fake-host')
      expect(r.data).to.be.an('object')
      expect(r.data).to.have.property('value').that.is.equal(1)
    })

    it('can receive post data', async () => {
      await interceptStringProtocol('http', (request, callback) => {
        const uploadData = request.uploadData[0].bytes.toString()
        callback({ data: uploadData })
      })
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData)
    })
  })

  describe('protocol.interceptBufferProtocol', () => {
    it('can intercept http protocol', async () => {
      await interceptBufferProtocol('http', (request, callback) => callback(Buffer.from(text)))
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal(text)
    })

    it('can receive post data', async () => {
      await interceptBufferProtocol('http', (request, callback) => {
        const uploadData = request.uploadData[0].bytes
        callback(uploadData)
      })
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect(r.data).to.equal('name=post+test&type=string')
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

      const port = (server.address() as AddressInfo).port
      const url = `http://127.0.0.1:${port}`
      await interceptHttpProtocol('http', (request, callback) => {
        const data: Electron.RedirectRequest = {
          url: url,
          method: 'POST',
          uploadData: {
            contentType: 'application/x-www-form-urlencoded',
            data: request.uploadData[0].bytes
          },
          session: null
        }
        callback(data)
      })
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

      await interceptHttpProtocol('http', (request, callback) => {
        callback({
          url: request.url,
          session: customSession
        })
      })
      await expect(ajax('http://fake-host')).to.be.eventually.rejectedWith(Error)
    })

    it('can access request headers', (done) => {
      protocol.interceptHttpProtocol('http', (request) => {
        expect(request).to.have.property('headers')
        done()
      }, () => {
        ajax('http://fake-host')
      })
    })
  })

  describe('protocol.interceptStreamProtocol', () => {
    it('can intercept http protocol', async () => {
      await interceptStreamProtocol('http', (request, callback) => callback(getStream()))
      const r = await ajax('http://fake-host')
      expect(r.data).to.equal(text)
    })

    it('can receive post data', async () => {
      await interceptStreamProtocol('http', (request, callback) => {
        callback(getStream(3, request.uploadData[0].bytes.toString()))
      })
      const r = await ajax('http://fake-host', { type: 'POST', data: postData })
      expect({ ...qs.parse(r.data) }).to.deep.equal(postData)
    })

    it('can execute redirects', async () => {
      await interceptStreamProtocol('http', (request, callback) => {
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
      })
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

  describe.skip('protocol.registerSchemesAsPrivileged standard', () => {
    const standardScheme = (global as any).standardScheme
    const origin = `${standardScheme}://fake-host`
    const imageURL = `${origin}/test.png`
    const filePath = path.join(fixturesPath, 'pages', 'b.html')
    const fileContent = '<img src="/test.png" />'
    let w: BrowserWindow = null as unknown as BrowserWindow

    beforeEach(() => {
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          nodeIntegration: true
        }
      })
    })

    afterEach(async () => {
      await closeWindow(w)
      await unregisterProtocol(standardScheme)
      w = null as unknown as BrowserWindow
    })

    it('resolves relative resources', async () => {
      await registerFileProtocol(standardScheme, (request, callback) => {
        if (request.url === imageURL) {
          callback()
        } else {
          callback(filePath)
        }
      })
      await w.loadURL(origin)
    })

    it('resolves absolute resources', async () => {
      await registerStringProtocol(standardScheme, (request, callback) => {
        if (request.url === imageURL) {
          callback()
        } else {
          callback({
            data: fileContent,
            mimeType: 'text/html'
          })
        }
      })
      await w.loadURL(origin)
    })

    it('can have fetch working in it', async () => {
      const requestReceived = defer()
      const server = http.createServer((req, res) => {
        res.end()
        server.close()
        requestReceived.resolve()
      })
      await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
      const port = (server.address() as AddressInfo).port
      const content = `<script>fetch("http://127.0.0.1:${port}")</script>`
      await registerStringProtocol(standardScheme, (request, callback) => callback({ data: content, mimeType: 'text/html' }))
      await w.loadURL(origin)
      await requestReceived
    })

    it.skip('can access files through the FileSystem API', (done) => {
      const filePath = path.join(fixturesPath, 'pages', 'filesystem.html')
      protocol.registerFileProtocol(standardScheme, (request, callback) => callback({ path: filePath }), (error) => {
        if (error) return done(error)
        w.loadURL(origin)
      })
      ipcMain.once('file-system-error', (event, err) => done(err))
      ipcMain.once('file-system-write-end', () => done())
    })

    it('registers secure, when {secure: true}', (done) => {
      const filePath = path.join(fixturesPath, 'pages', 'cache-storage.html')
      ipcMain.once('success', () => done())
      ipcMain.once('failure', (event, err) => done(err))
      protocol.registerFileProtocol(standardScheme, (request, callback) => callback({ path: filePath }), (error) => {
        if (error) return done(error)
        w.loadURL(origin)
      })
    })
  })

  describe.skip('protocol.registerSchemesAsPrivileged cors-fetch', function () {
    const standardScheme = (global as any).standardScheme
    let w: BrowserWindow = null as unknown as BrowserWindow
    beforeEach(async () => {
      w = new BrowserWindow({ show: false })
    })

    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
      await Promise.all(
        [standardScheme, 'cors', 'no-cors', 'no-fetch'].map(scheme =>
          new Promise(resolve => protocol.unregisterProtocol(scheme, (/* ignore error */) => resolve()))
        )
      )
    })

    it('supports fetch api by default', async () => {
      const url = `file://${fixturesPath}/assets/logo.png`
      await w.loadURL(`file://${fixturesPath}/pages/blank.html`)
      const ok = await w.webContents.executeJavaScript(`fetch(${JSON.stringify(url)}).then(r => r.ok)`)
      expect(ok).to.be.true('response ok')
    })

    it('allows CORS requests by default', async () => {
      await allowsCORSRequests('cors', 200, new RegExp(''), () => {
        const { ipcRenderer } = require('electron')
        fetch('cors://myhost').then(function (response) {
          ipcRenderer.send('response', response.status)
        }).catch(function () {
          ipcRenderer.send('response', 'failed')
        })
      })
    })

    it('disallows CORS and fetch requests when only supportFetchAPI is specified', async () => {
      await allowsCORSRequests('no-cors', ['failed xhr', 'failed fetch'], /has been blocked by CORS policy/, () => {
        const { ipcRenderer } = require('electron')
        Promise.all([
          new Promise(resolve => {
            const req = new XMLHttpRequest()
            req.onload = () => resolve('loaded xhr')
            req.onerror = () => resolve('failed xhr')
            req.open('GET', 'no-cors://myhost')
            req.send()
          }),
          fetch('no-cors://myhost')
            .then(() => 'loaded fetch')
            .catch(() => 'failed fetch')
        ]).then(([xhr, fetch]) => {
          ipcRenderer.send('response', [xhr, fetch])
        })
      })
    })

    it('allows CORS, but disallows fetch requests, when specified', async () => {
      await allowsCORSRequests('no-fetch', ['loaded xhr', 'failed fetch'], /Fetch API cannot load/, () => {
        const { ipcRenderer } = require('electron')
        Promise.all([
          new Promise(resolve => {
            const req = new XMLHttpRequest()
            req.onload = () => resolve('loaded xhr')
            req.onerror = () => resolve('failed xhr')
            req.open('GET', 'no-fetch://myhost')
            req.send()
          }),
          fetch('no-fetch://myhost')
            .then(() => 'loaded fetch')
            .catch(() => 'failed fetch')
        ]).then(([xhr, fetch]) => {
          ipcRenderer.send('response', [xhr, fetch])
        })
      })
    })

    async function allowsCORSRequests (corsScheme: string, expected: any, expectedConsole: RegExp, content: Function) {
      await registerStringProtocol(standardScheme, (request, callback) => {
        callback({ data: `<script>(${content})()</script>`, mimeType: 'text/html' })
      })
      await registerStringProtocol(corsScheme, (request, callback) => {
        callback('')
      })

      const newContents: WebContents = (webContents as any).create({ nodeIntegration: true })
      const consoleMessages: string[] = []
      newContents.on('console-message', (e, level, message) => consoleMessages.push(message))
      try {
        newContents.loadURL(standardScheme + '://fake-host')
        const [, response] = await emittedOnce(ipcMain, 'response')
        expect(response).to.deep.equal(expected)
        expect(consoleMessages.join('\n')).to.match(expectedConsole)
      } finally {
        // This is called in a timeout to avoid a crash that happens when
        // calling destroy() in a microtask.
        setTimeout(() => {
          (newContents as any).destroy()
        })
      }
    }
  })
})
