const assert = require('assert')
const http = require('http')
const qs = require('querystring')
const remote = require('electron').remote
const session = remote.session

/* The whole webRequest API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

describe('webRequest module', () => {
  const ses = session.defaultSession
  const server = http.createServer((req, res) => {
    if (req.url === '/serverRedirect') {
      res.statusCode = 301
      res.setHeader('Location', 'http://' + req.rawHeaders[1])
      res.end()
    } else {
      res.setHeader('Custom', ['Header'])
      let content = req.url
      if (req.headers.accept === '*/*;test/header') {
        content += 'header/received'
      }
      res.end(content)
    }
  })
  let defaultURL = null

  before((done) => {
    server.listen(0, '127.0.0.1', () => {
      const port = server.address().port
      defaultURL = 'http://127.0.0.1:' + port + '/'
      done()
    })
  })

  after(() => {
    server.close()
  })

  describe('webRequest.onBeforeRequest', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRequest(null)
    })

    it('can cancel the request', (done) => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({
          cancel: true
        })
      })
      $.ajax({
        url: defaultURL,
        success: () => {
          done('unexpected success')
        },
        error: () => {
          done()
        }
      })
    })

    it('can filter URLs', (done) => {
      const filter = { urls: [defaultURL + 'filter/*'] }
      ses.webRequest.onBeforeRequest(filter, (details, callback) => {
        callback({cancel: true})
      })
      $.ajax({
        url: `${defaultURL}nofilter/test`,
        success: (data) => {
          assert.equal(data, '/nofilter/test')
          $.ajax({
            url: `${defaultURL}filter/test`,
            success: () => done('unexpected success'),
            error: () => done()
          })
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('receives details object', (done) => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        assert.equal(typeof details.id, 'number')
        assert.equal(typeof details.timestamp, 'number')
        assert.equal(typeof details.webContentsId, 'number')
        assert.equal(details.url, defaultURL)
        assert.equal(details.method, 'GET')
        assert.equal(details.resourceType, 'xhr')
        assert(!details.uploadData)
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('receives post data in details object', (done) => {
      const postData = {
        name: 'post test',
        type: 'string'
      }
      ses.webRequest.onBeforeRequest((details, callback) => {
        assert.equal(details.url, defaultURL)
        assert.equal(details.method, 'POST')
        assert.equal(details.uploadData.length, 1)
        const data = qs.parse(details.uploadData[0].bytes.toString())
        assert.deepEqual(data, postData)
        callback({ cancel: true })
      })
      $.ajax({
        url: defaultURL,
        type: 'POST',
        data: postData,
        success: () => {},
        error: () => done()
      })
    })

    it('can redirect the request', (done) => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: `${defaultURL}redirect` })
        } else {
          callback({})
        }
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/redirect')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })
  })

  describe('webRequest.onBeforeSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onBeforeSendHeaders(null)
    })

    it('receives details object', (done) => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        assert.equal(typeof details.requestHeaders, 'object')
        assert.equal(details.requestHeaders['Foo.Bar'], 'baz')
        callback({})
      })
      $.ajax({
        url: defaultURL,
        headers: { 'Foo.Bar': 'baz' },
        success: (data) => {
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('can change the request headers', (done) => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        const requestHeaders = details.requestHeaders
        requestHeaders.Accept = '*/*;test/header'
        callback({ requestHeaders: requestHeaders })
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/header/received')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('resets the whole headers', (done) => {
      const requestHeaders = {
        Test: 'header'
      }
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        callback({ requestHeaders: requestHeaders })
      })
      ses.webRequest.onSendHeaders((details) => {
        assert.deepEqual(details.requestHeaders, requestHeaders)
        done()
      })
      $.ajax({
        url: defaultURL,
        error: (xhr, errorType) => done(errorType)
      })
    })
  })

  describe('webRequest.onSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onSendHeaders(null)
    })

    it('receives details object', (done) => {
      ses.webRequest.onSendHeaders((details) => {
        assert.equal(typeof details.requestHeaders, 'object')
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })
  })

  describe('webRequest.onHeadersReceived', () => {
    afterEach(() => {
      ses.webRequest.onHeadersReceived(null)
    })

    it('receives details object', (done) => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK')
        assert.equal(details.statusCode, 200)
        assert.equal(details.responseHeaders['Custom'], 'Header')
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('can change the response header', (done) => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders
        responseHeaders['Custom'] = ['Changed']
        callback({ responseHeaders: responseHeaders })
      })
      $.ajax({
        url: defaultURL,
        success: (data, status, xhr) => {
          assert.equal(xhr.getResponseHeader('Custom'), 'Changed')
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('does not change header by default', (done) => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: (data, status, xhr) => {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header')
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('follows server redirect', (done) => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders
        callback({ responseHeaders: responseHeaders })
      })
      $.ajax({
        url: defaultURL + 'serverRedirect',
        success: (data, status, xhr) => {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })

    it('can change the header status', (done) => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders
        callback({
          responseHeaders: responseHeaders,
          statusLine: 'HTTP/1.1 404 Not Found'
        })
      })
      $.ajax({
        url: defaultURL,
        success: (data, status, xhr) => {},
        error: (xhr, errorType) => {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header')
          done()
        }
      })
    })
  })

  describe('webRequest.onResponseStarted', () => {
    afterEach(() => {
      ses.webRequest.onResponseStarted(null)
    })

    it('receives details object', (done) => {
      ses.webRequest.onResponseStarted((details) => {
        assert.equal(typeof details.fromCache, 'boolean')
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK')
        assert.equal(details.statusCode, 200)
        assert.equal(details.responseHeaders['Custom'], 'Header')
      })
      $.ajax({
        url: defaultURL,
        success: (data, status, xhr) => {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header')
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })
  })

  describe('webRequest.onBeforeRedirect', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRedirect(null)
      ses.webRequest.onBeforeRequest(null)
    })

    it('receives details object', (done) => {
      const redirectURL = defaultURL + 'redirect'
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: redirectURL })
        } else {
          callback({})
        }
      })
      ses.webRequest.onBeforeRedirect((details) => {
        assert.equal(typeof details.fromCache, 'boolean')
        assert.equal(details.statusLine, 'HTTP/1.1 307 Internal Redirect')
        assert.equal(details.statusCode, 307)
        assert.equal(details.redirectURL, redirectURL)
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/redirect')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })
  })

  describe('webRequest.onCompleted', () => {
    afterEach(() => {
      ses.webRequest.onCompleted(null)
    })

    it('receives details object', (done) => {
      ses.webRequest.onCompleted((details) => {
        assert.equal(typeof details.fromCache, 'boolean')
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK')
        assert.equal(details.statusCode, 200)
      })
      $.ajax({
        url: defaultURL,
        success: (data) => {
          assert.equal(data, '/')
          done()
        },
        error: (xhr, errorType) => done(errorType)
      })
    })
  })

  describe('webRequest.onErrorOccurred', () => {
    afterEach(() => {
      ses.webRequest.onErrorOccurred(null)
      ses.webRequest.onBeforeRequest(null)
    })

    it('receives details object', (done) => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: true })
      })
      ses.webRequest.onErrorOccurred((details) => {
        assert.equal(details.error, 'net::ERR_BLOCKED_BY_CLIENT')
        done()
      })
      $.ajax({
        url: defaultURL,
        success: () => done('unexpected success')
      })
    })
  })
})
