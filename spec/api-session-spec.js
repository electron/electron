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
  const fixtures = path.resolve(__dirname, 'fixtures')
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
