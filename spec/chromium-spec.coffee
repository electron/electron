assert = require 'assert'
http = require 'http'
https = require 'https'
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
    it 'returns a BrowserWindowProxy object', ->
      b = window.open 'about:blank', 'test', 'show=no'
      assert.equal b.constructor.name, 'BrowserWindowProxy'
      b.close()

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

  describe 'iframe', ->
    iframe = null

    beforeEach ->
      iframe = document.createElement 'iframe'

    afterEach ->
      document.body.removeChild iframe

    it 'does not have node integration', (done) ->
      iframe.src = "file://#{fixtures}/pages/set-global.html"
      document.body.appendChild iframe
      iframe.onload = ->
        assert.equal iframe.contentWindow.test, 'undefined undefined undefined'
        done()

  describe 'storage', ->
    it 'requesting persitent quota works', (done) ->
      navigator.webkitPersistentStorage.requestQuota 1024 * 1024, (grantedBytes) ->
        assert.equal grantedBytes, 1048576
        done()
