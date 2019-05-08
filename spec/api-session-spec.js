const chai = require('chai')
const http = require('http')
const https = require('https')
const path = require('path')
const fs = require('fs')
const send = require('send')
const auth = require('basic-auth')
const ChildProcess = require('child_process')
const { closeWindow } = require('./window-helpers')

const { ipcRenderer, remote } = require('electron')
const { ipcMain, session, BrowserWindow, net } = remote
const { expect } = chai

/* The whole session API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('session module', () => {
  let fixtures = path.resolve(__dirname, 'fixtures')
  let w = null
  let webview = null
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
    if (webview != null) {
      if (!document.body.contains(webview)) {
        document.body.appendChild(webview)
      }
      webview.remove()
    }

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

        expect(list.some(cookie => cookie.name === name && cookie.value === value)).to.be.true()
      }
    })

    it('should remove cookies', async () => {
      const { cookies } = session.defaultSession
      const name = '2'
      const value = '2'

      await cookies.set({ url, name, value, expirationDate: (+new Date()) / 1000 + 120 })
      await cookies.remove(url, name)
      const list = await cookies.get({ url })

      expect(list.some(cookie => cookie.name === name && cookie.value === value)).to.be.false()
    })

    it('should set cookie for standard scheme', async () => {
      const { cookies } = session.defaultSession
      const standardScheme = remote.getGlobal('standardScheme')
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
      const eventName = 'changed'
      const listener = (event, cookie, cause, removed) => { changes.push({ cookie, cause, removed }) }

      cookies.on(eventName, listener)
      await cookies.set({ url, name, value, expirationDate: (+new Date()) / 1000 + 120 })
      await cookies.remove(url, name)
      cookies.off(eventName, listener)

      expect(changes).to.have.lengthOf(2)
      expect(changes.every(change => change.cookie.name === name))
      expect(changes.every(change => change.cookie.value === value))
      expect(changes.every(change => change.cause === 'explicit'))
      expect(changes[0].removed).to.eql(false)
      expect(changes[1].removed).to.eql(true)
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
      const appPath = path.join(__dirname, 'fixtures', 'api', 'cookie-app')
      const electronPath = remote.getGlobal('process').execPath

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
    fixtures = path.resolve(__dirname, 'fixtures')
    it('clears localstorage data', (done) => {
      ipcMain.on('count', (event, count) => {
        ipcMain.removeAllListeners('count')
        expect(count).to.equal(0)
        done()
      })
      w.webContents.on('did-finish-load', () => {
        const options = {
          origin: 'file://',
          storages: ['localstorage'],
          quotas: ['persistent']
        }
        w.webContents.session.clearStorageData(options).then(() => {
          w.webContents.send('getcount')
        })
      })
      w.loadFile(path.join(fixtures, 'api', 'localstorage.html'))
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

        ipcRenderer.sendSync('set-download-option', false, true)
        w.loadURL(url)
        ipcRenderer.once('download-error', (event, downloadUrl, filename, error) => {
          expect(downloadUrl).to.equal(url)
          expect(filename).to.equal('mockFile.txt')
          expect(error).to.equal('Object has been destroyed')
          done()
        })
      })
    })
  })

  describe('DownloadItem', () => {
    const mockPDF = Buffer.alloc(1024 * 1024 * 5)
    const protocolName = 'custom-dl'
    let contentDisposition = 'inline; filename="mock.pdf"'
    const downloadFilePath = path.join(fixtures, 'mock.pdf')
    const downloadServer = http.createServer((req, res) => {
      if (req.url === '/?testFilename') contentDisposition = 'inline'
      res.writeHead(200, {
        'Content-Length': mockPDF.length,
        'Content-Type': 'application/pdf',
        'Content-Disposition': contentDisposition
      })
      res.end(mockPDF)
      downloadServer.close()
    })

    const isPathEqual = (path1, path2) => {
      return path.relative(path1, path2) === ''
    }
    const assertDownload = (event, state, url, mimeType,
      receivedBytes, totalBytes, disposition,
      filename, port, savePath, isCustom) => {
      expect(state).to.equal('completed')
      expect(filename).to.equal('mock.pdf')
      expect(path.isAbsolute(savePath)).to.be.true()
      expect(isPathEqual(savePath, path.join(__dirname, 'fixtures', 'mock.pdf'))).to.be.true()
      if (isCustom) {
        expect(url).to.be.equal(`${protocolName}://item`)
      } else {
        expect(url).to.be.equal(`http://127.0.0.1:${port}/`)
      }
      expect(mimeType).to.equal('application/pdf')
      expect(receivedBytes).to.equal(mockPDF.length)
      expect(totalBytes).to.equal(mockPDF.length)
      expect(disposition).to.equal(contentDisposition)
      expect(fs.existsSync(downloadFilePath)).to.be.true()
      fs.unlinkSync(downloadFilePath)
    }

    it('can download using WebContents.downloadURL', (done) => {
      downloadServer.listen(0, '127.0.0.1', () => {
        const port = downloadServer.address().port
        ipcRenderer.sendSync('set-download-option', false, false)
        w.webContents.downloadURL(`${url}:${port}`)
        ipcRenderer.once('download-done', (event, state, url,
          mimeType, receivedBytes,
          totalBytes, disposition,
          filename, savePath) => {
          assertDownload(event, state, url, mimeType, receivedBytes,
            totalBytes, disposition, filename, port, savePath)
          done()
        })
      })
    })

    it('can download from custom protocols using WebContents.downloadURL', (done) => {
      const protocol = session.defaultSession.protocol
      downloadServer.listen(0, '127.0.0.1', () => {
        const port = downloadServer.address().port
        const handler = (ignoredError, callback) => {
          callback({ url: `${url}:${port}` })
        }
        protocol.registerHttpProtocol(protocolName, handler, (error) => {
          if (error) return done(error)
          ipcRenderer.sendSync('set-download-option', false, false)
          w.webContents.downloadURL(`${protocolName}://item`)
          ipcRenderer.once('download-done', (event, state, url,
            mimeType, receivedBytes,
            totalBytes, disposition,
            filename, savePath) => {
            assertDownload(event, state, url, mimeType, receivedBytes,
              totalBytes, disposition, filename, port, savePath,
              true)
            done()
          })
        })
      })
    })

    it('can download using WebView.downloadURL', (done) => {
      downloadServer.listen(0, '127.0.0.1', () => {
        const port = downloadServer.address().port
        ipcRenderer.sendSync('set-download-option', false, false)
        webview = new WebView()
        webview.addEventListener('did-finish-load', () => {
          webview.downloadURL(`${url}:${port}/`)
        })
        webview.src = `file://${fixtures}/api/blank.html`
        ipcRenderer.once('download-done', (event, state, url,
          mimeType, receivedBytes,
          totalBytes, disposition,
          filename, savePath) => {
          assertDownload(event, state, url, mimeType, receivedBytes,
            totalBytes, disposition, filename, port, savePath)
          document.body.removeChild(webview)
          done()
        })
        document.body.appendChild(webview)
      })
    })

    it('can cancel download', (done) => {
      downloadServer.listen(0, '127.0.0.1', () => {
        const port = downloadServer.address().port
        ipcRenderer.sendSync('set-download-option', true, false)
        w.webContents.downloadURL(`${url}:${port}/`)
        ipcRenderer.once('download-done', (event, state, url,
          mimeType, receivedBytes,
          totalBytes, disposition,
          filename) => {
          expect(state).to.equal('cancelled')
          expect(filename).to.equal('mock.pdf')
          expect(mimeType).to.equal('application/pdf')
          expect(receivedBytes).to.equal(0)
          expect(totalBytes).to.equal(mockPDF.length)
          expect(disposition).to.equal(contentDisposition)
          done()
        })
      })
    })

    it('can generate a default filename', function (done) {
      if (process.env.APPVEYOR === 'True') {
        // FIXME(alexeykuzmin): Skip the test.
        // this.skip()
        return done()
      }

      downloadServer.listen(0, '127.0.0.1', () => {
        const port = downloadServer.address().port
        ipcRenderer.sendSync('set-download-option', true, false)
        w.webContents.downloadURL(`${url}:${port}/?testFilename`)
        ipcRenderer.once('download-done', (event, state, url,
          mimeType, receivedBytes,
          totalBytes, disposition,
          filename) => {
          expect(state).to.equal('cancelled')
          expect(filename).to.equal('download.pdf')
          expect(mimeType).to.equal('application/pdf')
          expect(receivedBytes).to.equal(0)
          expect(totalBytes).to.equal(mockPDF.length)
          expect(disposition).to.equal(contentDisposition)
          done()
        })
      })
    })

    it('can set options for the save dialog', (done) => {
      downloadServer.listen(0, '127.0.0.1', () => {
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

        ipcRenderer.sendSync('set-download-option', true, false, filePath, options)
        w.webContents.downloadURL(`${url}:${port}`)
        ipcRenderer.once('download-done', (event, state, url,
          mimeType, receivedBytes,
          totalBytes, disposition,
          filename, savePath, dialogOptions) => {
          expect(dialogOptions).to.deep.equal(options)
          done()
        })
      })
    })

    describe('when a save path is specified and the URL is unavailable', () => {
      it('does not display a save dialog and reports the done state as interrupted', (done) => {
        ipcRenderer.sendSync('set-download-option', false, false)
        ipcRenderer.once('download-done', (event, state) => {
          expect(state).to.equal('interrupted')
          done()
        })
        w.webContents.downloadURL(`file://${path.join(__dirname, 'does-not-exist.txt')}`)
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

    it('handles requests from partition', (done) => {
      w.webContents.on('did-finish-load', () => done())
      w.loadURL(`${protocolName}://fake-host`)
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
      const certPath = path.join(__dirname, 'fixtures', 'certificates')
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
        expect(['net::ERR_CERT_AUTHORITY_INVALID', 'net::ERR_CERT_COMMON_NAME_INVALID'].includes(verificationResult)).to.be.true()
        expect([-202, -200].includes(errorCode)).to.be.true()
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
        expect(certificate.issuerCert.issuerCert.issuerCert).to.be.undefined()
        expect(['net::ERR_CERT_AUTHORITY_INVALID', 'net::ERR_CERT_COMMON_NAME_INVALID'].includes(verificationResult)).to.be.true()
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

  describe('ses.createInterruptedDownload(options)', () => {
    it('can create an interrupted download item', (done) => {
      ipcRenderer.sendSync('set-download-option', true, false)
      const filePath = path.join(__dirname, 'fixtures', 'mock.pdf')
      const options = {
        path: filePath,
        urlChain: ['http://127.0.0.1/'],
        mimeType: 'application/pdf',
        offset: 0,
        length: 5242880
      }
      w.webContents.session.createInterruptedDownload(options)
      ipcRenderer.once('download-created', (event, state, urlChain,
        mimeType, receivedBytes,
        totalBytes, filename,
        savePath) => {
        expect(state).to.equal('interrupted')
        expect(urlChain).to.deep.equal(['http://127.0.0.1/'])
        expect(mimeType).to.equal('application/pdf')
        expect(receivedBytes).to.equal(0)
        expect(totalBytes).to.equal(5242880)
        expect(savePath).to.equal(filePath)
        done()
      })
    })

    it('can be resumed', (done) => {
      const fixtures = path.join(__dirname, 'fixtures')
      const downloadFilePath = path.join(fixtures, 'logo.png')
      const rangeServer = http.createServer((req, res) => {
        const options = { root: fixtures }
        send(req, req.url, options)
          .on('error', (error) => { done(error) }).pipe(res)
      })
      ipcRenderer.sendSync('set-download-option', true, false, downloadFilePath)
      rangeServer.listen(0, '127.0.0.1', () => {
        const port = rangeServer.address().port
        const downloadUrl = `http://127.0.0.1:${port}/assets/logo.png`
        const callback = (event, state, url, mimeType,
          receivedBytes, totalBytes, disposition,
          filename, savePath, dialogOptions, urlChain,
          lastModifiedTime, eTag) => {
          if (state === 'cancelled') {
            const options = {
              path: savePath,
              urlChain: urlChain,
              mimeType: mimeType,
              offset: receivedBytes,
              length: totalBytes,
              lastModified: lastModifiedTime,
              eTag: eTag
            }
            ipcRenderer.sendSync('set-download-option', false, false, downloadFilePath)
            w.webContents.session.createInterruptedDownload(options)
          } else {
            expect(state).to.equal('completed')
            expect(filename).to.equal('logo.png')
            expect(savePath).to.equal(downloadFilePath)
            expect(url).to.equal(downloadUrl)
            expect(mimeType).to.equal('image/png')
            expect(receivedBytes).to.equal(14022)
            expect(totalBytes).to.equal(14022)
            expect(fs.existsSync(downloadFilePath)).to.be.true()
            fs.unlinkSync(downloadFilePath)
            rangeServer.close()
            ipcRenderer.removeListener('download-done', callback)
            done()
          }
        }
        ipcRenderer.on('download-done', callback)
        w.webContents.downloadURL(downloadUrl)
      })
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

  describe('ses.setPermissionRequestHandler(handler)', () => {
    it('cancels any pending requests when cleared', (done) => {
      const ses = session.fromPartition('permissionTest')
      ses.setPermissionRequestHandler(() => {
        ses.setPermissionRequestHandler(null)
      })

      webview = new WebView()
      webview.addEventListener('ipc-message', (e) => {
        expect(e.channel).to.equal('message')
        expect(e.args).to.deep.equal(['SecurityError'])
        done()
      })
      webview.src = `file://${fixtures}/pages/permissions/midi-sysex.html`
      webview.partition = 'permissionTest'
      webview.setAttribute('nodeintegration', 'on')
      document.body.appendChild(webview)
    })
  })
})
