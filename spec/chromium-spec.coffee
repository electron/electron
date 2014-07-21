assert = require 'assert'
http = require 'http'
path = require 'path'

describe 'chromium feature', ->
  fixtures = path.resolve __dirname, 'fixtures'

  describe 'heap snapshot', ->
    it 'does not crash', ->
      process.atomBinding('v8_util').takeHeapSnapshot()

  describe 'sending request of http protocol urls', ->
    it 'does not crash', (done) ->
      @timeout 5000
      server = http.createServer (req, res) ->
        res.end()
        server.close()
        done()
      server.listen 0, '127.0.0.1', ->
        {port} = server.address()
        $.get "http://127.0.0.1:#{port}"

  describe 'navigator.webkitGetUserMedia', ->
    it 'calls its callbacks', (done) ->
      @timeout 5000
      navigator.webkitGetUserMedia audio: true, video: false,
        -> done()
        -> done()

  describe 'navigator.language', ->
    it 'should not be empty', ->
      assert.notEqual navigator.language, ''

  describe 'window.open', ->
    it 'returns a BrowserWindow object', ->
      b = window.open 'about:blank', 'test', 'show=no'
      assert.equal b.constructor.name, 'BrowserWindow'
      b.destroy()

  describe 'iframe', ->
    page = path.join fixtures, 'pages', 'change-parent.html'

    beforeEach ->
      global.changedByIframe = false

    it 'can not modify parent by default', (done) ->
      iframe = $('<iframe>')
      iframe.hide()
      iframe.attr 'src', "file://#{page}"
      iframe.appendTo 'body'
      isChanged = ->
        iframe.remove()
        assert.equal global.changedByIframe, false
        done()
      setTimeout isChanged, 30

    it 'can modify parent when sanbox is set to none', (done) ->
      iframe = $('<iframe sandbox="none">')
      iframe.hide()
      iframe.attr 'src', "file://#{page}"
      iframe.appendTo 'body'
      isChanged = ->
        iframe.remove()
        assert.equal global.changedByIframe, true
        done()
      setTimeout isChanged, 30

  describe 'creating a Uint8Array under browser side', ->
    it 'does not crash', ->
      RUint8Array = require('remote').getGlobal 'Uint8Array'
      new RUint8Array

  describe 'webgl', ->
    it 'can be get as context in canvas', ->
      return if process.platform is 'linux'
      webgl = document.createElement('canvas').getContext 'webgl'
      assert.notEqual webgl, null

  describe 'web workers', ->
    it 'Worker can work', (done) ->
      worker = new Worker('../fixtures/workers/worker.js')
      message = 'ping'
      worker.onmessage = (event) ->
        assert.equal event.data, message
        worker.terminate()
        done()
      worker.postMessage message

    it 'SharedWorker can work', (done) ->
      worker = new SharedWorker('../fixtures/workers/shared_worker.js')
      message = 'ping'
      worker.port.onmessage = (event) ->
        assert.equal event.data, message
        done()
      worker.port.postMessage message
