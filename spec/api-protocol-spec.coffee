assert   = require 'assert'
ipc      = require 'ipc'
http     = require 'http'
path     = require 'path'
remote   = require 'remote'
protocol = remote.require 'protocol'

describe 'protocol module', ->
  describe 'protocol.registerProtocol', ->
    it 'throws error when scheme is already registered', (done) ->
      register = -> protocol.registerProtocol('test1', ->)
      protocol.once 'registered', (event, scheme) ->
        assert.equal scheme, 'test1'
        assert.throws register, /The scheme is already registered/
        protocol.unregisterProtocol 'test1'
        done()
      register()

    it 'calls the callback when scheme is visited', (done) ->
      protocol.registerProtocol 'test2', (request) ->
        assert.equal request.url, 'test2://test2'
        protocol.unregisterProtocol 'test2'
        done()
      $.get 'test2://test2', ->

  describe 'protocol.unregisterProtocol', ->
    it 'throws error when scheme does not exist', ->
      unregister = -> protocol.unregisterProtocol 'test3'
      assert.throws unregister, /The scheme has not been registered/

  describe 'registered protocol callback', ->
    it 'returns string should send the string as request content', (done) ->
      handler = remote.createFunctionWithReturnValue 'valar morghulis'
      protocol.registerProtocol 'atom-string', handler

      $.ajax
        url: 'atom-string://fake-host'
        success: (data) ->
          assert.equal data, handler()
          protocol.unregisterProtocol 'atom-string'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
          protocol.unregisterProtocol 'atom-string'

    it 'returns RequestStringJob should send string', (done) ->
      data = 'valar morghulis'
      job = new protocol.RequestStringJob(mimeType: 'text/html', data: data)
      handler = remote.createFunctionWithReturnValue job
      protocol.registerProtocol 'atom-string-job', handler

      $.ajax
        url: 'atom-string-job://fake-host'
        success: (response) ->
          assert.equal response, data
          protocol.unregisterProtocol 'atom-string-job'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
          protocol.unregisterProtocol 'atom-string-job'

    it 'returns RequestErrorJob should send error', (done) ->
      data = 'valar morghulis'
      job = new protocol.RequestErrorJob(-6)
      handler = remote.createFunctionWithReturnValue job
      protocol.registerProtocol 'atom-error-job', handler

      $.ajax
        url: 'atom-error-job://fake-host'
        success: (response) ->
          assert false, 'should not reach here'
        error: (xhr, errorType, error) ->
          assert errorType, 'error'
          protocol.unregisterProtocol 'atom-error-job'
          done()

    it 'returns RequestHttpJob should send respone', (done) ->
      server = http.createServer (req, res) ->
        res.writeHead(200, {'Content-Type': 'text/plain'})
        res.end('hello')
        server.close()
      server.listen 0, '127.0.0.1', ->
        {port} = server.address()
        url = "http://127.0.0.1:#{port}"
        job = new protocol.RequestHttpJob({url})
        handler = remote.createFunctionWithReturnValue job
        protocol.registerProtocol 'atom-http-job', handler

        $.ajax
          url: 'atom-http-job://fake-host'
          success: (data) ->
            assert.equal data, 'hello'
            protocol.unregisterProtocol 'atom-http-job'
            done()
          error: (xhr, errorType, error) ->
            assert false, 'Got error: ' + errorType + ' ' + error
            protocol.unregisterProtocol 'atom-http-job'

    it 'returns RequestBufferJob should send buffer', (done) ->
      data = new Buffer("hello")
      job = new protocol.RequestBufferJob(data: data)
      handler = remote.createFunctionWithReturnValue job
      protocol.registerProtocol 'atom-buffer-job', handler

      $.ajax
        url: 'atom-buffer-job://fake-host'
        success: (response) ->
          assert.equal response.length, data.length
          buf = new Buffer(response.length)
          buf.write(response)
          assert buf.equals(data)
          protocol.unregisterProtocol 'atom-buffer-job'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
          protocol.unregisterProtocol 'atom-buffer-job'

    it 'returns RequestFileJob should send file', (done) ->
      job = new protocol.RequestFileJob(__filename)
      handler = remote.createFunctionWithReturnValue job
      protocol.registerProtocol 'atom-file-job', handler

      $.ajax
        url: 'atom-file-job://' + __filename
        success: (data) ->
          content = require('fs').readFileSync __filename
          assert.equal data, String(content)
          protocol.unregisterProtocol 'atom-file-job'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
          protocol.unregisterProtocol 'atom-file-job'

    it 'returns RequestFileJob should send file from asar archive', (done) ->
      p = path.join __dirname, 'fixtures', 'asar', 'a.asar', 'file1'
      job = new protocol.RequestFileJob(p)
      handler = remote.createFunctionWithReturnValue job
      protocol.registerProtocol 'atom-file-job', handler

      $.ajax
        url: 'atom-file-job://' + p
        success: (data) ->
          content = require('fs').readFileSync(p)
          assert.equal data, String(content)
          protocol.unregisterProtocol 'atom-file-job'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
          protocol.unregisterProtocol 'atom-file-job'

    it 'returns RequestFileJob should send file from asar archive with unpacked file', (done) ->
      p = path.join __dirname, 'fixtures', 'asar', 'unpack.asar', 'a.txt'
      job = new protocol.RequestFileJob(p)
      handler = remote.createFunctionWithReturnValue job
      protocol.registerProtocol 'atom-file-job', handler

      $.ajax
        url: 'atom-file-job://' + p
        success: (response) ->
          data = require('fs').readFileSync(p)
          assert.equal response.length, data.length
          buf = new Buffer(response.length)
          buf.write(response)
          assert buf.equals(data)
          protocol.unregisterProtocol 'atom-file-job'
          done()
        error: (xhr, errorType, error) ->
          assert false, 'Got error: ' + errorType + ' ' + error
          protocol.unregisterProtocol 'atom-file-job'

  describe 'protocol.isHandledProtocol', ->
    it 'returns true if the scheme can be handled', ->
      assert.equal protocol.isHandledProtocol('file'), true
      assert.equal protocol.isHandledProtocol('http'), true
      assert.equal protocol.isHandledProtocol('https'), true
      assert.equal protocol.isHandledProtocol('atom'), false

  describe 'protocol.interceptProtocol', ->
    it 'throws error when scheme is not a registered one', ->
      register = -> protocol.interceptProtocol('test-intercept', ->)
      assert.throws register, /Scheme does not exist/

    it 'throws error when scheme is a custom protocol', (done) ->
      protocol.once 'unregistered', (event, scheme) ->
        assert.equal scheme, 'atom'
        done()
      protocol.once 'registered', (event, scheme) ->
        assert.equal scheme, 'atom'
        register = -> protocol.interceptProtocol('test-intercept', ->)
        assert.throws register, /Scheme does not exist/
        protocol.unregisterProtocol scheme
      protocol.registerProtocol('atom', ->)

    it 'returns original job when callback returns nothing', (done) ->
      targetScheme = 'file'
      protocol.once 'intercepted', (event, scheme) ->
        assert.equal scheme, targetScheme
        free = -> protocol.uninterceptProtocol targetScheme
        $.ajax
          url: "#{targetScheme}://#{__filename}",
          success: ->
            protocol.once 'unintercepted', (event, scheme) ->
              assert.equal scheme, targetScheme
              done()
            free()
          error: (xhr, errorType, error) ->
            free()
            assert false, 'Got error: ' + errorType + ' ' + error
      protocol.interceptProtocol targetScheme, (request) ->
        if process.platform is 'win32'
          pathInUrl = path.normalize request.url.substr(8)
          assert.equal pathInUrl.toLowerCase(), __filename.toLowerCase()
        else
          assert.equal request.url, "#{targetScheme}://#{__filename}"

    it 'can override original protocol handler', (done) ->
      handler = remote.createFunctionWithReturnValue 'valar morghulis'
      protocol.once 'intercepted', ->
        free = -> protocol.uninterceptProtocol 'file'
        $.ajax
          url: 'file://fake-host'
          success: (data) ->
            protocol.once 'unintercepted', ->
              assert.equal data, handler()
              done()
            free()
          error: (xhr, errorType, error) ->
            assert false, 'Got error: ' + errorType + ' ' + error
            free()
      protocol.interceptProtocol 'file', handler

    it 'can override http protocol handler', (done) ->
      handler = remote.createFunctionWithReturnValue 'valar morghulis'
      protocol.once 'intercepted', ->
        protocol.uninterceptProtocol 'http'
        done()
      protocol.interceptProtocol 'http', handler

    it 'can override https protocol handler', (done) ->
      handler = remote.createFunctionWithReturnValue 'valar morghulis'
      protocol.once 'intercepted', ->
        protocol.uninterceptProtocol 'https'
        done()
      protocol.interceptProtocol 'https', handler

    it 'can override ws protocol handler', (done) ->
      handler = remote.createFunctionWithReturnValue 'valar morghulis'
      protocol.once 'intercepted', ->
        protocol.uninterceptProtocol 'ws'
        done()
      protocol.interceptProtocol 'ws', handler

    it 'can override wss protocol handler', (done) ->
      handler = remote.createFunctionWithReturnValue 'valar morghulis'
      protocol.once 'intercepted', ->
        protocol.uninterceptProtocol 'wss'
        done()
      protocol.interceptProtocol 'wss', handler
