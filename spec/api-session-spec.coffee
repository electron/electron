assert = require 'assert'
remote = require 'remote'
http   = require 'http'
path   = require 'path'
fs     = require 'fs'
app    = remote.require 'app'
BrowserWindow = remote.require 'browser-window'

describe 'session module', ->
  @timeout 10000
  fixtures = path.resolve __dirname, 'fixtures'
  w = null
  url = "http://127.0.0.1"

  beforeEach -> w = new BrowserWindow(show: false, width: 400, height: 400)
  afterEach -> w.destroy()

  it 'should get cookies', (done) ->
    server = http.createServer (req, res) ->
      res.setHeader('Set-Cookie', ['0=0'])
      res.end('finished')
      server.close()

    server.listen 0, '127.0.0.1', ->
      {port} = server.address()
      w.loadUrl "#{url}:#{port}"
      w.webContents.on 'did-finish-load', ->
        w.webContents.session.cookies.get {url: url}, (error, list) ->
          return done(error) if error
          for cookie in list when cookie.name is '0'
            if cookie.value is '0'
              return done()
            else
              return done("cookie value is #{cookie.value} while expecting 0")
          done('Can not find cookie')

  it 'should over-write the existent cookie', (done) ->
    app.defaultSession.cookies.set {url: url, name: '1', value: '1'}, (error) ->
      return done(error) if error
      app.defaultSession.cookies.get {url: url}, (error, list) ->
        return done(error) if error
        for cookie in list when cookie.name is '1'
          if cookie.value is '1'
            return done()
          else
            return done("cookie value is #{cookie.value} while expecting 1")
        done('Can not find cookie')

  it 'should remove cookies', (done) ->
    app.defaultSession.cookies.set {url: url, name: '2', value: '2'}, (error) ->
      return done(error) if error
      app.defaultSession.cookies.remove {url: url, name: '2'}, (error) ->
        return done(error) if error
        app.defaultSession.cookies.get {url: url}, (error, list) ->
          return done(error) if error
          for cookie in list when cookie.name is '2'
             return done('Cookie not deleted')
          done()

  describe 'session.clearStorageData(options)', ->
    fixtures = path.resolve __dirname, 'fixtures'
    it 'clears localstorage data', (done) ->
      ipc = remote.require('ipc')
      ipc.on 'count', (event, count) ->
        ipc.removeAllListeners 'count'
        assert not count
        done()
      w.loadUrl 'file://' + path.join(fixtures, 'api', 'localstorage.html')
      w.webContents.on 'did-finish-load', ->
        options =
          origin: "file://",
          storages: ['localstorage'],
          quotas: ['persistent'],
        w.webContents.session.clearStorageData options, ->
          w.webContents.send 'getcount'

  describe 'DownloadItem', ->
    # A 5 MB mock pdf.
    mockPDF = new Buffer 1024 * 1024 * 5
    contentDisposition = 'inline; filename="mock.pdf"'
    ipc = require 'ipc'
    downloadFilePath = path.join fixtures, 'mock.pdf'
    downloadServer = http.createServer (req, res) ->
      res.writeHead 200, {
        'Content-Length': mockPDF.length,
        'Content-Type': 'application/pdf',
        'Content-Disposition': contentDisposition
      }
      res.end mockPDF
      downloadServer.close()

    it 'can download successfully', (done) ->
      downloadServer.listen 0, '127.0.0.1', ->
        {port} = downloadServer.address()
        ipc.sendSync 'set-download-option', false
        w.loadUrl "#{url}:#{port}"
        ipc.once 'download-done', (state, url, mimeType, receivedBytes,
            totalBytes, disposition, filename) ->
          assert.equal state, 'completed'
          assert.equal filename, 'mock.pdf'
          assert.equal url, "http://127.0.0.1:#{port}/"
          assert.equal mimeType, 'application/pdf'
          assert.equal receivedBytes, mockPDF.length
          assert.equal totalBytes, mockPDF.length
          assert.equal disposition, contentDisposition
          assert fs.existsSync downloadFilePath
          fs.unlinkSync downloadFilePath
          done()

    it 'can cancel download', (done) ->
      downloadServer.listen 0, '127.0.0.1', ->
        {port} = downloadServer.address()
        ipc.sendSync 'set-download-option', true
        w.loadUrl "#{url}:#{port}/"
        ipc.once 'download-done', (state, url, mimeType, receivedBytes,
            totalBytes, disposition, filename) ->
          assert.equal state, 'cancelled'
          assert.equal filename, 'mock.pdf'
          assert.equal mimeType, 'application/pdf'
          assert.equal receivedBytes, 0
          assert.equal totalBytes, mockPDF.length
          assert.equal disposition, contentDisposition
          done()
