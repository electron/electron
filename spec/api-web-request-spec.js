/* globals $ */

const assert = require('assert')
const http = require('http')
const qs = require('querystring')
const remote = require('electron').remote
const session = remote.session

describe('webRequest module', function () {
  var ses = session.defaultSession
  var server = http.createServer(function (req, res) {
    res.setHeader('Custom', ['Header'])
    var content = req.url
    if (req.headers.accept === '*/*;test/header') {
      content += 'header/received'
    }
    res.end(content)
  })
  var defaultURL = null

  before(function (done) {
    server.listen(0, '127.0.0.1', function () {
      var port = server.address().port
      defaultURL = 'http://127.0.0.1:' + port + '/'
      done()
    })
  })

  after(function () {
    server.close()
  })

  describe('webRequest.onBeforeRequest', function () {
    afterEach(function () {
      ses.webRequest.onBeforeRequest(null)
    })

    it('can cancel the request', function (done) {
      ses.webRequest.onBeforeRequest(function (details, callback) {
        callback({
          cancel: true
        })
      })
      $.ajax({
        url: defaultURL,
        success: function () {
          done('unexpected success')
        },
        error: function () {
          done()
        }
      })
    })

    it('can filter URLs', function (done) {
      var filter = {
        urls: [defaultURL + 'filter/*']
      }
      ses.webRequest.onBeforeRequest(filter, function (details, callback) {
        callback({
          cancel: true
        })
      })
      $.ajax({
        url: defaultURL + 'nofilter/test',
        success: function (data) {
          assert.equal(data, '/nofilter/test')
          $.ajax({
            url: defaultURL + 'filter/test',
            success: function () {
              done('unexpected success')
            },
            error: function () {
              done()
            }
          })
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })

    it('receives details object', function (done) {
      ses.webRequest.onBeforeRequest(function (details, callback) {
        assert.equal(typeof details.id, 'number')
        assert.equal(typeof details.timestamp, 'number')
        assert.equal(details.url, defaultURL)
        assert.equal(details.method, 'GET')
        assert.equal(details.resourceType, 'xhr')
        assert(!details.uploadData)
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })

    it('receives post data in details object', function (done) {
      var postData = {
        name: 'post test',
        type: 'string'
      }
      ses.webRequest.onBeforeRequest(function (details, callback) {
        assert.equal(details.url, defaultURL)
        assert.equal(details.method, 'POST')
        assert.equal(details.uploadData.length, 1)
        var data = qs.parse(details.uploadData[0].bytes.toString())
        assert.deepEqual(data, postData)
        callback({
          cancel: true
        })
      })
      $.ajax({
        url: defaultURL,
        type: 'POST',
        data: postData,
        success: function () {},
        error: function () {
          done()
        }
      })
    })

    it('can redirect the request', function (done) {
      ses.webRequest.onBeforeRequest(function (details, callback) {
        if (details.url === defaultURL) {
          callback({
            redirectURL: defaultURL + 'redirect'
          })
        } else {
          callback({})
        }
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/redirect')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onBeforeSendHeaders', function () {
    afterEach(function () {
      ses.webRequest.onBeforeSendHeaders(null)
    })

    it('receives details object', function (done) {
      ses.webRequest.onBeforeSendHeaders(function (details, callback) {
        assert.equal(typeof details.requestHeaders, 'object')
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })

    it('can change the request headers', function (done) {
      ses.webRequest.onBeforeSendHeaders(function (details, callback) {
        var requestHeaders = details.requestHeaders
        requestHeaders.Accept = '*/*;test/header'
        callback({
          requestHeaders: requestHeaders
        })
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/header/received')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })

    it('resets the whole headers', function (done) {
      var requestHeaders = {
        Test: 'header'
      }
      ses.webRequest.onBeforeSendHeaders(function (details, callback) {
        callback({
          requestHeaders: requestHeaders
        })
      })
      ses.webRequest.onSendHeaders(function (details) {
        assert.deepEqual(details.requestHeaders, requestHeaders)
        done()
      })
      $.ajax({
        url: defaultURL,
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onSendHeaders', function () {
    afterEach(function () {
      ses.webRequest.onSendHeaders(null)
    })

    it('receives details object', function (done) {
      ses.webRequest.onSendHeaders(function (details) {
        assert.equal(typeof details.requestHeaders, 'object')
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onHeadersReceived', function () {
    afterEach(function () {
      ses.webRequest.onHeadersReceived(null)
    })

    it('receives details object', function (done) {
      ses.webRequest.onHeadersReceived(function (details, callback) {
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK')
        assert.equal(details.statusCode, 200)
        assert.equal(details.responseHeaders['Custom'], 'Header')
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })

    it('can change the response header', function (done) {
      ses.webRequest.onHeadersReceived(function (details, callback) {
        var responseHeaders = details.responseHeaders
        responseHeaders['Custom'] = ['Changed']
        callback({
          responseHeaders: responseHeaders
        })
      })
      $.ajax({
        url: defaultURL,
        success: function (data, status, xhr) {
          assert.equal(xhr.getResponseHeader('Custom'), 'Changed')
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })

    it('does not change header by default', function (done) {
      ses.webRequest.onHeadersReceived(function (details, callback) {
        callback({})
      })
      $.ajax({
        url: defaultURL,
        success: function (data, status, xhr) {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header')
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onResponseStarted', function () {
    afterEach(function () {
      ses.webRequest.onResponseStarted(null)
    })

    it('receives details object', function (done) {
      ses.webRequest.onResponseStarted(function (details) {
        assert.equal(typeof details.fromCache, 'boolean')
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK')
        assert.equal(details.statusCode, 200)
        assert.equal(details.responseHeaders['Custom'], 'Header')
      })
      $.ajax({
        url: defaultURL,
        success: function (data, status, xhr) {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header')
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onBeforeRedirect', function () {
    afterEach(function () {
      ses.webRequest.onBeforeRedirect(null)
      ses.webRequest.onBeforeRequest(null)
    })

    it('receives details object', function (done) {
      var redirectURL = defaultURL + 'redirect'
      ses.webRequest.onBeforeRequest(function (details, callback) {
        if (details.url === defaultURL) {
          callback({
            redirectURL: redirectURL
          })
        } else {
          callback({})
        }
      })
      ses.webRequest.onBeforeRedirect(function (details) {
        assert.equal(typeof details.fromCache, 'boolean')
        assert.equal(details.statusLine, 'HTTP/1.1 307 Internal Redirect')
        assert.equal(details.statusCode, 307)
        assert.equal(details.redirectURL, redirectURL)
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/redirect')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onCompleted', function () {
    afterEach(function () {
      ses.webRequest.onCompleted(null)
    })

    it('receives details object', function (done) {
      ses.webRequest.onCompleted(function (details) {
        assert.equal(typeof details.fromCache, 'boolean')
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK')
        assert.equal(details.statusCode, 200)
      })
      $.ajax({
        url: defaultURL,
        success: function (data) {
          assert.equal(data, '/')
          done()
        },
        error: function (xhr, errorType) {
          done(errorType)
        }
      })
    })
  })

  describe('webRequest.onErrorOccurred', function () {
    afterEach(function () {
      ses.webRequest.onErrorOccurred(null)
      ses.webRequest.onBeforeRequest(null)
    })

    it('receives details object', function (done) {
      ses.webRequest.onBeforeRequest(function (details, callback) {
        callback({
          cancel: true
        })
      })
      ses.webRequest.onErrorOccurred(function (details) {
        assert.equal(details.error, 'net::ERR_BLOCKED_BY_CLIENT')
        done()
      })
      $.ajax({
        url: defaultURL,
        success: function () {
          done('unexpected success')
        }
      })
    })
  })
})
