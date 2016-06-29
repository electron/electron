const assert = require('assert')
const http = require('http')
const path = require('path')
const qs = require('querystring')
const remote = require('electron').remote
const {BrowserWindow, protocol, webContents} = remote

describe('protocol module', function () {
  var protocolName = 'sp'
  var text = 'valar morghulis'
  var postData = {
    name: 'post test',
    type: 'string'
  }

  afterEach(function (done) {
    protocol.unregisterProtocol(protocolName, function () {
      protocol.uninterceptProtocol('http', function () {
        done()
      })
    })
  })

  describe('protocol.register(Any)Protocol', function () {
    var emptyHandler = function (request, callback) {
      callback()
    }

    it('throws error when scheme is already registered', function (done) {
      protocol.registerStringProtocol(protocolName, emptyHandler, function (error) {
        assert.equal(error, null)
        protocol.registerBufferProtocol(protocolName, emptyHandler, function (error) {
          assert.notEqual(error, null)
          done()
        })
      })
    })

    it('does not crash when handler is called twice', function (done) {
      var doubleHandler = function (request, callback) {
        try {
          callback(text)
          callback()
        } catch (error) {
          // Ignore error
        }
      }
      protocol.registerStringProtocol(protocolName, doubleHandler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sends error when callback is called with nothing', function (done) {
      protocol.registerBufferProtocol(protocolName, emptyHandler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            return done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            return done()
          }
        })
      })
    })

    it('does not crash when callback is called in next tick', function (done) {
      var handler = function (request, callback) {
        setImmediate(function () {
          callback(text)
        })
      }
      protocol.registerStringProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })
  })

  describe('protocol.unregisterProtocol', function () {
    it('returns error when scheme does not exist', function (done) {
      protocol.unregisterProtocol('not-exist', function (error) {
        assert.notEqual(error, null)
        done()
      })
    })
  })

  describe('protocol.registerStringProtocol', function () {
    it('sends string as response', function (done) {
      var handler = function (request, callback) {
        callback(text)
      }
      protocol.registerStringProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sets Access-Control-Allow-Origin', function (done) {
      var handler = function (request, callback) {
        callback(text)
      }
      protocol.registerStringProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data, status, request) {
            assert.equal(data, text)
            assert.equal(request.getResponseHeader('Access-Control-Allow-Origin'), '*')
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sends object as response', function (done) {
      var handler = function (request, callback) {
        callback({
          data: text,
          mimeType: 'text/html'
        })
      }
      protocol.registerStringProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('fails when sending object other than string', function (done) {
      var handler = function (request, callback) {
        callback(new Date())
      }
      protocol.registerBufferProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })
  })

  describe('protocol.registerBufferProtocol', function () {
    var buffer = new Buffer(text)

    it('sends Buffer as response', function (done) {
      var handler = function (request, callback) {
        callback(buffer)
      }
      protocol.registerBufferProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sets Access-Control-Allow-Origin', function (done) {
      var handler = function (request, callback) {
        callback(buffer)
      }

      protocol.registerBufferProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data, status, request) {
            assert.equal(data, text)
            assert.equal(request.getResponseHeader('Access-Control-Allow-Origin'), '*')
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sends object as response', function (done) {
      var handler = function (request, callback) {
        callback({
          data: buffer,
          mimeType: 'text/html'
        })
      }
      protocol.registerBufferProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('fails when sending string', function (done) {
      var handler = function (request, callback) {
        callback(text)
      }
      protocol.registerBufferProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })
  })

  describe('protocol.registerFileProtocol', function () {
    var filePath = path.join(__dirname, 'fixtures', 'asar', 'a.asar', 'file1')
    var fileContent = require('fs').readFileSync(filePath)
    var normalPath = path.join(__dirname, 'fixtures', 'pages', 'a.html')
    var normalContent = require('fs').readFileSync(normalPath)

    it('sends file path as response', function (done) {
      var handler = function (request, callback) {
        callback(filePath)
      }
      protocol.registerFileProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, String(fileContent))
            return done()
          },
          error: function (xhr, errorType, error) {
            return done(error)
          }
        })
      })
    })

    it('sets Access-Control-Allow-Origin', function (done) {
      var handler = function (request, callback) {
        callback(filePath)
      }
      protocol.registerFileProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data, status, request) {
            assert.equal(data, String(fileContent))
            assert.equal(request.getResponseHeader('Access-Control-Allow-Origin'), '*')
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sends object as response', function (done) {
      var handler = function (request, callback) {
        callback({
          path: filePath
        })
      }
      protocol.registerFileProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, String(fileContent))
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('can send normal file', function (done) {
      var handler = function (request, callback) {
        callback(normalPath)
      }

      protocol.registerFileProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function (data) {
            assert.equal(data, String(normalContent))
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('fails when sending unexist-file', function (done) {
      var fakeFilePath = path.join(__dirname, 'fixtures', 'asar', 'a.asar', 'not-exist')
      var handler = function (request, callback) {
        callback(fakeFilePath)
      }
      protocol.registerFileProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })

    it('fails when sending unsupported content', function (done) {
      var handler = function (request, callback) {
        callback(new Date())
      }
      protocol.registerFileProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })
  })

  describe('protocol.registerHttpProtocol', function () {
    it('sends url as response', function (done) {
      var server = http.createServer(function (req, res) {
        assert.notEqual(req.headers.accept, '')
        res.end(text)
        server.close()
      })
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        var url = 'http://127.0.0.1:' + port
        var handler = function (request, callback) {
          callback({
            url: url
          })
        }
        protocol.registerHttpProtocol(protocolName, handler, function (error) {
          if (error) {
            return done(error)
          }
          $.ajax({
            url: protocolName + '://fake-host',
            success: function (data) {
              assert.equal(data, text)
              done()
            },
            error: function (xhr, errorType, error) {
              done(error)
            }
          })
        })
      })
    })

    it('fails when sending invalid url', function (done) {
      var handler = function (request, callback) {
        callback({
          url: 'url'
        })
      }
      protocol.registerHttpProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })

    it('fails when sending unsupported content', function (done) {
      var handler = function (request, callback) {
        callback(new Date())
      }
      protocol.registerHttpProtocol(protocolName, handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: protocolName + '://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })

    it('works when target URL redirects', function (done) {
      var contents = null
      var server = http.createServer(function (req, res) {
        if (req.url === '/serverRedirect') {
          res.statusCode = 301
          res.setHeader('Location', 'http://' + req.rawHeaders[1])
          res.end()
        } else {
          res.end(text)
        }
      })
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        var url = `${protocolName}://fake-host`
        var redirectURL = `http://127.0.0.1:${port}/serverRedirect`
        var handler = function (request, callback) {
          callback({
            url: redirectURL
          })
        }
        protocol.registerHttpProtocol(protocolName, handler, function (error) {
          if (error) {
            return done(error)
          }
          contents = webContents.create({})
          contents.on('did-finish-load', function () {
            assert.equal(contents.getURL(), url)
            server.close()
            contents.destroy()
            done()
          })
          contents.loadURL(url)
        })
      })
    })
  })

  describe('protocol.isProtocolHandled', function () {
    it('returns true for file:', function (done) {
      protocol.isProtocolHandled('file', function (result) {
        assert.equal(result, true)
        done()
      })
    })

    it('returns true for http:', function (done) {
      protocol.isProtocolHandled('http', function (result) {
        assert.equal(result, true)
        done()
      })
    })

    it('returns true for https:', function (done) {
      protocol.isProtocolHandled('https', function (result) {
        assert.equal(result, true)
        done()
      })
    })

    it('returns false when scheme is not registred', function (done) {
      protocol.isProtocolHandled('no-exist', function (result) {
        assert.equal(result, false)
        done()
      })
    })

    it('returns true for custom protocol', function (done) {
      var emptyHandler = function (request, callback) {
        callback()
      }
      protocol.registerStringProtocol(protocolName, emptyHandler, function (error) {
        assert.equal(error, null)
        protocol.isProtocolHandled(protocolName, function (result) {
          assert.equal(result, true)
          done()
        })
      })
    })

    it('returns true for intercepted protocol', function (done) {
      var emptyHandler = function (request, callback) {
        callback()
      }
      protocol.interceptStringProtocol('http', emptyHandler, function (error) {
        assert.equal(error, null)
        protocol.isProtocolHandled('http', function (result) {
          assert.equal(result, true)
          done()
        })
      })
    })
  })

  describe('protocol.intercept(Any)Protocol', function () {
    var emptyHandler = function (request, callback) {
      callback()
    }

    it('throws error when scheme is already intercepted', function (done) {
      protocol.interceptStringProtocol('http', emptyHandler, function (error) {
        assert.equal(error, null)
        protocol.interceptBufferProtocol('http', emptyHandler, function (error) {
          assert.notEqual(error, null)
          done()
        })
      })
    })

    it('does not crash when handler is called twice', function (done) {
      var doubleHandler = function (request, callback) {
        try {
          callback(text)
          callback()
        } catch (error) {
          // Ignore error
        }
      }
      protocol.interceptStringProtocol('http', doubleHandler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('sends error when callback is called with nothing', function (done) {
      if (process.env.TRAVIS === 'true') {
        return done()
      }
      protocol.interceptBufferProtocol('http', emptyHandler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          success: function () {
            done('request succeeded but it should not')
          },
          error: function (xhr, errorType) {
            assert.equal(errorType, 'error')
            done()
          }
        })
      })
    })
  })

  describe('protocol.interceptStringProtocol', function () {
    it('can intercept http protocol', function (done) {
      var handler = function (request, callback) {
        callback(text)
      }
      protocol.interceptStringProtocol('http', handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('can set content-type', function (done) {
      var handler = function (request, callback) {
        callback({
          mimeType: 'application/json',
          data: '{"value": 1}'
        })
      }
      protocol.interceptStringProtocol('http', handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          success: function (data) {
            assert.equal(typeof data, 'object')
            assert.equal(data.value, 1)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('can receive post data', function (done) {
      var handler = function (request, callback) {
        var uploadData = request.uploadData[0].bytes.toString()
        callback({
          data: uploadData
        })
      }
      protocol.interceptStringProtocol('http', handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          type: 'POST',
          data: postData,
          success: function (data) {
            assert.deepEqual(qs.parse(data), postData)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })
  })

  describe('protocol.interceptBufferProtocol', function () {
    it('can intercept http protocol', function (done) {
      var handler = function (request, callback) {
        callback(new Buffer(text))
      }
      protocol.interceptBufferProtocol('http', handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          success: function (data) {
            assert.equal(data, text)
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })

    it('can receive post data', function (done) {
      var handler = function (request, callback) {
        var uploadData = request.uploadData[0].bytes
        callback(uploadData)
      }
      protocol.interceptBufferProtocol('http', handler, function (error) {
        if (error) {
          return done(error)
        }
        $.ajax({
          url: 'http://fake-host',
          type: 'POST',
          data: postData,
          success: function (data) {
            assert.equal(data, $.param(postData))
            done()
          },
          error: function (xhr, errorType, error) {
            done(error)
          }
        })
      })
    })
  })

  describe('protocol.interceptHttpProtocol', function () {
    it('can send POST request', function (done) {
      var server = http.createServer(function (req, res) {
        var body = ''
        req.on('data', function (chunk) {
          body += chunk
        })
        req.on('end', function () {
          res.end(body)
        })
        server.close()
      })
      server.listen(0, '127.0.0.1', function () {
        var port = server.address().port
        var url = 'http://127.0.0.1:' + port
        var handler = function (request, callback) {
          var data = {
            url: url,
            method: 'POST',
            uploadData: {
              contentType: 'application/x-www-form-urlencoded',
              data: request.uploadData[0].bytes.toString()
            },
            session: null
          }
          callback(data)
        }
        protocol.interceptHttpProtocol('http', handler, function (error) {
          if (error) {
            return done(error)
          }
          $.ajax({
            url: 'http://fake-host',
            type: 'POST',
            data: postData,
            success: function (data) {
              assert.deepEqual(qs.parse(data), postData)
              done()
            },
            error: function (xhr, errorType, error) {
              done(error)
            }
          })
        })
      })
    })
  })

  describe('protocol.uninterceptProtocol', function () {
    it('returns error when scheme does not exist', function (done) {
      protocol.uninterceptProtocol('not-exist', function (error) {
        assert.notEqual(error, null)
        done()
      })
    })

    it('returns error when scheme is not intercepted', function (done) {
      protocol.uninterceptProtocol('http', function (error) {
        assert.notEqual(error, null)
        done()
      })
    })
  })

  describe('protocol.registerStandardSchemes', function () {
    const standardScheme = remote.getGlobal('standardScheme')
    const origin = standardScheme + '://fake-host'
    const imageURL = origin + '/test.png'
    const filePath = path.join(__dirname, 'fixtures', 'pages', 'b.html')
    const fileContent = '<img src="/test.png" />'
    var w = null
    var success = null

    beforeEach(function () {
      w = new BrowserWindow({show: false})
      success = false
    })

    afterEach(function (done) {
      protocol.unregisterProtocol(standardScheme, function () {
        if (w != null) {
          w.destroy()
        }
        w = null
        done()
      })
    })

    it('resolves relative resources', function (done) {
      var handler = function (request, callback) {
        if (request.url === imageURL) {
          success = true
          callback()
        } else {
          callback(filePath)
        }
      }
      protocol.registerFileProtocol(standardScheme, handler, function (error) {
        if (error) {
          return done(error)
        }
        w.webContents.on('did-finish-load', function () {
          assert(success)
          done()
        })
        w.loadURL(origin)
      })
    })

    it('resolves absolute resources', function (done) {
      var handler = function (request, callback) {
        if (request.url === imageURL) {
          success = true
          callback()
        } else {
          callback({
            data: fileContent,
            mimeType: 'text/html'
          })
        }
      }
      protocol.registerStringProtocol(standardScheme, handler, function (error) {
        if (error) {
          return done(error)
        }
        w.webContents.on('did-finish-load', function () {
          assert(success)
          done()
        })
        w.loadURL(origin)
      })
    })

    it('can have fetch working in it', function (done) {
      const content = '<html><script>fetch("http://github.com")</script></html>'
      const handler = function (request, callback) {
        callback({data: content, mimeType: 'text/html'})
      }
      protocol.registerStringProtocol(standardScheme, handler, function (error) {
        if (error) return done(error)
        w.webContents.on('crashed', function () {
          done('WebContents crashed')
        })
        w.webContents.on('did-finish-load', function () {
          done()
        })
        w.loadURL(origin)
      })
    })
  })
})
