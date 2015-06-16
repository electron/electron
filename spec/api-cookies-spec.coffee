assert  = require 'assert'
remote  = require 'remote'
http       = require 'http'
cookies = remote.require 'cookies'
BrowserWindow = remote.require 'browser-window'

describe 'cookies module', ->
  w = null
  url = "http://127.0.0.1:9999"
  beforeEach -> w = new BrowserWindow(show: true)
  afterEach -> w.destroy()

  it 'should get cookies', (done) ->
    server = http.createServer (req, res) ->
      console.log req
      res.setHeader('Set-Cookie', ['type=dummy'])
      res.end('finished')

    server.listen 9999, '127.0.0.1', ->
      {port} = server.address()
      w.loadUrl url
      w.webContents.on 'did-finish-load', ()->
        cookies.get {url:url}, (error, cookies) ->
          throw error if error
          assert.equal 1, cookies.length
          assert.equal 'type', cookies[0].name
          assert.equal 'dummy', cookies[0].value
          done()

  it 'should overwrite the existent cookie', (done) ->
    cookies.set {url:url, name:'type', value:'dummy2'}, (error) ->
      throw error if error
      cookies.get {url:url}, (error, cookies_list) ->
        throw error if error
        assert.equal 1, cookies_list.length
        assert.equal 'type', cookies_list[0].name
        assert.equal 'dummy2', cookies_list[0].value
        done()

  it 'should set new cookie', (done) ->
    cookies.set {url:url, name:'key', value:'dummy2'}, (error) ->
      throw error if error
      cookies.get {url:url}, (error, cookies_list) ->
        throw error if error
        assert.equal 2, cookies_list.length
        for cookie in cookies_list
          if cookie.name is 'key'
             assert.equal 'dummy2', cookie.value
             done();

  it 'should remove cookies', (done) ->
    cookies.get {url:url}, (error, cookies_list) ->
      count = 0
      console.log cookies_list
      for cookie in cookies_list
        cookies.remove {url:url, name:cookie.name}, (error) ->
          throw error if error
          ++count
          if count == cookies_list.length
            cookies.get {url:url}, (error, cookies_list) ->
              throw error if error
              assert.equal 0, cookies_list.length
              done()
