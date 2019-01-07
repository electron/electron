const assert = require('assert')
const http = require('http')
const fs = require('fs')
const path = require('path')
const url = require('url')

const { remote } = require('electron')
const { BrowserWindow } = remote

const { closeWindow } = require('./window-helpers')

describe('security warnings', () => {
  let server
  let w = null
  let useCsp = true

  before((done) => {
    // Create HTTP Server
    server = http.createServer((request, response) => {
      const uri = url.parse(request.url).pathname
      let filename = path.join(__dirname, './fixtures/pages', uri)

      fs.stat(filename, (error, stats) => {
        if (error) {
          response.writeHead(404, { 'Content-Type': 'text/plain' })
          response.end()
          return
        }

        if (stats.isDirectory()) {
          filename += '/index.html'
        }

        fs.readFile(filename, 'binary', (err, file) => {
          if (err) {
            response.writeHead(404, { 'Content-Type': 'text/plain' })
            response.end()
            return
          }

          const cspHeaders = { 'Content-Security-Policy': `script-src 'self' 'unsafe-inline'` }
          response.writeHead(200, useCsp ? cspHeaders : undefined)
          response.write(file, 'binary')
          response.end()
        })
      })
    }).listen(8881, () => done())
  })

  after(() => {
    // Close server
    server.close()
    server = null
  })

  afterEach(() => {
    useCsp = true
    return closeWindow(w).then(() => { w = null })
  })

  it('should warn about Node.js integration with remote content', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: true
      }
    })
    w.webContents.once('console-message', (e, level, message) => {
      assert(message.includes('Node.js Integration with Remote Content'), message)
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/base-page-security.html`)
  })

  const generateSpecs = (description, webPreferences) => {
    describe(description, () => {
      it('should warn about disabled webSecurity', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            webSecurity: false,
            ...webPreferences
          }
        })
        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('Disabled webSecurity'), message)
          done()
        })

        w.loadURL(`http://127.0.0.1:8881/base-page-security.html`)
      })

      it('should warn about insecure Content-Security-Policy', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences
        })

        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('Insecure Content-Security-Policy'), message)
          done()
        })

        useCsp = false
        w.loadURL(`http://127.0.0.1:8881/base-page-security.html`)
      })

      it('should warn about allowRunningInsecureContent', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            allowRunningInsecureContent: true,
            ...webPreferences
          }
        })
        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('allowRunningInsecureContent'), message)
          done()
        })

        w.loadURL(`http://127.0.0.1:8881/base-page-security.html`)
      })

      it('should warn about experimentalFeatures', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            experimentalFeatures: true,
            ...webPreferences
          }
        })
        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('experimentalFeatures'), message)
          done()
        })

        w.loadURL(`http://127.0.0.1:8881/base-page-security.html`)
      })

      it('should warn about enableBlinkFeatures', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            enableBlinkFeatures: ['my-cool-feature'],
            ...webPreferences
          }
        })
        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('enableBlinkFeatures'), message)
          done()
        })

        w.loadURL(`http://127.0.0.1:8881/base-page-security.html`)
      })

      it('should warn about allowpopups', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences
        })
        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('allowpopups'), message)
          done()
        })

        w.loadURL(`http://127.0.0.1:8881/webview-allowpopups.html`)
      })

      it('should warn about insecure resources', (done) => {
        w = new BrowserWindow({
          show: false,
          webPreferences
        })
        w.webContents.once('console-message', (e, level, message) => {
          assert(message.includes('Insecure Resources'), message)
          done()
        })

        w.loadURL(`http://127.0.0.1:8881/insecure-resources.html`)
        w.webContents.openDevTools()
      })
    })
  }

  generateSpecs('without sandbox', {})
  generateSpecs('with sandbox', { sandbox: true })
  generateSpecs('with remote module disabled', { enableRemoteModule: false })
})
