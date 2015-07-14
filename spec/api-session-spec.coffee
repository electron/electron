assert  = require 'assert'
remote  = require 'remote'
http    = require 'http'
path    = require 'path'
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
      res.setHeader('Set-Cookie', ['type=dummy'])
      res.end('finished')
      server.close()

    port = remote.process.port
    server.listen port, '127.0.0.1', ->
      {port} = server.address()
      w.loadUrl "#{url}:#{port}"
      w.webContents.on 'did-finish-load', ->
        w.webContents.session.cookies.get {url: url}, (error, cookies) ->
          return done(error) if error
          assert.equal 'type', cookies[0].name
          assert.equal 'dummy', cookies[0].value
          done()

  it 'should over-write the existent cookie', (done) ->
    w.loadUrl 'file://' + path.join(fixtures, 'page', 'a.html')
    w.webContents.on 'did-finish-load', ->
      w.webContents.session.cookies.set {url: url, name: 'type', value: 'dummy2'}, (error) ->
        return done(error) if error
        w.webContents.session.cookies.get {url: url}, (error, cookies_list) ->
          return done(error) if error
          assert.equal 'type', cookies_list[0].name
          assert.equal 'dummy2', cookies_list[0].value
          done()

  it 'should set new cookie', (done) ->
    w.loadUrl 'file://' + path.join(fixtures, 'page', 'a.html')
    w.webContents.on 'did-finish-load', ->
      w.webContents.session.cookies.set {url: url, name: 'key', value: 'dummy2'}, (error) ->
        return done(error) if error
        w.webContents.session.cookies.get {url: url}, (error, cookies_list) ->
          return done(error) if error
          for cookie in cookies_list
            if cookie.name is 'key'
               assert.equal 'dummy2', cookie.value
               return done()
          done('Can not find cookie')

  it 'should remove cookies', (done) ->
    w.loadUrl 'file://' + path.join(fixtures, 'page', 'a.html')
    w.webContents.on 'did-finish-load', ->
      w.webContents.session.cookies.set {url: url, name: 'key', value: 'dummy2'}, (error) ->
        return done(error) if error
        w.webContents.session.cookies.remove {url: url, name: 'key'}, (error) ->
          return done(error) if error
          w.webContents.session.cookies.get {url: url}, (error, list) ->
            return done(error) if error
            for cookie in list when cookie.name is 'key'
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
