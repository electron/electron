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

const { session, BrowserWindow, net, ipcMain } = require('electron')
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
        nodeIntegration: true,
        webviewTag: true,
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

    // TODO(codebytere): remove in Electron v8.0.0
    it('created session is ref-counted (functions)', () => {
      const partition = 'test2'
      const userAgent = 'test-agent'
      const ses1 = session.fromPartition(partition)
      ses1.setUserAgent(userAgent)
      expect(ses1.getUserAgent()).to.equal(userAgent)
      ses1.destroy()
      const ses2 = session.fromPartition(partition)
      expect(ses2.getUserAgent()).to.not.equal(userAgent)
    })

    it('created session is ref-counted', () => {
      const partition = 'test2'
      const userAgent = 'test-agent'
      const ses1 = session.fromPartition(partition)
      ses1.userAgent = userAgent
      expect(ses1.userAgent).to.equal(userAgent)
      ses1.destroy()
      const ses2 = session.fromPartition(partition)
      expect(ses2.userAgent).to.not.equal(userAgent)
    })
  })

  describe('ses.cookies', () => {
    const name = '0'
    const value = '0'

    it('should get cookies', async () => {
      const server = http.createServer((req, res) => {
        res.setHeader('Set-Cookie', [`${name}=${value}`])
        res.end('finished')
        server.close()
      })
      await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
      const { port } = server.address()
      await w.loadURL(`${url}:${port}`)
      const list = await w.webContents.session.cookies.get({ url })
      const cookie = list.find(cookie => cookie.name === name)
      expect(cookie).to.exist.and.to.have.property('value', value)
    })

    it('sets cookies', async () => {
      const { cookies } = session.defaultSession
      const name = '1'
      const value = '1'

      await cookies.set({ url, name, value, expirationDate: (+new Date) / 1000 + 120 })
      const cs = await cookies.get({ url })
      expect(cs.some(c => c.name === name && c.value === value)).to.equal(true)
    })

    it('yields an error when setting a cookie with missing required fields', async () => {
      const { cookies } = session.defaultSession
      const name = '1'
      const value = '1'

      await expect(
        cookies.set({ url: '', name, value })
      ).to.eventually.be.rejectedWith('Failed to get cookie domain')
    })

    it('yields an error when setting a cookie with an invalid URL', async () => {
      const { cookies } = session.defaultSession
      const name = '1'
      const value = '1'

      await expect(
        cookies.set({ url: 'asdf', name, value })
      ).to.eventually.be.rejectedWith('Failed to get cookie domain')
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
      it('flushes the cookies to disk', async () => {
        const name = 'foo'
        const value = 'bar'
        const { cookies } = session.defaultSession

        await cookies.set({ url, name, value })
        await cookies.flushStore()
      })
    })

    it('should survive an app restart for persistent partition', async () => {
      const appPath = path.join(fixtures, 'api', 'cookie-app')

      const runAppWithPhase = (phase) => {
        return new Promise((resolve, reject) => {
          let output = ''

          const appProcess = ChildProcess.spawn(
            process.execPath,
            [appPath],
            { env: { PHASE: phase, ...process.env } }
          )

          appProcess.stdout.on('data', data => { output += data })
          appProcess.stdout.on('end', () => {
            resolve(output.replace(/(\r\n|\n|\r)/gm, ''))
          })
        })
      }

      expect(await runAppWithPhase('one')).to.equal('011')
      expect(await runAppWithPhase('two')).to.equal('110')
    })
  })

  describe('ses.clearStorageData(options)', () => {
    it('clears localstorage data', async () => {
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
    it('can cancel default download behavior', async () => {
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
      await new Promise(resolve => downloadServer.listen(0, '127.0.0.1', resolve))

      const port = downloadServer.address().port
      const url = `http://127.0.0.1:${port}/`

      const downloadPrevented = new Promise(resolve => {
        w.webContents.session.once('will-download', function (e, item) {
          e.preventDefault()
          resolve(item)
        })
      })
      w.loadURL(url)
      const item = await downloadPrevented
      expect(item.getURL()).to.equal(url)
      expect(item.getFilename()).to.equal('mockFile.txt')
      await new Promise(setImmediate)
      expect(() => item.getURL()).to.throw('Object has been destroyed')
    })
  })

  describe('ses.protocol', () => {
    const partitionName = 'temp'
    const protocolName = 'sp'
    let customSession = null
    const protocol = session.defaultSession.protocol
    const handler = (ignoredError, callback) => {
      callback({ data: `<script>require('electron').ipcRenderer.send('hello')</script>`, mimeType: 'text/html' })
    }

    beforeEach(async () => {
      if (w != null) w.destroy()
      w = new BrowserWindow({
        show: false,
        webPreferences: {
          partition: partitionName,
          nodeIntegration: true,
        }
      })
      customSession = session.fromPartition(partitionName)
      await customSession.protocol.registerStringProtocol(protocolName, handler)
    })

    afterEach(async () => {
      await customSession.protocol.unregisterProtocol(protocolName)
      customSession = null
    })

    it('does not affect defaultSession', async () => {
      const result1 = await protocol.isProtocolHandled(protocolName)
      expect(result1).to.equal(false)

      const result2 = await customSession.protocol.isProtocolHandled(protocolName)
      expect(result2).to.equal(true)
    })

    it('handles requests from partition', async () => {
      w.loadURL(`${protocolName}://fake-host`)
      await emittedOnce(ipcMain, 'hello')
    })
  })

  describe('ses.setProxy(options)', () => {
    let server = null
    let customSession = null

    beforeEach(async () => {
      customSession = session.fromPartition('proxyconfig')
      // FIXME(deepak1556): This is just a hack to force
      // creation of request context which in turn initializes
      // the network context, can be removed with network
      // service enabled.
      await customSession.clearHostResolverCache()
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
      await new Promise(resolve => server.listen(0, '127.0.0.1', resolve))
      const config = { pacScript: `http://127.0.0.1:${server.address().port}` }
      await customSession.setProxy(config)
      const proxy = await customSession.resolveProxy('https://google.com')
      expect(proxy).to.equal('PROXY myproxy:8132')
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
    const scheme = 'cors-blob'
    const protocol = session.defaultSession.protocol
    const url = `${scheme}://host`
    after(async () => {
      await protocol.unregisterProtocol(scheme)
    })

    it('returns blob data for uuid', (done) => {
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

    afterEach((done) => {
      session.defaultSession.setCertificateVerifyProc(null)
      server.close(done)
    })

    it('accepts the request when the callback is called with 0', async () => {
      session.defaultSession.setCertificateVerifyProc(({ hostname, certificate, verificationResult, errorCode }, callback) => {
        expect(['net::ERR_CERT_AUTHORITY_INVALID', 'net::ERR_CERT_COMMON_NAME_INVALID'].includes(verificationResult)).to.be.true
        expect([-202, -200].includes(errorCode)).to.be.true
        callback(0)
      })

      await w.loadURL(`https://127.0.0.1:${server.address().port}`)
      expect(w.webContents.getTitle()).to.equal('hello')
    })

    it('rejects the request when the callback is called with -2', async () => {
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
      await expect(w.loadURL(url)).to.eventually.be.rejectedWith(/ERR_FAILED/)
      expect(w.webContents.getTitle()).to.equal(url)
    })

    it('saves cached results', async () => {
      let numVerificationRequests = 0
      session.defaultSession.setCertificateVerifyProc(({ hostname, certificate, verificationResult }, callback) => {
        numVerificationRequests++
        callback(-2)
      })

      const url = `https://127.0.0.1:${server.address().port}`
      await expect(w.loadURL(url), 'first load').to.eventually.be.rejectedWith(/ERR_FAILED/)
      await emittedOnce(w.webContents, 'did-stop-loading')
      await expect(w.loadURL(url + '/test'), 'second load').to.eventually.be.rejectedWith(/ERR_FAILED/)
      expect(w.webContents.getTitle()).to.equal(url + '/test')

      // TODO(nornagon): there's no way to check if the network service is
      // enabled from JS, so once we switch it on by default just change this
      // test :)
      const networkServiceEnabled = false
      expect(numVerificationRequests).to.equal(networkServiceEnabled ? 1 : 2)
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

  describe('DownloadItem', () => {
    const mockPDF = Buffer.alloc(1024 * 1024 * 5)
    const downloadFilePath = path.join(__dirname, '..', 'fixtures', 'mock.pdf')
    const protocolName = 'custom-dl'
    const contentDisposition = 'inline; filename="mock.pdf"'
    let address = null
    let downloadServer = null
    before(async () => {
      downloadServer = http.createServer((req, res) => {
        address = downloadServer.address()
        res.writeHead(200, {
          'Content-Length': mockPDF.length,
          'Content-Type': 'application/pdf',
          'Content-Disposition': req.url === '/?testFilename' ? 'inline' : contentDisposition
        })
        res.end(mockPDF)
      })
      await new Promise(resolve => downloadServer.listen(0, '127.0.0.1', resolve))
    })
    after(async () => {
      await new Promise(resolve => downloadServer.close(resolve))
    })

    const isPathEqual = (path1, path2) => {
      return path.relative(path1, path2) === ''
    }
    const assertDownload = (state, item, isCustom = false) => {
      expect(state).to.equal('completed')
      expect(item.getFilename()).to.equal('mock.pdf')
      expect(path.isAbsolute(item.savePath)).to.equal(true)
      expect(isPathEqual(item.savePath, downloadFilePath)).to.equal(true)
      if (isCustom) {
        expect(item.getURL()).to.equal(`${protocolName}://item`)
      } else {
        expect(item.getURL()).to.be.equal(`${url}:${address.port}/`)
      }
      expect(item.getMimeType()).to.equal('application/pdf')
      expect(item.getReceivedBytes()).to.equal(mockPDF.length)
      expect(item.getTotalBytes()).to.equal(mockPDF.length)
      expect(item.getContentDisposition()).to.equal(contentDisposition)
      expect(fs.existsSync(downloadFilePath)).to.equal(true)
      fs.unlinkSync(downloadFilePath)
    }

    it('can download using WebContents.downloadURL', (done) => {
      const port = downloadServer.address().port
      w.webContents.session.once('will-download', function (e, item) {
        item.savePath = downloadFilePath
        item.on('done', function (e, state) {
          assertDownload(state, item)
          done()
        })
      })
      w.webContents.downloadURL(`${url}:${port}`)
    })

    it('can download from custom protocols using WebContents.downloadURL', (done) => {
      const protocol = session.defaultSession.protocol
      const port = downloadServer.address().port
      const handler = (ignoredError, callback) => {
        callback({ url: `${url}:${port}` })
      }
      protocol.registerHttpProtocol(protocolName, handler, (error) => {
        if (error) return done(error)
        w.webContents.session.once('will-download', function (e, item) {
          item.savePath = downloadFilePath
          item.on('done', function (e, state) {
            assertDownload(state, item, true)
            done()
          })
        })
        w.webContents.downloadURL(`${protocolName}://item`)
      })
    })

    it('can download using WebView.downloadURL', async () => {
      const port = downloadServer.address().port
      await w.loadURL('about:blank')
      function webviewDownload({fixtures, url, port}) {
        const webview = new WebView()
        webview.addEventListener('did-finish-load', () => {
          webview.downloadURL(`${url}:${port}/`)
        })
        webview.src = `file://${fixtures}/api/blank.html`
        document.body.appendChild(webview)
      }
      const done = new Promise(resolve => {
        w.webContents.session.once('will-download', function (e, item) {
          item.savePath = downloadFilePath
          item.on('done', function (e, state) {
            resolve([state, item])
          })
        })
      })
      await w.webContents.executeJavaScript(`(${webviewDownload})(${JSON.stringify({fixtures, url, port})})`)
      const [state, item] = await done
      assertDownload(state, item)
    })

    it('can cancel download', (done) => {
      const port = downloadServer.address().port
      w.webContents.session.once('will-download', function (e, item) {
        item.savePath = downloadFilePath
        item.on('done', function (e, state) {
          expect(state).to.equal('cancelled')
          expect(item.getFilename()).to.equal('mock.pdf')
          expect(item.getMimeType()).to.equal('application/pdf')
          expect(item.getReceivedBytes()).to.equal(0)
          expect(item.getTotalBytes()).to.equal(mockPDF.length)
          expect(item.getContentDisposition()).to.equal(contentDisposition)
          done()
        })
        item.cancel()
      })
      w.webContents.downloadURL(`${url}:${port}/`)
    })

    it('can generate a default filename', function (done) {
      if (process.env.APPVEYOR === 'True') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      const port = downloadServer.address().port
      w.webContents.session.once('will-download', function (e, item) {
        item.savePath = downloadFilePath
        item.on('done', function (e, state) {
          expect(item.getFilename()).to.equal('download.pdf')
          done()
        })
        item.cancel()
      })
      w.webContents.downloadURL(`${url}:${port}/?testFilename`)
    })

    it('can set options for the save dialog', (done) => {
      const filePath = path.join(__dirname, 'fixtures', 'mock.pdf')
      const port = downloadServer.address().port
      const options = {
        window: null,
        title: 'title',
        message: 'message',
        buttonLabel: 'buttonLabel',
        nameFieldLabel: 'nameFieldLabel',
        defaultPath: '/',
        filters: [{
          name: '1', extensions: ['.1', '.2']
        }, {
          name: '2', extensions: ['.3', '.4', '.5']
        }],
        showsTagField: true,
        securityScopedBookmarks: true
      }

      w.webContents.session.once('will-download', function (e, item) {
        item.setSavePath(filePath)
        item.setSaveDialogOptions(options)
        item.on('done', function (e, state) {
          expect(item.getSaveDialogOptions()).to.deep.equal(options)
          done()
        })
        item.cancel()
      })
      w.webContents.downloadURL(`${url}:${port}`)
    })

    describe('when a save path is specified and the URL is unavailable', () => {
      it('does not display a save dialog and reports the done state as interrupted', (done) => {
        w.webContents.session.once('will-download', function (e, item) {
          item.savePath = downloadFilePath
          if (item.getState() === 'interrupted') {
            item.resume()
          }
          item.on('done', function (e, state) {
            expect(state).to.equal('interrupted')
            done()
          })
        })
        w.webContents.downloadURL(`file://${path.join(__dirname, 'does-not-exist.txt')}`)
      })
    })
  })

  describe('ses.createInterruptedDownload(options)', () => {
    it('can create an interrupted download item', (done) => {
      const downloadFilePath = path.join(__dirname, '..', 'fixtures', 'mock.pdf')
      const options = {
        path: downloadFilePath,
        urlChain: ['http://127.0.0.1/'],
        mimeType: 'application/pdf',
        offset: 0,
        length: 5242880
      }
      w.webContents.session.once('will-download', function (e, item) {
        expect(item.getState()).to.equal('interrupted')
        item.cancel()
        expect(item.getURLChain()).to.deep.equal(options.urlChain)
        expect(item.getMimeType()).to.equal(options.mimeType)
        expect(item.getReceivedBytes()).to.equal(options.offset)
        expect(item.getTotalBytes()).to.equal(options.length)
        expect(item.savePath).to.equal(downloadFilePath)
        done()
      })
      w.webContents.session.createInterruptedDownload(options)
    })

    it('can be resumed', async () => {
      const downloadFilePath = path.join(fixtures, 'logo.png')
      const rangeServer = http.createServer((req, res) => {
        const options = { root: fixtures }
        send(req, req.url, options)
          .on('error', (error) => { done(error) }).pipe(res)
      })
      try {
        await new Promise(resolve => rangeServer.listen(0, '127.0.0.1', resolve))
        const port = rangeServer.address().port
        const downloadCancelled = new Promise((resolve) => {
          w.webContents.session.once('will-download', function (e, item) {
            item.setSavePath(downloadFilePath)
            item.on('done', function (e, state) {
              resolve(item)
            })
            item.cancel()
          })
        })
        const downloadUrl = `http://127.0.0.1:${port}/assets/logo.png`
        w.webContents.downloadURL(downloadUrl)
        const item = await downloadCancelled
        expect(item.getState()).to.equal('cancelled')

        const options = {
          path: item.savePath,
          urlChain: item.getURLChain(),
          mimeType: item.getMimeType(),
          offset: item.getReceivedBytes(),
          length: item.getTotalBytes(),
          lastModified: item.getLastModifiedTime(),
          eTag: item.getETag(),
        }
        const downloadResumed = new Promise((resolve) => {
          w.webContents.session.once('will-download', function (e, item) {
            expect(item.getState()).to.equal('interrupted')
            item.setSavePath(downloadFilePath)
            item.resume()
            item.on('done', function (e, state) {
              resolve(item)
            })
          })
        })
        w.webContents.session.createInterruptedDownload(options)
        const completedItem = await downloadResumed
        expect(completedItem.getState()).to.equal('completed')
        expect(completedItem.getFilename()).to.equal('logo.png')
        expect(completedItem.savePath).to.equal(downloadFilePath)
        expect(completedItem.getURL()).to.equal(downloadUrl)
        expect(completedItem.getMimeType()).to.equal('image/png')
        expect(completedItem.getReceivedBytes()).to.equal(14022)
        expect(completedItem.getTotalBytes()).to.equal(14022)
        expect(fs.existsSync(downloadFilePath)).to.equal(true)
      } finally {
        rangeServer.close()
      }
    })
  })

  describe('ses.setPermissionRequestHandler(handler)', () => {
    it('cancels any pending requests when cleared', async () => {
      const ses = w.webContents.session
      ses.setPermissionRequestHandler(() => {
        ses.setPermissionRequestHandler(null)
      })

      const result = emittedOnce(require('electron').ipcMain, 'message')

      function remote() {
        navigator.requestMIDIAccess({sysex: true}).then(() => {}, (err) => {
          require('electron').ipcRenderer.send('message', err.name);
        });
      }

      await w.loadURL(`data:text/html,<script>(${remote})()</script>`)

      const [,name] = await result
      expect(name).to.deep.equal('SecurityError')
    })
  })
})
