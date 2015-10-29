assert   = require 'assert'
http     = require 'http'
path     = require 'path'
remote   = require 'remote'
protocol = remote.require 'protocol'

describe 'protocol module', ->
  protocolName = 'sp'
  text = 'valar morghulis'

  afterEach (done) ->
    protocol.unregisterProtocol protocolName, ->
      protocol.uninterceptProtocol 'http', -> done()

  describe 'protocol.register(Any)Protocol', ->
    emptyHandler = (request, callback) -> callback()
    it 'throws error when scheme is already registered', (done) ->
      protocol.registerStringProtocol protocolName, emptyHandler, (error) ->
        assert.equal error, null
        protocol.registerBufferProtocol protocolName, emptyHandler, (error) ->
          assert.notEqual error, null
          done()

    it 'does not crash when handler is called twice', (done) ->
      doubleHandler = (request, callback) ->
        try
          callback(text)
          callback()
        catch
      protocol.registerStringProtocol protocolName, doubleHandler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'sends error when callback is called with nothing', (done) ->
      protocol.registerBufferProtocol protocolName, emptyHandler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

    it 'does not crash when callback is called in next tick', (done) ->
      handler = (request, callback) ->
        setImmediate -> callback(text)
      protocol.registerStringProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

  describe 'protocol.unregisterProtocol', ->
    it 'returns error when scheme does not exist', (done) ->
      protocol.unregisterProtocol 'not-exist', (error) ->
        assert.notEqual error, null
        done()

  describe 'protocol.registerStringProtocol', ->
    it 'sends string as response', (done) ->
      handler = (request, callback) -> callback(text)
      protocol.registerStringProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'sends object as response', (done) ->
      handler = (request, callback) -> callback(data: text, mimeType: 'text/html')
      protocol.registerStringProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data, statux, request) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'fails when sending object other than string', (done) ->
      handler = (request, callback) -> callback(new Date)
      protocol.registerBufferProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

  describe 'protocol.registerBufferProtocol', ->
    buffer = new Buffer(text)

    it 'sends Buffer as response', (done) ->
      handler = (request, callback) -> callback(buffer)
      protocol.registerBufferProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'sends object as response', (done) ->
      handler = (request, callback) -> callback(data: buffer, mimeType: 'text/html')
      protocol.registerBufferProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data, statux, request) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'fails when sending string', (done) ->
      handler = (request, callback) -> callback(text)
      protocol.registerBufferProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

  describe 'protocol.registerFileProtocol', ->
    filePath = path.join __dirname, 'fixtures', 'asar', 'a.asar', 'file1'
    fileContent = require('fs').readFileSync(filePath)

    normalPath = path.join __dirname, 'fixtures', 'pages', 'a.html'
    normalContent = require('fs').readFileSync(normalPath)

    it 'sends file path as response', (done) ->
      handler = (request, callback) -> callback(filePath)
      protocol.registerFileProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, String(fileContent)
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'sends object as response', (done) ->
      handler = (request, callback) -> callback(path: filePath)
      protocol.registerFileProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data, statux, request) ->
            assert.equal data, String(fileContent)
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'can send normal file', (done) ->
      handler = (request, callback) -> callback(normalPath)
      protocol.registerFileProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, String(normalContent)
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'fails when sending unexist-file', (done) ->
      fakeFilePath = path.join __dirname, 'fixtures', 'asar', 'a.asar', 'not-exist'
      handler = (request, callback) -> callback(fakeFilePath)
      protocol.registerBufferProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

    it 'fails when sending unsupported content', (done) ->
      handler = (request, callback) -> callback(new Date)
      protocol.registerBufferProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

  describe 'protocol.registerHttpProtocol', ->
    it 'sends url as response', (done) ->
      server = http.createServer (req, res) ->
        assert.notEqual req.headers.accept, ''
        res.end(text)
        server.close()
      server.listen 0, '127.0.0.1', ->
        {port} = server.address()
        url = "http://127.0.0.1:#{port}"
        handler = (request, callback) -> callback({url})
        protocol.registerHttpProtocol protocolName, handler, (error) ->
          return done(error) if error
          $.ajax
            url: "#{protocolName}://fake-host"
            success: (data) ->
              assert.equal data, text
              done()
            error: (xhr, errorType, error) ->
              done(error)

    it 'fails when sending invalid url', (done) ->
      handler = (request, callback) -> callback({url: 'url'})
      protocol.registerHttpProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

    it 'fails when sending unsupported content', (done) ->
      handler = (request, callback) -> callback(new Date)
      protocol.registerHttpProtocol protocolName, handler, (error) ->
        return done(error) if error
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

  describe 'protocol.isProtocolHandled', ->
    it 'returns true for file:', (done) ->
      protocol.isProtocolHandled 'file', (result) ->
        assert.equal result, true
        done()

    it 'returns true for http:', (done) ->
      protocol.isProtocolHandled 'http', (result) ->
        assert.equal result, true
        done()

    it 'returns true for https:', (done) ->
      protocol.isProtocolHandled 'https', (result) ->
        assert.equal result, true
        done()

    it 'returns false when scheme is not registred', (done) ->
      protocol.isProtocolHandled 'no-exist', (result) ->
        assert.equal result, false
        done()

    it 'returns true for custom protocol', (done) ->
      emptyHandler = (request, callback) -> callback()
      protocol.registerStringProtocol protocolName, emptyHandler, (error) ->
        assert.equal error, null
        protocol.isProtocolHandled protocolName, (result) ->
          assert.equal result, true
          done()

    it 'returns true for intercepted protocol', (done) ->
      emptyHandler = (request, callback) -> callback()
      protocol.interceptStringProtocol 'http', emptyHandler, (error) ->
        assert.equal error, null
        protocol.isProtocolHandled 'http', (result) ->
          assert.equal result, true
          done()

  describe 'protocol.intercept(Any)Protocol', ->
    emptyHandler = (request, callback) -> callback()

    it 'throws error when scheme is already intercepted', (done) ->
      protocol.interceptStringProtocol 'http', emptyHandler, (error) ->
        assert.equal error, null
        protocol.interceptBufferProtocol 'http', emptyHandler, (error) ->
          assert.notEqual error, null
          done()

    it 'does not crash when handler is called twice', (done) ->
      doubleHandler = (request, callback) ->
        try
          callback(text)
          callback()
        catch
      protocol.interceptStringProtocol 'http', doubleHandler, (error) ->
        return done(error) if error
        $.ajax
          url: 'http://fake-host'
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'sends error when callback is called with nothing', (done) ->
      protocol.interceptBufferProtocol 'http', emptyHandler, (error) ->
        return done(error) if error
        $.ajax
          url: 'http://fake-host'
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

  describe 'protocol.interceptStringProtocol', ->
    it 'can intercept http protocol', (done) ->
      handler = (request, callback) -> callback(text)
      protocol.interceptStringProtocol 'http', handler, (error) ->
        return done(error) if error
        $.ajax
          url: 'http://fake-host'
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'can set content-type', (done) ->
      handler = (request, callback) ->
        callback({mimeType: 'application/json', data: '{"value": 1}'})
      protocol.interceptStringProtocol 'http', handler, (error) ->
        return done(error) if error
        $.ajax
          url: 'http://fake-host'
          success: (data) ->
            assert.equal typeof(data), 'object'
            assert.equal data.value, 1
            done()
          error: (xhr, errorType, error) ->
            done(error)

  describe 'protocol.interceptBufferProtocol', ->
    it 'can intercept http protocol', (done) ->
      handler = (request, callback) -> callback(new Buffer(text))
      protocol.interceptBufferProtocol 'http', handler, (error) ->
        return done(error) if error
        $.ajax
          url: 'http://fake-host'
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

  describe 'protocol.uninterceptProtocol', ->
    it 'returns error when scheme does not exist', (done) ->
      protocol.uninterceptProtocol 'not-exist', (error) ->
        assert.notEqual error, null
        done()

    it 'returns error when scheme is not intercepted', (done) ->
      protocol.uninterceptProtocol 'http', (error) ->
        assert.notEqual error, null
        done()
