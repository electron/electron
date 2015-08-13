assert   = require 'assert'
http     = require 'http'
path     = require 'path'
remote   = require 'remote'
protocol = remote.require 'protocol'

describe 'protocol module', ->
  protocolName = 'sp'
  text = 'valar morghulis'

  afterEach (done) ->
    protocol.unregisterProtocol protocolName, -> done()

  describe 'protocol.unregisterProtocol', ->
    it 'returns error when scheme does not exist', (done) ->
      protocol.unregisterProtocol 'not-exist', (error) ->
        assert.notEqual error, null
        done()

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
        callback(text)
        callback()
      protocol.registerStringProtocol protocolName, doubleHandler, (error) ->
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

    it 'sends error when callback is called with nothing', (done) ->
      protocol.registerBufferProtocol protocolName, emptyHandler, (error) ->
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
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            assert.equal data, text
            done()
          error: (xhr, errorType, error) ->
            done(error)

  describe 'protocol.registerStringProtocol', ->
    it 'sends string as response', (done) ->
      handler = (request, callback) -> callback(text)
      protocol.registerStringProtocol protocolName, handler, (error) ->
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
        $.ajax
          url: "#{protocolName}://fake-host"
          success: (data) ->
            done('request succeeded but it should not')
          error: (xhr, errorType, error) ->
            assert.equal errorType, 'error'
            done()

  describe 'protocol.isHandledProtocol', ->
    it 'returns true for file:', (done) ->
      protocol.isHandledProtocol 'file', (result) ->
        assert.equal result, true
        done()

    it 'returns true for http:', (done) ->
      protocol.isHandledProtocol 'http', (result) ->
        assert.equal result, true
        done()

    it 'returns true for https:', (done) ->
      protocol.isHandledProtocol 'https', (result) ->
        assert.equal result, true
        done()

    it 'returns false when scheme is not registred', (done) ->
      protocol.isHandledProtocol 'atom', (result) ->
        assert.equal result, false
        done()
