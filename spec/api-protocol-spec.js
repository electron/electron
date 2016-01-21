const assert = require('assert');
const http = require('http');
const path = require('path');
const qs = require('querystring');
const remote = require('electron').remote;
const protocol = remote.require('electron').protocol;

describe('protocol module', function() {
  var postData, protocolName, text;
  protocolName = 'sp';
  text = 'valar morghulis';
  postData = {
    name: 'post test',
    type: 'string'
  };
  afterEach(function(done) {
    return protocol.unregisterProtocol(protocolName, function() {
      return protocol.uninterceptProtocol('http', function() {
        return done();
      });
    });
  });
  describe('protocol.register(Any)Protocol', function() {
    var emptyHandler;
    emptyHandler = function(request, callback) {
      return callback();
    };
    it('throws error when scheme is already registered', function(done) {
      return protocol.registerStringProtocol(protocolName, emptyHandler, function(error) {
        assert.equal(error, null);
        return protocol.registerBufferProtocol(protocolName, emptyHandler, function(error) {
          assert.notEqual(error, null);
          return done();
        });
      });
    });
    it('does not crash when handler is called twice', function(done) {
      var doubleHandler;
      doubleHandler = function(request, callback) {
        try {
          callback(text);
          return callback();
        } catch (error) {
          // Ignore error
        }
      };
      return protocol.registerStringProtocol(protocolName, doubleHandler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sends error when callback is called with nothing', function(done) {
      return protocol.registerBufferProtocol(protocolName, emptyHandler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
    return it('does not crash when callback is called in next tick', function(done) {
      var handler;
      handler = function(request, callback) {
        return setImmediate(function() {
          return callback(text);
        });
      };
      return protocol.registerStringProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
  });
  describe('protocol.unregisterProtocol', function() {
    return it('returns error when scheme does not exist', function(done) {
      return protocol.unregisterProtocol('not-exist', function(error) {
        assert.notEqual(error, null);
        return done();
      });
    });
  });
  describe('protocol.registerStringProtocol', function() {
    it('sends string as response', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(text);
      };
      return protocol.registerStringProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sets Access-Control-Allow-Origin', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(text);
      };
      return protocol.registerStringProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data, status, request) {
            assert.equal(data, text);
            assert.equal(request.getResponseHeader('Access-Control-Allow-Origin'), '*');
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sends object as response', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback({
          data: text,
          mimeType: 'text/html'
        });
      };
      return protocol.registerStringProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    return it('fails when sending object other than string', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(new Date);
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
  });
  describe('protocol.registerBufferProtocol', function() {
    var buffer;
    buffer = new Buffer(text);
    it('sends Buffer as response', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(buffer);
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sets Access-Control-Allow-Origin', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(buffer);
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data, status, request) {
            assert.equal(data, text);
            assert.equal(request.getResponseHeader('Access-Control-Allow-Origin'), '*');
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sends object as response', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback({
          data: buffer,
          mimeType: 'text/html'
        });
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    return it('fails when sending string', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(text);
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
  });
  describe('protocol.registerFileProtocol', function() {
    var fileContent, filePath, normalContent, normalPath;
    filePath = path.join(__dirname, 'fixtures', 'asar', 'a.asar', 'file1');
    fileContent = require('fs').readFileSync(filePath);
    normalPath = path.join(__dirname, 'fixtures', 'pages', 'a.html');
    normalContent = require('fs').readFileSync(normalPath);
    it('sends file path as response', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(filePath);
      };
      return protocol.registerFileProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, String(fileContent));
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sets Access-Control-Allow-Origin', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(filePath);
      };
      return protocol.registerFileProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data, status, request) {
            assert.equal(data, String(fileContent));
            assert.equal(request.getResponseHeader('Access-Control-Allow-Origin'), '*');
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('sends object as response', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback({
          path: filePath
        });
      };
      return protocol.registerFileProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, String(fileContent));
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('can send normal file', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(normalPath);
      };
      return protocol.registerFileProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function(data) {
            assert.equal(data, String(normalContent));
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('fails when sending unexist-file', function(done) {
      var fakeFilePath, handler;
      fakeFilePath = path.join(__dirname, 'fixtures', 'asar', 'a.asar', 'not-exist');
      handler = function(request, callback) {
        return callback(fakeFilePath);
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
    return it('fails when sending unsupported content', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(new Date);
      };
      return protocol.registerBufferProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
  });
  describe('protocol.registerHttpProtocol', function() {
    it('sends url as response', function(done) {
      var server;
      server = http.createServer(function(req, res) {
        assert.notEqual(req.headers.accept, '');
        res.end(text);
        return server.close();
      });
      return server.listen(0, '127.0.0.1', function() {
        var handler, port, url;
        port = server.address().port;
        url = "http://127.0.0.1:" + port;
        handler = function(request, callback) {
          return callback({
            url: url
          });
        };
        return protocol.registerHttpProtocol(protocolName, handler, function(error) {
          if (error) {
            return done(error);
          }
          return $.ajax({
            url: protocolName + "://fake-host",
            success: function(data) {
              assert.equal(data, text);
              return done();
            },
            error: function(xhr, errorType, error) {
              return done(error);
            }
          });
        });
      });
    });
    it('fails when sending invalid url', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback({
          url: 'url'
        });
      };
      return protocol.registerHttpProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
    return it('fails when sending unsupported content', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(new Date);
      };
      return protocol.registerHttpProtocol(protocolName, handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: protocolName + "://fake-host",
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
  });
  describe('protocol.isProtocolHandled', function() {
    it('returns true for file:', function(done) {
      return protocol.isProtocolHandled('file', function(result) {
        assert.equal(result, true);
        return done();
      });
    });
    it('returns true for http:', function(done) {
      return protocol.isProtocolHandled('http', function(result) {
        assert.equal(result, true);
        return done();
      });
    });
    it('returns true for https:', function(done) {
      return protocol.isProtocolHandled('https', function(result) {
        assert.equal(result, true);
        return done();
      });
    });
    it('returns false when scheme is not registred', function(done) {
      return protocol.isProtocolHandled('no-exist', function(result) {
        assert.equal(result, false);
        return done();
      });
    });
    it('returns true for custom protocol', function(done) {
      var emptyHandler;
      emptyHandler = function(request, callback) {
        return callback();
      };
      return protocol.registerStringProtocol(protocolName, emptyHandler, function(error) {
        assert.equal(error, null);
        return protocol.isProtocolHandled(protocolName, function(result) {
          assert.equal(result, true);
          return done();
        });
      });
    });
    return it('returns true for intercepted protocol', function(done) {
      var emptyHandler;
      emptyHandler = function(request, callback) {
        return callback();
      };
      return protocol.interceptStringProtocol('http', emptyHandler, function(error) {
        assert.equal(error, null);
        return protocol.isProtocolHandled('http', function(result) {
          assert.equal(result, true);
          return done();
        });
      });
    });
  });
  describe('protocol.intercept(Any)Protocol', function() {
    var emptyHandler;
    emptyHandler = function(request, callback) {
      return callback();
    };
    it('throws error when scheme is already intercepted', function(done) {
      return protocol.interceptStringProtocol('http', emptyHandler, function(error) {
        assert.equal(error, null);
        return protocol.interceptBufferProtocol('http', emptyHandler, function(error) {
          assert.notEqual(error, null);
          return done();
        });
      });
    });
    it('does not crash when handler is called twice', function(done) {
      var doubleHandler;
      doubleHandler = function(request, callback) {
        try {
          callback(text);
          return callback();
        } catch (error) {
          // Ignore error
        }
      };
      return protocol.interceptStringProtocol('http', doubleHandler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: 'http://fake-host',
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    return it('sends error when callback is called with nothing', function(done) {
      if (process.env.TRAVIS === 'true') {
        return done();
      }
      return protocol.interceptBufferProtocol('http', emptyHandler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: 'http://fake-host',
          success: function() {
            return done('request succeeded but it should not');
          },
          error: function(xhr, errorType) {
            assert.equal(errorType, 'error');
            return done();
          }
        });
      });
    });
  });
  describe('protocol.interceptStringProtocol', function() {
    it('can intercept http protocol', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(text);
      };
      return protocol.interceptStringProtocol('http', handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: 'http://fake-host',
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    it('can set content-type', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback({
          mimeType: 'application/json',
          data: '{"value": 1}'
        });
      };
      return protocol.interceptStringProtocol('http', handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: 'http://fake-host',
          success: function(data) {
            assert.equal(typeof data, 'object');
            assert.equal(data.value, 1);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    return it('can receive post data', function(done) {
      var handler;
      handler = function(request, callback) {
        var uploadData;
        uploadData = request.uploadData[0].bytes.toString();
        return callback({
          data: uploadData
        });
      };
      return protocol.interceptStringProtocol('http', handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: "http://fake-host",
          type: "POST",
          data: postData,
          success: function(data) {
            assert.deepEqual(qs.parse(data), postData);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
  });
  describe('protocol.interceptBufferProtocol', function() {
    it('can intercept http protocol', function(done) {
      var handler;
      handler = function(request, callback) {
        return callback(new Buffer(text));
      };
      return protocol.interceptBufferProtocol('http', handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: 'http://fake-host',
          success: function(data) {
            assert.equal(data, text);
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
    return it('can receive post data', function(done) {
      var handler;
      handler = function(request, callback) {
        var uploadData;
        uploadData = request.uploadData[0].bytes;
        return callback(uploadData);
      };
      return protocol.interceptBufferProtocol('http', handler, function(error) {
        if (error) {
          return done(error);
        }
        return $.ajax({
          url: "http://fake-host",
          type: "POST",
          data: postData,
          success: function(data) {
            assert.equal(data, $.param(postData));
            return done();
          },
          error: function(xhr, errorType, error) {
            return done(error);
          }
        });
      });
    });
  });
  describe('protocol.interceptHttpProtocol', function() {
    return it('can send POST request', function(done) {
      var server;
      server = http.createServer(function(req, res) {
        var body;
        body = '';
        req.on('data', function(chunk) {
          return body += chunk;
        });
        req.on('end', function() {
          return res.end(body);
        });
        return server.close();
      });
      return server.listen(0, '127.0.0.1', function() {
        var handler, port, url;
        port = server.address().port;
        url = "http://127.0.0.1:" + port;
        handler = function(request, callback) {
          var data;
          data = {
            url: url,
            method: 'POST',
            uploadData: {
              contentType: 'application/x-www-form-urlencoded',
              data: request.uploadData[0].bytes.toString()
            },
            session: null
          };
          return callback(data);
        };
        return protocol.interceptHttpProtocol('http', handler, function(error) {
          if (error) {
            return done(error);
          }
          return $.ajax({
            url: "http://fake-host",
            type: "POST",
            data: postData,
            success: function(data) {
              assert.deepEqual(qs.parse(data), postData);
              return done();
            },
            error: function(xhr, errorType, error) {
              return done(error);
            }
          });
        });
      });
    });
  });
  return describe('protocol.uninterceptProtocol', function() {
    it('returns error when scheme does not exist', function(done) {
      return protocol.uninterceptProtocol('not-exist', function(error) {
        assert.notEqual(error, null);
        return done();
      });
    });
    return it('returns error when scheme is not intercepted', function(done) {
      return protocol.uninterceptProtocol('http', function(error) {
        assert.notEqual(error, null);
        return done();
      });
    });
  });
});
