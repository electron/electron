const chai = require('chai')
const http = require('http')
const https = require('https')
const path = require('path')
const fs = require('fs')
const send = require('send')
const auth = require('basic-auth')
const ChildProcess = require('child_process')
const { closeWindow } = require('./window-helpers')
const { emittedOnce } = require('./events-helpers')

const { session, BrowserWindow, net } = require('electron')
const { expect } = chai

/* The whole session API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('session module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures')
  let w = null
  const url = 'http://127.0.0.1'

  beforeEach(() => {
    w = new BrowserWindow({
      show: false,
      width: 400,
      height: 400,
      webPreferences: {
        nodeIntegration: true
      }
    })
  })

  afterEach(() => {
    return closeWindow(w).then(() => { w = null })
  })

  describe('session.defaultSession', () => {
    it('returns the default session', () => {
      expect(session.defaultSession).to.equal(session.fromPartition(''))
    })
  })

  describe('session.fromPartition(partition, options)', () => {
    it('returns existing session with same partition', () => {
      expect(session.fromPartition('test')).to.equal(session.fromPartition('test'))
    })

    it('created session is ref-counted', () => {
      const partition = 'test2'
      const userAgent = 'test-agent'
      const ses1 = session.fromPartition(partition)
      ses1.setUserAgent(userAgent)
      expect(ses1.getUserAgent()).to.equal(userAgent)
      ses1.destroy()
      const ses2 = session.fromPartition(partition)
      expect(ses2.getUserAgent()).to.not.equal(userAgent)
    })
  })

  describe('ses.cookies', () => {
    const name = '0'
    const value = '0'

    it('should get cookies', (done) => {
      const server = http.createServer((req, res) => {
        res.setHeader('Set-Cookie', [`${name}=${value}`])
        res.end('finished')
        server.close()
      })
      server.listen(0, '127.0.0.1', () => {
        w.webContents.once('did-finish-load', async () => {
          const list = await w.webContents.session.cookies.get({ url })
          const cookie = list.find(cookie => cookie.name === name)

          expect(cookie).to.exist.and.to.have.property('value', value)
          done()
        })
        const { port } = server.address()
        w.loadURL(`${url}:${port}`)
      })
    })

    it('sets cookies', async () => {
      const { cookies } = session.defaultSession
      const name = '1'
      const value = '1'

      await cookies.set({ url, name, value })
    })

    it('yields an error when setting a cookie with missing required fields', async () => {
      let error
      try {
        const { cookies } = session.defaultSession
        const name = '1'
        const value = '1'
        await cookies.set({ url: '', name, value })
      } catch (e) {
        error = e
      }
      expect(error).is.an('Error')
      expect(error).to.have.property('message').which.equals('Failed to get cookie domain')
    })

    it('should overwrite previous cookies', async () => {
      const { cookies } = session.defaultSession
      const name = 'DidOverwrite'
      for (const value of [ 'No', 'Yes' ]) {
        await cookies.set({ url, name, value, expirationDate: (+new Date()) / 1000 + 120 })
        const list = await cookies.get({ url })

        expect(list.some(cookie => cookie.name === name && cookie.value === value)).to.equal(true)
      }
    })

    it('should remove cookies', async () => {
      const { cookies } = session.defaultSession
      const name = '2'
      const value = '2'

      await cookies.set({ url, name, value, expirationDate: (+new Date()) / 1000 + 120 })
      await cookies.remove(url, name)
      const list = await cookies.get({ url })

      expect(list.some(cookie => cookie.name === name && cookie.value === value)).to.equal(false)
    })

    it('should set cookie for standard scheme', async () => {
      const { cookies } = session.defaultSession
      const standardScheme = global.standardScheme
      const domain = 'fake-host'
      const url = `${standardScheme}://${domain}`
      const name = 'custom'
      const value = '1'

      await cookies.set({ url, name, value, expirationDate: (+new Date()) / 1000 + 120 })
      const list = await cookies.get({ url })

      expect(list).to.have.lengthOf(1)
      expect(list[0]).to.have.property('name', name)
      expect(list[0]).to.have.property('value', value)
      expect(list[0]).to.have.property('domain', domain)
    })

    it('emits a changed event when a cookie is added or removed', async () => {
      const changes = []

      const { cookies } = session.fromPartition('cookies-changed')
      const name = 'foo'
      const value = 'bar'
      const listener = (event, cookie, cause, removed) => { changes.push({ cookie, cause, removed }) }

      const a = emittedOnce(cookies, 'changed')
      await cookies.set({ url, name, value, expirationDate: (+new Date()) / 1000 + 120 })
      const [, setEventCookie, setEventCause, setEventRemoved] = await a

      const b = emittedOnce(cookies, 'changed')
      await cookies.remove(url, name)
      const [, removeEventCookie, removeEventCause, removeEventRemoved] = await b

      expect(setEventCookie.name).to.equal(name)
      expect(setEventCookie.value).to.equal(value)
      expect(setEventCause).to.equal('explicit')
      expect(setEventRemoved).to.equal(false)

      expect(removeEventCookie.name).to.equal(name)
      expect(removeEventCookie.value).to.equal(value)
      expect(removeEventCause).to.equal('explicit')
      expect(removeEventRemoved).to.equal(true)
    })

    describe('ses.cookies.flushStore()', async () => {
      describe('flushes the cookies to disk', async () => {
        const name = 'foo'
        const value = 'bar'
        const { cookies } = session.defaultSession

        await cookies.set({ url, name, value })
        await cookies.flushStore()
      })
    })

    it('should survive an app restart for persistent partition', async () => {
      const appPath = path.join(fixtures, 'api', 'cookie-app')
      const electronPath = process.execPath

      const test = (result, phase) => {
        return new Promise((resolve, reject) => {
          let output = ''

          const appProcess = ChildProcess.spawn(
            electronPath,
            [appPath],
            { env: { PHASE: phase, ...process.env } }
          )

          appProcess.stdout.on('data', data => { output += data })
          appProcess.stdout.on('end', () => {
            output = output.replace(/(\r\n|\n|\r)/gm, '')
            expect(output).to.equal(result)
            resolve()
          })
        })
      }

      await test('011', 'one')
      await test('110', 'two')
    })
  })

  describe('ses.clearStorageData(options)', () => {
    it('clears localstorage data', async () => {
      const { ipcMain } = require('electron')
      await w.loadFile(path.join(fixtures, 'api', 'localstorage.html'))
      const options = {
        origin: 'file://',
        storages: ['localstorage'],
        quotas: ['persistent']
      }
      await w.webContents.session.clearStorageData(options)
      while (await w.webContents.executeJavaScript('localStorage.length') !== 0) {
        // The storage clear isn't instantly visible to the renderer, so keep
        // trying until it is.
      }
    })
  })

  describe('will-download event', () => {
    beforeEach(() => {
      if (w != null) w.destroy()
      w = new BrowserWindow({
        show: false,
        width: 400,
        height: 400
      })
    })

    it('can cancel default download behavior', (done) => {
      const mockFile = Buffer.alloc(1024)
      const contentDisposition = 'inline; filename="mockFile.txt"'
      const downloadServer = http.createServer((req, res) => {
        res.writeHead(200, {
          'Content-Length': mockFile.length,
          'Content-Type': 'application/plain',
          'Content-Disposition': contentDisposition
        })
        res.end(mockFile)
        downloadServer.close()
      })

      downloadServer.listen(0, '127.0.0.1', () => {
        const port = downloadServer.address().port
        const url = `http://127.0.0.1:${port}/`

        w.webContents.session.once('will-download', function (e, item) {
          e.preventDefault()
          expect(item.getURL()).to.equal(url)
          expect(item.getFilename()).to.equal('mockFile.txt')
          setImmediate(() => {
            expect(() => item.getURL()).to.throw('Object has been destroyed')
            done()
          })
        })
        w.loadURL(url)
      })
    })
  })

  describe('ses.protocol', () => {
    const partitionName = 'temp'
    const protocolName = 'sp'
    let customSession = null
    const protocol = session.defaultSession.protocol
    const handler = (ignoredError, callback) => {
      callback({ data: 'test', mimeType: 'text/html' })
    }

    beforeEach((done) => {
      if (w != null) w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: partitionName
        }
      })
      customSession = session.fromPartition(partitionName)
      customSession.protocol.registerStringProtocol(protocolName, handler, (error) => {
        done(error != null ? error : undefined)
      })
    })

    afterEach((done) => {
      customSession.protocol.unregisterProtocol(protocolName, () => done())
      customSession = null
    })

    it('does not affect defaultSession', async () => {
      const result1 = await protocol.isProtocolHandled(protocolName)
      expect(result1).to.equal(false)

      const result2 = await customSession.protocol.isProtocolHandled(protocolName)
      expect(result2).to.equal(true)
    })

    it('handles requests from partition', async () => {
      await w.loadURL(`${protocolName}://fake-host`)
      expect(await w.webContents.executeJavaScript('document.body.textContent')).to.equal('test')
    })
  })

  describe('ses.setProxy(options)', () => {
    let server = null
    let customSession = null

    beforeEach((done) => {
      customSession = session.fromPartition('proxyconfig')
      // FIXME(deepak1556): This is just a hack to force
      // creation of request context which in turn initializes
      // the network context, can be removed with network
      // service enabled.
      customSession.clearHostResolverCache().then(() => done())
    })

    afterEach(() => {
      if (server) {
        server.close()
      }
      if (customSession) {
        customSession.destroy()
      }
    })

    it('allows configuring proxy settings', async () => {
      const config = { proxyRules: 'http=myproxy:80' }
      await customSession.setProxy(config)
      const proxy = await customSession.resolveProxy('http://example.com/')
      expect(proxy).to.equal('PROXY myproxy:80')
    })

    it('allows removing the implicit bypass rules for localhost', async () => {
      const config = {
        proxyRules: 'http=myproxy:80',
        proxyBypassRules: '<-loopback>'
      }

      await customSession.setProxy(config)
      const proxy = await customSession.resolveProxy('http://localhost')
      expect(proxy).to.equal('PROXY myproxy:80')
    })

    it('allows configuring proxy settings with pacScript', async () => {
      server = http.createServer((req, res) => {
        const pac = `
          function FindProxyForURL(url, host) {
            return "PROXY myproxy:8132";
          }
        `
        res.writeHead(200, {
          'Content-Type': 'application/x-ns-proxy-autoconfig'
        })
        res.end(pac)
      })
      return new Promise((resolve, reject) => {
        server.listen(0, '127.0.0.1', async () => {
          try {
            const config = { pacScript: `http://127.0.0.1:${server.address().port}` }
            await customSession.setProxy(config)
            const proxy = await customSession.resolveProxy('https://google.com')
            expect(proxy).to.equal('PROXY myproxy:8132')
            resolve()
          } catch (error) {
            reject(error)
          }
        })
      })
    })

    it('allows bypassing proxy settings', async () => {
      const config = {
        proxyRules: 'http=myproxy:80',
        proxyBypassRules: '<local>'
      }
      await customSession.setProxy(config)
      const proxy = await customSession.resolveProxy('http://example/')
      expect(proxy).to.equal('DIRECT')
    })
  })

  describe('ses.getBlobData()', () => {
    it('returns blob data for uuid', (done) => {
      const scheme = 'cors-blob'
      const protocol = session.defaultSession.protocol
      const url = `${scheme}://host`
      before(() => {
        if (w != null) w.destroy()
        w = new BrowserWindow({ show: false })
      })

      after((done) => {
        protocol.unregisterProtocol(scheme, () => {
          closeWindow(w).then(() => {
            w = null
            done()
          })
        })
      })

      const postData = JSON.stringify({
        type: 'blob',
        value: 'hello'
      })
      const content = `<html>
                       <script>
                       let fd = new FormData();
                       fd.append('file', new Blob(['${postData}'], {type:'application/json'}));
                       fetch('${url}', {method:'POST', body: fd });
                       </script>
                       </html>`

      protocol.registerStringProtocol(scheme, (request, callback) => {
        if (request.method === 'GET') {
          callback({ data: content, mimeType: 'text/html' })
        } else if (request.method === 'POST') {
          const uuid = request.uploadData[1].blobUUID
          expect(uuid).to.be.a('string')
          session.defaultSession.getBlobData(uuid).then(result => {
            expect(result.toString()).to.equal(postData)
            done()
          })
        }
      }, (error) => {
        if (error) return done(error)
        w.loadURL(url)
      })
    })
  })

  describe('ses.setCertificateVerifyProc(callback)', (done) => {
    let server = null

    beforeEach((done) => {
      const certPath = path.join(fixtures, 'certificates')
      const options = {
        key: fs.readFileSync(path.join(certPath, 'server.key')),
        cert: fs.readFileSync(path.join(certPath, 'server.pem')),
        ca: [
          fs.readFileSync(path.join(certPath, 'rootCA.pem')),
          fs.readFileSync(path.join(certPath, 'intermediateCA.pem'))
        ],
        requestCert: true,
        rejectUnauthorized: false
      }

      server = https.createServer(options, (req, res) => {
        res.writeHead(200)
        res.end('<title>hello</title>')
      })
      server.listen(0, '127.0.0.1', done)
    })

    afterEach(() => {
      session.defaultSession.setCertificateVerifyProc(null)
      server.close()
    })

    it('accepts the request when the callback is called with 0', (done) => {
      session.defaultSession.setCertificateVerifyProc(({ hostname, certificate, verificationResult, errorCode }, callback) => {
        expect(['net::ERR_CERT_AUTHORITY_INVALID', 'net::ERR_CERT_COMMON_NAME_INVALID'].includes(verificationResult)).to.be.true
        expect([-202, -200].includes(errorCode)).to.be.true
        callback(0)
      })

      w.webContents.once('did-finish-load', () => {
        expect(w.webContents.getTitle()).to.equal('hello')
        done()
      })
      w.loadURL(`https://127.0.0.1:${server.address().port}`)
    })

    it('rejects the request when the callback is called with -2', (done) => {
      session.defaultSession.setCertificateVerifyProc(({ hostname, certificate, verificationResult }, callback) => {
        expect(hostname).to.equal('127.0.0.1')
        expect(certificate.issuerName).to.equal('Intermediate CA')
        expect(certificate.subjectName).to.equal('localhost')
        expect(certificate.issuer.commonName).to.equal('Intermediate CA')
        expect(certificate.subject.commonName).to.equal('localhost')
        expect(certificate.issuerCert.issuer.commonName).to.equal('Root CA')
        expect(certificate.issuerCert.subject.commonName).to.equal('Intermediate CA')
        expect(certificate.issuerCert.issuerCert.issuer.commonName).to.equal('Root CA')
        expect(certificate.issuerCert.issuerCert.subject.commonName).to.equal('Root CA')
        expect(certificate.issuerCert.issuerCert.issuerCert).to.equal(undefined)
        expect(['net::ERR_CERT_AUTHORITY_INVALID', 'net::ERR_CERT_COMMON_NAME_INVALID'].includes(verificationResult)).to.be.true
        callback(-2)
      })

      const url = `https://127.0.0.1:${server.address().port}`
      w.webContents.once('did-finish-load', () => {
        expect(w.webContents.getTitle()).to.equal(url)
        done()
      })
      w.loadURL(url)
    })
  })

  describe('ses.clearAuthCache(options)', () => {
    it('can clear http auth info from cache', (done) => {
      const ses = session.fromPartition('auth-cache')
      const server = http.createServer((req, res) => {
        const credentials = auth(req)
        if (!credentials || credentials.name !== 'test' || credentials.pass !== 'test') {
          res.statusCode = 401
          res.setHeader('WWW-Authenticate', 'Basic realm="Restricted"')
          res.end()
        } else {
          res.end('authenticated')
        }
      })
      server.listen(0, '127.0.0.1', () => {
        const port = server.address().port
        function issueLoginRequest (attempt = 1) {
          if (attempt > 2) {
            server.close()
            return done()
          }
          const request = net.request({
            url: `http://127.0.0.1:${port}`,
            session: ses
          })
          request.on('login', (info, callback) => {
            attempt += 1
            expect(info.scheme).to.equal('basic')
            expect(info.realm).to.equal('Restricted')
            callback('test', 'test')
          })
          request.on('response', (response) => {
            let data = ''
            response.pause()
            response.on('data', (chunk) => {
              data += chunk
            })
            response.on('end', () => {
              expect(data).to.equal('authenticated')
              ses.clearAuthCache({ type: 'password' }).then(() => {
                issueLoginRequest(attempt)
              })
            })
            response.on('error', (error) => { done(error) })
            response.resume()
          })
          // Internal api to bypass cache for testing.
          request.urlRequest._setLoadFlags(1 << 1)
          request.end()
        }
        issueLoginRequest()
      })
    })
  })
})
