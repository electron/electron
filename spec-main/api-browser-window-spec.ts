import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as path from 'path'
import * as fs from 'fs'
import * as http from 'http'
import { AddressInfo } from 'net'
import { BrowserWindow } from 'electron'
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

})
