const assert = require('assert');
const http = require('http');
const remote = require('electron').remote;
const session = remote.session;

describe('webRequest module', function() {
  var defaultURL, server, ses;
  ses = session.defaultSession;
  server = http.createServer(function(req, res) {
    var content;
    res.setHeader('Custom', ['Header']);
    content = req.url;
    if (req.headers.accept === '*/*;test/header') {
      content += 'header/received';
    }
    return res.end(content);
  });
  defaultURL = null;
  before(function(done) {
    return server.listen(0, '127.0.0.1', function() {
      var port;
      port = server.address().port;
      defaultURL = "http://127.0.0.1:" + port + "/";
      return done();
    });
  });
  after(function() {
    return server.close();
  });
  describe('webRequest.onBeforeRequest', function() {
    afterEach(function() {
      return ses.webRequest.onBeforeRequest(null);
    });
    it('can cancel the request', function(done) {
      ses.webRequest.onBeforeRequest(function(details, callback) {
        return callback({
          cancel: true
        });
      });
      return $.ajax({
        url: defaultURL,
        success: function() {
          return done('unexpected success');
        },
        error: function() {
          return done();
        }
      });
    });
    it('can filter URLs', function(done) {
      var filter;
      filter = {
        urls: [defaultURL + "filter/*"]
      };
      ses.webRequest.onBeforeRequest(filter, function(details, callback) {
        return callback({
          cancel: true
        });
      });
      return $.ajax({
        url: defaultURL + "nofilter/test",
        success: function(data) {
          assert.equal(data, '/nofilter/test');
          return $.ajax({
            url: defaultURL + "filter/test",
            success: function() {
              return done('unexpected success');
            },
            error: function() {
              return done();
            }
          });
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
    it('receives details object', function(done) {
      ses.webRequest.onBeforeRequest(function(details, callback) {
        assert.equal(typeof details.id, 'number');
        assert.equal(typeof details.timestamp, 'number');
        assert.equal(details.url, defaultURL);
        assert.equal(details.method, 'GET');
        assert.equal(details.resourceType, 'xhr');
        return callback({});
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
    return it('can redirect the request', function(done) {
      ses.webRequest.onBeforeRequest(function(details, callback) {
        if (details.url === defaultURL) {
          return callback({
            redirectURL: defaultURL + "redirect"
          });
        } else {
          return callback({});
        }
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/redirect');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  describe('webRequest.onBeforeSendHeaders', function() {
    afterEach(function() {
      return ses.webRequest.onBeforeSendHeaders(null);
    });
    it('receives details object', function(done) {
      ses.webRequest.onBeforeSendHeaders(function(details, callback) {
        assert.equal(typeof details.requestHeaders, 'object');
        return callback({});
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
    it('can change the request headers', function(done) {
      ses.webRequest.onBeforeSendHeaders(function(details, callback) {
        var requestHeaders;
        requestHeaders = details.requestHeaders;
        requestHeaders.Accept = '*/*;test/header';
        return callback({
          requestHeaders: requestHeaders
        });
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/header/received');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
    return it('resets the whole headers', function(done) {
      var requestHeaders;
      requestHeaders = {
        Test: 'header'
      };
      ses.webRequest.onBeforeSendHeaders(function(details, callback) {
        return callback({
          requestHeaders: requestHeaders
        });
      });
      ses.webRequest.onSendHeaders(function(details) {
        assert.deepEqual(details.requestHeaders, requestHeaders);
        return done();
      });
      return $.ajax({
        url: defaultURL,
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  describe('webRequest.onSendHeaders', function() {
    afterEach(function() {
      return ses.webRequest.onSendHeaders(null);
    });
    return it('receives details object', function(done) {
      ses.webRequest.onSendHeaders(function(details) {
        return assert.equal(typeof details.requestHeaders, 'object');
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  describe('webRequest.onHeadersReceived', function() {
    afterEach(function() {
      return ses.webRequest.onHeadersReceived(null);
    });
    it('receives details object', function(done) {
      ses.webRequest.onHeadersReceived(function(details, callback) {
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK');
        assert.equal(details.statusCode, 200);
        assert.equal(details.responseHeaders['Custom'], 'Header');
        return callback({});
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
    it('can change the response header', function(done) {
      ses.webRequest.onHeadersReceived(function(details, callback) {
        var responseHeaders;
        responseHeaders = details.responseHeaders;
        responseHeaders['Custom'] = ['Changed'];
        return callback({
          responseHeaders: responseHeaders
        });
      });
      return $.ajax({
        url: defaultURL,
        success: function(data, status, xhr) {
          assert.equal(xhr.getResponseHeader('Custom'), 'Changed');
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
    return it('does not change header by default', function(done) {
      ses.webRequest.onHeadersReceived(function(details, callback) {
        return callback({});
      });
      return $.ajax({
        url: defaultURL,
        success: function(data, status, xhr) {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header');
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  describe('webRequest.onResponseStarted', function() {
    afterEach(function() {
      return ses.webRequest.onResponseStarted(null);
    });
    return it('receives details object', function(done) {
      ses.webRequest.onResponseStarted(function(details) {
        assert.equal(typeof details.fromCache, 'boolean');
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK');
        assert.equal(details.statusCode, 200);
        return assert.equal(details.responseHeaders['Custom'], 'Header');
      });
      return $.ajax({
        url: defaultURL,
        success: function(data, status, xhr) {
          assert.equal(xhr.getResponseHeader('Custom'), 'Header');
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  describe('webRequest.onBeforeRedirect', function() {
    afterEach(function() {
      ses.webRequest.onBeforeRedirect(null);
      return ses.webRequest.onBeforeRequest(null);
    });
    return it('receives details object', function(done) {
      var redirectURL;
      redirectURL = defaultURL + "redirect";
      ses.webRequest.onBeforeRequest(function(details, callback) {
        if (details.url === defaultURL) {
          return callback({
            redirectURL: redirectURL
          });
        } else {
          return callback({});
        }
      });
      ses.webRequest.onBeforeRedirect(function(details) {
        assert.equal(typeof details.fromCache, 'boolean');
        assert.equal(details.statusLine, 'HTTP/1.1 307 Internal Redirect');
        assert.equal(details.statusCode, 307);
        return assert.equal(details.redirectURL, redirectURL);
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/redirect');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  describe('webRequest.onCompleted', function() {
    afterEach(function() {
      return ses.webRequest.onCompleted(null);
    });
    return it('receives details object', function(done) {
      ses.webRequest.onCompleted(function(details) {
        assert.equal(typeof details.fromCache, 'boolean');
        assert.equal(details.statusLine, 'HTTP/1.1 200 OK');
        return assert.equal(details.statusCode, 200);
      });
      return $.ajax({
        url: defaultURL,
        success: function(data) {
          assert.equal(data, '/');
          return done();
        },
        error: function(xhr, errorType) {
          return done(errorType);
        }
      });
    });
  });
  return describe('webRequest.onErrorOccurred', function() {
    afterEach(function() {
      ses.webRequest.onErrorOccurred(null);
      return ses.webRequest.onBeforeRequest(null);
    });
    return it('receives details object', function(done) {
      ses.webRequest.onBeforeRequest(function(details, callback) {
        return callback({
          cancel: true
        });
      });
      ses.webRequest.onErrorOccurred(function(details) {
        assert.equal(details.error, 'net::ERR_BLOCKED_BY_CLIENT');
        return done();
      });
      return $.ajax({
        url: defaultURL,
        success: function() {
          return done('unexpected success');
        }
      });
    });
  });
});
