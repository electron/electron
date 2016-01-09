assert = require 'assert'
http = require 'http'

{remote} = require 'electron'
{session} = remote

describe 'webRequest module', ->
  ses = session.defaultSession
  server = http.createServer (req, res) ->
    res.setHeader('Custom', ['Header'])
    content = req.url
    if req.headers.accept is '*/*;test/header'
      content += 'header/received'
    res.end content
  defaultURL = null

  before (done) ->
    server.listen 0, '127.0.0.1', ->
      {port} = server.address()
      defaultURL = "http://127.0.0.1:#{port}/"
      done()
  after ->
    server.close()

  describe 'webRequest.onBeforeRequest', ->
    afterEach ->
      ses.webRequest.onBeforeRequest null

    it 'can cancel the request', (done) ->
      ses.webRequest.onBeforeRequest (details, callback) ->
        callback(cancel: true)
      $.ajax
        url: defaultURL
        success: (data) -> done('unexpected success')
        error: (xhr, errorType, error) -> done()

    it 'can filter URLs', (done) ->
      filter = urls: ["#{defaultURL}filter/*"]
      ses.webRequest.onBeforeRequest filter, (details, callback) ->
        callback(cancel: true)
      $.ajax
        url: "#{defaultURL}nofilter/test"
        success: (data) ->
          assert.equal data, '/nofilter/test'
          $.ajax
            url: "#{defaultURL}filter/test"
            success: (data) -> done('unexpected success')
            error: (xhr, errorType, error) -> done()
        error: (xhr, errorType, error) -> done(errorType)

    it 'receives details object', (done) ->
      ses.webRequest.onBeforeRequest (details, callback) ->
        assert.equal typeof details.id, 'number'
        assert.equal typeof details.timestamp, 'number'
        assert.equal details.url, defaultURL
        assert.equal details.method, 'GET'
        assert.equal details.resourceType, 'xhr'
        callback({})
      $.ajax
        url: defaultURL
        success: (data) ->
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

    it 'can redirect the request', (done) ->
      ses.webRequest.onBeforeRequest (details, callback) ->
        if details.url is defaultURL
          callback(redirectURL: "#{defaultURL}redirect")
        else
          callback({})
      $.ajax
        url: defaultURL
        success: (data) ->
          assert.equal data, '/redirect'
          done()
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onBeforeSendHeaders', ->
    afterEach ->
      ses.webRequest.onBeforeSendHeaders null

    it 'receives details object', (done) ->
      ses.webRequest.onBeforeSendHeaders (details, callback) ->
        assert.equal typeof details.requestHeaders, 'object'
        callback({})
      $.ajax
        url: defaultURL
        success: (data) ->
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

    it 'can change the request headers', (done) ->
      ses.webRequest.onBeforeSendHeaders (details, callback) ->
        {requestHeaders} = details
        requestHeaders.Accept = '*/*;test/header'
        callback({requestHeaders})
      $.ajax
        url: defaultURL
        success: (data, textStatus, request) ->
          assert.equal data, '/header/received'
          done()
        error: (xhr, errorType, error) -> done(errorType)

    it 'resets the whole headers', (done) ->
      requestHeaders = Test: 'header'
      ses.webRequest.onBeforeSendHeaders (details, callback) ->
        callback({requestHeaders})
      ses.webRequest.onSendHeaders (details) ->
        assert.deepEqual details.requestHeaders, requestHeaders
        done()
      $.ajax
        url: defaultURL
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onSendHeaders', ->
    afterEach ->
      ses.webRequest.onSendHeaders null

    it 'receives details object', (done) ->
      ses.webRequest.onSendHeaders (details) ->
        assert.equal typeof details.requestHeaders, 'object'
      $.ajax
        url: defaultURL
        success: (data) ->
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onHeadersReceived', ->
    afterEach ->
      ses.webRequest.onHeadersReceived null

    it 'receives details object', (done) ->
      ses.webRequest.onHeadersReceived (details, callback) ->
        assert.equal details.statusLine, 'HTTP/1.1 200 OK'
        assert.equal details.statusCode, 200
        assert.equal details.responseHeaders['Custom'], 'Header'
        callback({})
      $.ajax
        url: defaultURL
        success: (data) ->
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

    it 'can change the response header', (done) ->
      ses.webRequest.onHeadersReceived (details, callback) ->
        {responseHeaders} = details
        responseHeaders['Custom'] = ['Changed']
        callback({responseHeaders})
      $.ajax
        url: defaultURL
        success: (data, status, xhr) ->
          assert.equal xhr.getResponseHeader('Custom'), 'Changed'
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

    it 'does not change header by default', (done) ->
      ses.webRequest.onHeadersReceived (details, callback) ->
        callback({})
      $.ajax
        url: defaultURL
        success: (data, status, xhr) ->
          assert.equal xhr.getResponseHeader('Custom'), 'Header'
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onResponseStarted', ->
    afterEach ->
      ses.webRequest.onResponseStarted null

    it 'receives details object', (done) ->
      ses.webRequest.onResponseStarted (details) ->
        assert.equal typeof details.fromCache, 'boolean'
        assert.equal details.statusLine, 'HTTP/1.1 200 OK'
        assert.equal details.statusCode, 200
        assert.equal details.responseHeaders['Custom'], 'Header'
      $.ajax
        url: defaultURL
        success: (data, status, xhr) ->
          assert.equal xhr.getResponseHeader('Custom'), 'Header'
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onBeforeRedirect', ->
    afterEach ->
      ses.webRequest.onBeforeRedirect null
      ses.webRequest.onBeforeRequest null

    it 'receives details object', (done) ->
      redirectURL = "#{defaultURL}redirect"
      ses.webRequest.onBeforeRequest (details, callback) ->
        if details.url is defaultURL
          callback({redirectURL})
        else
          callback({})
      ses.webRequest.onBeforeRedirect (details) ->
        assert.equal typeof details.fromCache, 'boolean'
        assert.equal details.statusLine, 'HTTP/1.1 307 Internal Redirect'
        assert.equal details.statusCode, 307
        assert.equal details.redirectURL, redirectURL
      $.ajax
        url: defaultURL
        success: (data, status, xhr) ->
          assert.equal data, '/redirect'
          done()
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onCompleted', ->
    afterEach ->
      ses.webRequest.onCompleted null

    it 'receives details object', (done) ->
      ses.webRequest.onCompleted (details) ->
        assert.equal typeof details.fromCache, 'boolean'
        assert.equal details.statusLine, 'HTTP/1.1 200 OK'
        assert.equal details.statusCode, 200
      $.ajax
        url: defaultURL
        success: (data, status, xhr) ->
          assert.equal data, '/'
          done()
        error: (xhr, errorType, error) -> done(errorType)

  describe 'webRequest.onErrorOccurred', ->
    afterEach ->
      ses.webRequest.onErrorOccurred null
      ses.webRequest.onBeforeRequest null

    it 'receives details object', (done) ->
      ses.webRequest.onBeforeRequest (details, callback) ->
        callback(cancel: true)
      ses.webRequest.onErrorOccurred (details) ->
        assert.equal details.error, 'net::ERR_BLOCKED_BY_CLIENT'
        done()
      $.ajax
        url: defaultURL
        success: (data) -> done('unexpected success')
