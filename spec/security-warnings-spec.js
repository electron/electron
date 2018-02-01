const assert = require('assert')
const http = require('http')
const fs = require('fs')
const path = require('path')
const url = require('url')
const os = require('os')
const qs = require('querystring')

const {remote} = require('electron')
const {BrowserWindow} = remote

describe('security warnings', () => {
  let server
  let w = null
  let useCsp = true

  before(() => {
    server = http.createServer(function(request, response) {
      const uri = url.parse(request.url).pathname
      let filename = path.join(__dirname, './fixtures/pages', uri)

      fs.exists(filename, (exists) => {
        if (!exists) {
          response.writeHead(404, { 'Content-Type': 'text/plain' })
          response.end()
          return
        }

        if (fs.statSync(filename).isDirectory()) {
          filename += '/index.html'
        }

        fs.readFile(filename, 'binary', function(err, file) {
          if (err) {
            response.writeHead(404, { 'Content-Type': 'text/plain' })
            response.end()
            return
          }

          const cspHeaders = { 'Content-Security-Policy': `script-src 'self'` }
          response.writeHead(200, useCsp ? cspHeaders : undefined)
          response.write(file, 'binary')
          response.end()
        })
      })
    }).listen(8881)
  })

  after(() => {
    server.close()
    server = null
  })

  afterEach(() => {
    if (w && w.close) w.close()
    useCsp = true
  })

  it('should warn about Node.js integration with remote content', (done) => {
    w = new BrowserWindow({ show: false })
    w.webContents.on('console-message', (e, level, message) => {
      assert(message.includes('Node.js Integration with Remote Content'))
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/base-page.html`)
  })

  it('should warn about disabled webSecurity', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        webSecurity: false,
        nodeIntegration: false
      }
    })
    w.webContents.on('console-message', (e, level, message) => {
      assert(message.includes('Disabled webSecurity'))
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/base-page.html`)
  })

  it('should warn about insecure Content-Security-Policy', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: false
      }
    })

    w.webContents.on('console-message', (e, level, message) => {
      assert(message.includes('Insecure Content-Security-Policy'))
      done()
    })

    useCsp = false
    w.loadURL(`http://127.0.0.1:8881/base-page.html`)
  })

  it('should warn about allowRunningInsecureContent', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        allowRunningInsecureContent: true,
        nodeIntegration: false
      }
    })
    w.webContents.on('console-message', (e, level, message) => {
      assert(message.includes('allowRunningInsecureContent'))
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/base-page.html`)
  })

  it('should warn about experimentalFeatures', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        experimentalFeatures: true,
        nodeIntegration: false
      }
    })
    w.webContents.on('console-message', (e, level, message) => {
      assert(message.includes('experimentalFeatures'))
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/base-page.html`)
  })

  it('should warn about blinkFeatures', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        blinkFeatures: ['my-cool-feature'],
        nodeIntegration: false
      }
    })
    w.webContents.on('console-message', (e, level, message) => {
      assert(message.includes('blinkFeatures'))
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/base-page.html`)
  })

  it('should warn about allowpopups', (done) => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        nodeIntegration: false
      }
    })
    w.webContents.on('console-message', (e, level, message) => {
      console.log(message)
      assert(message.includes('allowpopups'))
      done()
    })

    w.loadURL(`http://127.0.0.1:8881/webview-allowpopups.html`)
  })
})