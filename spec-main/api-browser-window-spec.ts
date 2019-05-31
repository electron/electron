import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import * as fs from 'fs'
import * as qs from 'querystring'
import * as http from 'http'
import { AddressInfo } from 'net'
import { app, BrowserWindow, ipcMain, OnBeforeSendHeadersListenerDetails } from 'electron'
import { emittedOnce } from './events-helpers';
import { closeWindow } from './window-helpers';

const { expect } = chai

chai.use(chaiAsPromised)

const fixtures = path.resolve(__dirname, '..', 'spec', 'fixtures')

describe('BrowserWindow module', () => {
  describe('BrowserWindow constructor', () => {
    it('allows passing undefined as the webContents', async () => {
      expect(() => {
        const w = new BrowserWindow({
          show: false,
          webContents: undefined
        } as any)
        w.destroy()
      }).not.to.throw()
    })
  })

  describe('BrowserWindow.close()', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('should emit unload handler', async () => {
      await w.loadFile(path.join(fixtures, 'api', 'unload.html'))
      const closed = emittedOnce(w, 'closed')
      w.close()
      await closed
      const test = path.join(fixtures, 'api', 'unload')
      const content = fs.readFileSync(test)
      fs.unlinkSync(test)
      expect(String(content)).to.equal('unload')
    })
    it('should emit beforeunload handler', async () => {
      await w.loadFile(path.join(fixtures, 'api', 'beforeunload-false.html'))
      const beforeunload = emittedOnce(w, 'onbeforeunload')
      w.close()
      await beforeunload
    })

    describe('when invoked synchronously inside navigation observer', () => {
      let server: http.Server = null as unknown as http.Server
      let url: string = null as unknown as string

      before((done) => {
        server = http.createServer((request, response) => {
          switch (request.url) {
            case '/net-error':
              response.destroy()
              break
            case '/301':
              response.statusCode = 301
              response.setHeader('Location', '/200')
              response.end()
              break
            case '/200':
              response.statusCode = 200
              response.end('hello')
              break
            case '/title':
              response.statusCode = 200
              response.end('<title>Hello</title>')
              break
            default:
              throw new Error(`unsupported endpoint: ${request.url}`)
          }
        }).listen(0, '127.0.0.1', () => {
          url = 'http://127.0.0.1:' + (server.address() as AddressInfo).port
          done()
        })
      })

      after(() => {
        server.close()
      })

      const events = [
        { name: 'did-start-loading', path: '/200' },
        { name: 'dom-ready', path: '/200' },
        { name: 'page-title-updated', path: '/title' },
        { name: 'did-stop-loading', path: '/200' },
        { name: 'did-finish-load', path: '/200' },
        { name: 'did-frame-finish-load', path: '/200' },
        { name: 'did-fail-load', path: '/net-error' }
      ]

      for (const {name, path} of events) {
        it(`should not crash when closed during ${name}`, async () => {
          const w = new BrowserWindow({ show: false })
          w.webContents.once((name as any), () => {
            w.close()
          })
          const destroyed = emittedOnce(w.webContents, 'destroyed')
          w.webContents.loadURL(url + path)
          await destroyed
        })
      }
    })
  })

  describe('window.close()', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('should emit unload event', async () => {
      w.loadFile(path.join(fixtures, 'api', 'close.html'))
      await emittedOnce(w, 'closed')
      const test = path.join(fixtures, 'api', 'close')
      const content = fs.readFileSync(test).toString()
      fs.unlinkSync(test)
      expect(content).to.equal('close')
    })
    it('should emit beforeunload event', async () => {
      w.loadFile(path.join(fixtures, 'api', 'close-beforeunload-false.html'))
      await emittedOnce(w, 'onbeforeunload')
    })
  })

  describe('BrowserWindow.destroy()', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })

    it('prevents users to access methods of webContents', async () => {
      const contents = w.webContents
      w.destroy()
      await new Promise(setImmediate)
      expect(() => {
        contents.getProcessId()
      }).to.throw('Object has been destroyed')
    })
    it('should not crash when destroying windows with pending events', () => {
      const focusListener = () => {}
      app.on('browser-window-focus', focusListener)
      const windowCount = 3
      const windowOptions = {
        show: false,
        width: 400,
        height: 400,
        webPreferences: {
          backgroundThrottling: false
        }
      }
      const windows = Array.from(Array(windowCount)).map(x => new BrowserWindow(windowOptions))
      windows.forEach(win => win.show())
      windows.forEach(win => win.focus())
      windows.forEach(win => win.destroy())
      app.removeListener('browser-window-focus', focusListener)
    })
  })

  describe('BrowserWindow.loadURL(url)', () => {
    let w = null as unknown as BrowserWindow
    beforeEach(() => {
      w = new BrowserWindow({show: false, webPreferences: {nodeIntegration: true}})
    })
    afterEach(async () => {
      await closeWindow(w)
      w = null as unknown as BrowserWindow
    })
    let server = null as unknown as http.Server
    let url = null as unknown as string
    let postData = null as any
    before((done) => {
      const filePath = path.join(fixtures, 'pages', 'a.html')
      const fileStats = fs.statSync(filePath)
      postData = [
        {
          type: 'rawData',
          bytes: Buffer.from('username=test&file=')
        },
        {
          type: 'file',
          filePath: filePath,
          offset: 0,
          length: fileStats.size,
          modificationTime: fileStats.mtime.getTime() / 1000
        }
      ]
      server = http.createServer((req, res) => {
        function respond () {
          if (req.method === 'POST') {
            let body = ''
            req.on('data', (data) => {
              if (data) body += data
            })
            req.on('end', () => {
              const parsedData = qs.parse(body)
              fs.readFile(filePath, (err, data) => {
                if (err) return
                if (parsedData.username === 'test' &&
                    parsedData.file === data.toString()) {
                  res.end()
                }
              })
            })
          } else if (req.url === '/302') {
            res.setHeader('Location', '/200')
            res.statusCode = 302
            res.end()
          } else if (req.url === '/navigate-302') {
            res.end(`<html><body><script>window.location='${url}/302'</script></body></html>`)
          } else if (req.url === '/cross-site') {
            res.end(`<html><body><h1>${req.url}</h1></body></html>`)
          } else {
            res.end()
          }
        }
        setTimeout(respond, req.url && req.url.includes('slow') ? 200 : 0)
      })
      server.listen(0, '127.0.0.1', () => {
        url = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
        done()
      })
    })

    after(() => {
      server.close()
    })

    it('should emit did-start-loading event', (done) => {
      w.webContents.on('did-start-loading', () => { done() })
      w.loadURL('about:blank')
    })
    it('should emit ready-to-show event', (done) => {
      w.on('ready-to-show', () => { done() })
      w.loadURL('about:blank')
    })
    it('should emit did-fail-load event for files that do not exist', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        expect(code).to.equal(-6)
        expect(desc).to.equal('ERR_FILE_NOT_FOUND')
        expect(isMainFrame).to.equal(true)
        done()
      })
      w.loadURL('file://a.txt')
    })
    it('should emit did-fail-load event for invalid URL', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        expect(desc).to.equal('ERR_INVALID_URL')
        expect(code).to.equal(-300)
        expect(isMainFrame).to.equal(true)
        done()
      })
      w.loadURL('http://example:port')
    })
    it('should set `mainFrame = false` on did-fail-load events in iframes', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        expect(isMainFrame).to.equal(false)
        done()
      })
      w.loadFile(path.join(fixtures, 'api', 'did-fail-load-iframe.html'))
    })
    it('does not crash in did-fail-provisional-load handler', (done) => {
      w.webContents.once('did-fail-provisional-load', () => {
        w.loadURL('http://127.0.0.1:11111')
        done()
      })
      w.loadURL('http://127.0.0.1:11111')
    })
    it('should emit did-fail-load event for URL exceeding character limit', (done) => {
      w.webContents.on('did-fail-load', (event, code, desc, url, isMainFrame) => {
        expect(desc).to.equal('ERR_INVALID_URL')
        expect(code).to.equal(-300)
        expect(isMainFrame).to.equal(true)
        done()
      })
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64')
      w.loadURL(`data:image/png;base64,${data}`)
    })

    it('should return a promise', () => {
      const p = w.loadURL('about:blank')
      expect(p).to.have.property('then')
    })

    it('should return a promise that resolves', async () => {
      expect(w.loadURL('about:blank')).to.eventually.be.fulfilled
    })

    it('should return a promise that rejects on a load failure', async () => {
      const data = Buffer.alloc(2 * 1024 * 1024).toString('base64')
      const p = w.loadURL(`data:image/png;base64,${data}`)
      await expect(p).to.eventually.be.rejected
    })

    it('should return a promise that resolves even if pushState occurs during navigation', async () => {
      const p = w.loadURL('data:text/html,<script>window.history.pushState({}, "/foo")</script>')
      await expect(p).to.eventually.be.fulfilled
    })

    describe('POST navigations', () => {
      afterEach(() => { w.webContents.session.webRequest.onBeforeSendHeaders(null) })

      it('supports specifying POST data', async () => {
        await w.loadURL(url, { postData: postData })
      })
      it('sets the content type header on URL encoded forms', async () => {
        await w.loadURL(url)
        const requestDetails: Promise<OnBeforeSendHeadersListenerDetails> = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            resolve(details)
          })
        })
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.submit()
        `)
        const details = await requestDetails
        expect(details.requestHeaders['Content-Type']).to.equal('application/x-www-form-urlencoded')
      })
      it('sets the content type header on multi part forms', async () => {
        await w.loadURL(url)
        const requestDetails: Promise<OnBeforeSendHeadersListenerDetails> = new Promise(resolve => {
          w.webContents.session.webRequest.onBeforeSendHeaders((details, callback) => {
            resolve(details)
          })
        })
        w.webContents.executeJavaScript(`
          form = document.createElement('form')
          document.body.appendChild(form)
          form.method = 'POST'
          form.enctype = 'multipart/form-data'
          file = document.createElement('input')
          file.type = 'file'
          file.name = 'file'
          form.appendChild(file)
          form.submit()
        `)
        const details = await requestDetails
        expect(details.requestHeaders['Content-Type'].startsWith('multipart/form-data; boundary=----WebKitFormBoundary')).to.equal(true)
      })
    })

    it('should support support base url for data urls', (done) => {
      ipcMain.once('answer', (event, test) => {
        expect(test).to.equal('test')
        done()
      })
      w.loadURL('data:text/html,<script src="loaded-from-dataurl.js"></script>', { baseURLForDataURL: `file://${path.join(fixtures, 'api')}${path.sep}` })
    })
  })
})
