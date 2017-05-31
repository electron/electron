const assert = require('assert')
const {remote} = require('electron')
const {ipcRenderer} = require('electron')
const http = require('http')
const url = require('url')
const {net} = remote
const {session} = remote

function randomBuffer (size, start, end) {
  start = start || 0
  end = end || 255
  let range = 1 + end - start
  const buffer = Buffer.allocUnsafe(size)
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random() * range)
  }
  return buffer
}

function randomString (length) {
  let buffer = randomBuffer(length, '0'.charCodeAt(0), 'z'.charCodeAt(0))
  return buffer.toString()
}

const kOneKiloByte = 1024
const kOneMegaByte = kOneKiloByte * kOneKiloByte

describe('net module', function () {
  let server
  const connections = new Set()

  beforeEach(function (done) {
    server = http.createServer()
    server.listen(0, '127.0.0.1', function () {
      server.url = `http://127.0.0.1:${server.address().port}`
      done()
    })
    server.on('connection', (connection) => {
      connections.add(connection)
      connection.once('close', () => {
        connections.delete(connection)
      })
    })
  })

  afterEach(function (done) {
    for (const connection of connections) {
      connection.destroy()
    }
    server.close(function () {
      server = null
      done()
    })
  })

  describe('HTTP basics', function () {
    it('should be able to issue a basic GET request', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.method, 'GET')
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request(`${server.url}${requestUrl}`)
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should be able to issue a basic POST request', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.method, 'POST')
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should fetch correct data in a GET request', function (done) {
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.method, 'GET')
            response.write(bodyData)
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request(`${server.url}${requestUrl}`)
      urlRequest.on('response', function (response) {
        let expectedBodyData = ''
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
          expectedBodyData += chunk.toString()
        })
        response.on('end', function () {
          assert.equal(expectedBodyData, bodyData)
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should post the correct data in a POST request', function (done) {
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      server.on('request', function (request, response) {
        let postedBodyData = ''
        switch (request.url) {
          case requestUrl:
            assert.equal(request.method, 'POST')
            request.on('data', function (chunk) {
              postedBodyData += chunk.toString()
            })
            request.on('end', function () {
              assert.equal(postedBodyData, bodyData)
              response.end()
            })
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.write(bodyData)
      urlRequest.end()
    })

    it('should support chunked encoding', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.chunkedEncoding = true
            assert.equal(request.method, 'POST')
            assert.equal(request.headers['transfer-encoding'], 'chunked')
            assert(!request.headers['content-length'])
            request.on('data', function (chunk) {
              response.write(chunk)
            })
            request.on('end', function (chunk) {
              response.end(chunk)
            })
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${requestUrl}`
      })

      let chunkIndex = 0
      let chunkCount = 100
      let sentChunks = []
      let receivedChunks = []
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
          receivedChunks.push(chunk)
        })
        response.on('end', function () {
          let sentData = Buffer.concat(sentChunks)
          let receivedData = Buffer.concat(receivedChunks)
          assert.equal(sentData.toString(), receivedData.toString())
          assert.equal(chunkIndex, chunkCount)
          done()
        })
        response.resume()
      })
      urlRequest.chunkedEncoding = true
      while (chunkIndex < chunkCount) {
        ++chunkIndex
        let chunk = randomBuffer(kOneKiloByte)
        sentChunks.push(chunk)
        assert(urlRequest.write(chunk))
      }
      urlRequest.end()
    })
  })

  describe('ClientRequest API', function () {
    afterEach(function () {
      session.defaultSession.webRequest.onBeforeRequest(null)
    })

    it('request/response objects should emit expected events', function (done) {
      const requestUrl = '/requestUrl'
      let bodyData = randomString(kOneMegaByte)
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(bodyData)
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let requestResponseEventEmitted = false
      let requestFinishEventEmitted = false
      let requestCloseEventEmitted = false
      let responseDataEventEmitted = false
      let responseEndEventEmitted = false

      function maybeDone (done) {
        if (!requestCloseEventEmitted || !responseEndEventEmitted) {
          return
        }

        assert(requestResponseEventEmitted)
        assert(requestFinishEventEmitted)
        assert(requestCloseEventEmitted)
        assert(responseDataEventEmitted)
        assert(responseEndEventEmitted)
        done()
      }

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        requestResponseEventEmitted = true
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        let buffers = []
        response.pause()
        response.on('data', function (chunk) {
          buffers.push(chunk)
          responseDataEventEmitted = true
        })
        response.on('end', function () {
          let receivedBodyData = Buffer.concat(buffers)
          assert(receivedBodyData.toString() === bodyData)
          responseEndEventEmitted = true
          maybeDone(done)
        })
        response.resume()
        response.on('error', function (error) {
          assert.ifError(error)
        })
        response.on('aborted', function () {
          assert.fail('response aborted')
        })
      })
      urlRequest.on('finish', function () {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', function (error) {
        assert.ifError(error)
      })
      urlRequest.on('abort', function () {
        assert.fail('request aborted')
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        maybeDone(done)
      })
      urlRequest.end()
    })

    it('should be able to set a custom HTTP request header before first write', function (done) {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.write('')
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.end()
    })

    it('should be able to set a non-string object as a header value', function (done) {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Integer-Value'
      const customHeaderValue = 900
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue.toString())
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.equal(request.url, requestUrl)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.write('')
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.end()
    })

    it('should not be able to set a custom HTTP request header after first write', function (done) {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert(!request.headers[customHeaderName.toLowerCase()])
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.write('')
      assert.throws(() => {
        urlRequest.setHeader(customHeaderName, customHeaderValue)
      })
      assert(!urlRequest.getHeader(customHeaderName))
      urlRequest.end()
    })

    it('should be able to remove a custom HTTP request header before first write', function (done) {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert(!request.headers[customHeaderName.toLowerCase()])
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      urlRequest.removeHeader(customHeaderName)
      assert(!urlRequest.getHeader(customHeaderName))
      urlRequest.write('')
      urlRequest.end()
    })

    it('should not be able to remove a custom HTTP request header after first write', function (done) {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      urlRequest.write('')
      assert.throws(function () {
        urlRequest.removeHeader(customHeaderName)
      })
      assert.equal(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      urlRequest.end()
    })

    it('should be able to set cookie header line', function (done) {
      const requestUrl = '/requestUrl'
      const cookieHeaderName = 'Cookie'
      const cookieHeaderValue = 'test=12345'
      const customSession = session.fromPartition('test-cookie-header')
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.headers[cookieHeaderName.toLowerCase()],
              cookieHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      customSession.cookies.set({
        url: `${server.url}`,
        name: 'test',
        value: '11111'
      }, function (error) {
        if (error) {
          return done(error)
        }
        const urlRequest = net.request({
          method: 'GET',
          url: `${server.url}${requestUrl}`,
          session: customSession
        })
        urlRequest.on('response', function (response) {
          const statusCode = response.statusCode
          assert.equal(statusCode, 200)
          response.pause()
          response.on('data', function (chunk) {
          })
          response.on('end', function () {
            done()
          })
          response.resume()
        })
        urlRequest.setHeader(cookieHeaderName, cookieHeaderValue)
        assert.equal(urlRequest.getHeader(cookieHeaderName),
          cookieHeaderValue)
        urlRequest.end()
      })
    })

    it('should be able to abort an HTTP request before first write', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        assert.fail('Unexpected request event')
      })

      let requestAbortEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', function () {
        assert.fail('Unexpected finish event')
      })
      urlRequest.on('error', function () {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', function () {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        assert(requestAbortEventEmitted)
        assert(requestCloseEventEmitted)
        done()
      })
      urlRequest.abort()
      assert(!urlRequest.write(''))
      urlRequest.end()
    })

    it('it should be able to abort an HTTP request before request end', function (done) {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            cancelRequest()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let requestAbortEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', function () {
        assert.fail('Unexpected finish event')
      })
      urlRequest.on('error', function () {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', function () {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        assert(requestReceivedByServer)
        assert(requestAbortEventEmitted)
        assert(requestCloseEventEmitted)
        done()
      })

      urlRequest.chunkedEncoding = true
      urlRequest.write(randomString(kOneKiloByte))
      function cancelRequest () {
        urlRequest.abort()
      }
    })

    it('it should be able to abort an HTTP request after request end and before response', function (done) {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            cancelRequest()
            process.nextTick(() => {
              response.statusCode = 200
              response.statusMessage = 'OK'
              response.end()
            })
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let requestAbortEventEmitted = false
      let requestFinishEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', function () {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', function () {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', function () {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        assert(requestFinishEventEmitted)
        assert(requestReceivedByServer)
        assert(requestAbortEventEmitted)
        assert(requestCloseEventEmitted)
        done()
      })

      urlRequest.end(randomString(kOneKiloByte))
      function cancelRequest () {
        urlRequest.abort()
      }
    })

    it('it should be able to abort an HTTP request after response start', function (done) {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(randomString(kOneKiloByte))
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let requestFinishEventEmitted = false
      let requestResponseEventEmitted = false
      let requestAbortEventEmitted = false
      let requestCloseEventEmitted = false
      let responseAbortedEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        requestResponseEventEmitted = true
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          assert.fail('Unexpected end event')
        })
        response.resume()
        response.on('error', function () {
          assert.fail('Unexpected error event')
        })
        response.on('aborted', function () {
          responseAbortedEventEmitted = true
        })
        urlRequest.abort()
      })
      urlRequest.on('finish', function () {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', function () {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', function () {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        assert(requestFinishEventEmitted, 'request should emit "finish" event')
        assert(requestReceivedByServer, 'request should be received by the server')
        assert(requestResponseEventEmitted, '"response" event should be emitted')
        assert(requestAbortEventEmitted, 'request should emit "abort" event')
        assert(responseAbortedEventEmitted, 'response should emit "aborted" event')
        assert(requestCloseEventEmitted, 'request should emit "close" event')
        done()
      })
      urlRequest.end(randomString(kOneKiloByte))
    })

    it('abort event should be emitted at most once', function (done) {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            cancelRequest()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let requestFinishEventEmitted = false
      let requestAbortEventCount = 0
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function () {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', function () {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', function () {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', function () {
        ++requestAbortEventCount
        urlRequest.abort()
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        // Let all pending async events to be emitted
        setTimeout(function () {
          assert(requestFinishEventEmitted)
          assert(requestReceivedByServer)
          assert.equal(requestAbortEventCount, 1)
          assert(requestCloseEventEmitted)
          done()
        }, 500)
      })

      urlRequest.end(randomString(kOneKiloByte))
      function cancelRequest () {
        urlRequest.abort()
        urlRequest.abort()
      }
    })

    it('Requests should be intercepted by webRequest module', function (done) {
      const requestUrl = '/requestUrl'
      const redirectUrl = '/redirectUrl'
      let requestIsRedirected = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.fail(`Unexpected url: ${request.url}`)
            break
          case redirectUrl:
            requestIsRedirected = true
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let requestIsIntercepted = false
      session.defaultSession.webRequest.onBeforeRequest(
        function (details, callback) {
          if (details.url === `${server.url}${requestUrl}`) {
            requestIsIntercepted = true
            callback({
              redirectURL: `${server.url}${redirectUrl}`
            })
          } else {
            callback({
              cancel: false
            })
          }
        })

      const urlRequest = net.request(`${server.url}${requestUrl}`)

      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          assert(requestIsRedirected, 'The server should receive a request to the forward URL')
          assert(requestIsIntercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should to able to create and intercept a request using a custom session object', function (done) {
      const requestUrl = '/requestUrl'
      const redirectUrl = '/redirectUrl'
      const customPartitionName = 'custom-partition'
      let requestIsRedirected = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.fail(`Unexpected url: ${request.url}`)
            break
          case redirectUrl:
            requestIsRedirected = true
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      session.defaultSession.webRequest.onBeforeRequest(
        function (details, callback) {
          assert.fail('Request should not be intercepted by the default session')
        })

      let customSession = session.fromPartition(customPartitionName, {
        cache: false
      })
      let requestIsIntercepted = false
      customSession.webRequest.onBeforeRequest(
        function (details, callback) {
          if (details.url === `${server.url}${requestUrl}`) {
            requestIsIntercepted = true
            callback({
              redirectURL: `${server.url}${redirectUrl}`
            })
          } else {
            callback({
              cancel: false
            })
          }
        })

      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        session: customSession
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          assert(requestIsRedirected, 'The server should receive a request to the forward URL')
          assert(requestIsIntercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should throw if given an invalid redirect mode', function () {
      const requestUrl = '/requestUrl'
      const options = {
        url: `${server.url}${requestUrl}`,
        redirect: 'custom'
      }
      assert.throws(function () {
        net.request(options)
      }, 'redirect mode should be one of follow, error or manual')
    })

    it('should throw when calling getHeader without a name', function () {
      assert.throws(function () {
        net.request({url: `${server.url}/requestUrl`}).getHeader()
      }, /`name` is required for getHeader\(name\)\./)

      assert.throws(function () {
        net.request({url: `${server.url}/requestUrl`}).getHeader(null)
      }, /`name` is required for getHeader\(name\)\./)
    })

    it('should throw when calling removeHeader without a name', function () {
      assert.throws(function () {
        net.request({url: `${server.url}/requestUrl`}).removeHeader()
      }, /`name` is required for removeHeader\(name\)\./)

      assert.throws(function () {
        net.request({url: `${server.url}/requestUrl`}).removeHeader(null)
      }, /`name` is required for removeHeader\(name\)\./)
    })

    it('should follow redirect when no redirect mode is provided', function (done) {
      const requestUrl = '/301'
      server.on('request', function (request, response) {
        switch (request.url) {
          case '/301':
            response.statusCode = '301'
            response.setHeader('Location', '/200')
            response.end()
            break
          case '/200':
            response.statusCode = '200'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        done()
      })
      urlRequest.end()
    })

    it('should follow redirect chain when no redirect mode is provided', function (done) {
      const requestUrl = '/redirectChain'
      server.on('request', function (request, response) {
        switch (request.url) {
          case '/redirectChain':
            response.statusCode = '301'
            response.setHeader('Location', '/301')
            response.end()
            break
          case '/301':
            response.statusCode = '301'
            response.setHeader('Location', '/200')
            response.end()
            break
          case '/200':
            response.statusCode = '200'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        done()
      })
      urlRequest.end()
    })

    it('should not follow redirect when mode is error', function (done) {
      const requestUrl = '/301'
      server.on('request', function (request, response) {
        switch (request.url) {
          case '/301':
            response.statusCode = '301'
            response.setHeader('Location', '/200')
            response.end()
            break
          case '/200':
            response.statusCode = '200'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        redirect: 'error'
      })
      urlRequest.on('error', function (error) {
        assert.equal(error.message, 'Request cannot follow redirect with the current redirect mode')
      })
      urlRequest.on('close', function () {
        done()
      })
      urlRequest.end()
    })

    it('should allow follow redirect when mode is manual', function (done) {
      const requestUrl = '/redirectChain'
      let redirectCount = 0
      server.on('request', function (request, response) {
        switch (request.url) {
          case '/redirectChain':
            response.statusCode = '301'
            response.setHeader('Location', '/301')
            response.end()
            break
          case '/301':
            response.statusCode = '301'
            response.setHeader('Location', '/200')
            response.end()
            break
          case '/200':
            response.statusCode = '200'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        redirect: 'manual'
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        assert.equal(redirectCount, 2)
        done()
      })
      urlRequest.on('redirect', function (status, method, url) {
        if (url === `${server.url}/301` || url === `${server.url}/200`) {
          redirectCount += 1
          urlRequest.followRedirect()
        }
      })
      urlRequest.end()
    })

    it('should allow cancelling redirect when mode is manual', function (done) {
      const requestUrl = '/redirectChain'
      let redirectCount = 0
      server.on('request', function (request, response) {
        switch (request.url) {
          case '/redirectChain':
            response.statusCode = '301'
            response.setHeader('Location', '/redirect/1')
            response.end()
            break
          case '/redirect/1':
            response.statusCode = '200'
            response.setHeader('Location', '/redirect/2')
            response.end()
            break
          case '/redirect/2':
            response.statusCode = '200'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        redirect: 'manual'
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          urlRequest.abort()
        })
        response.resume()
      })
      urlRequest.on('close', function () {
        assert.equal(redirectCount, 1)
        done()
      })
      urlRequest.on('redirect', function (status, method, url) {
        if (url === `${server.url}/redirect/1`) {
          redirectCount += 1
          urlRequest.followRedirect()
        }
      })
      urlRequest.end()
    })

    it('should throw if given an invalid session option', function (done) {
      const requestUrl = '/requestUrl'
      try {
        const urlRequest = net.request({
          url: `${server.url}${requestUrl}`,
          session: 1
        })
        urlRequest
      } catch (exception) {
        done()
      }
    })

    it('should to able to create and intercept a request using a custom partition name', function (done) {
      const requestUrl = '/requestUrl'
      const redirectUrl = '/redirectUrl'
      const customPartitionName = 'custom-partition'
      let requestIsRedirected = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.fail(`Unexpected url: ${request.url}`)
            break
          case redirectUrl:
            requestIsRedirected = true
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      session.defaultSession.webRequest.onBeforeRequest(
        function (details, callback) {
          assert.fail('Request should not be intercepted by the default session')
        })

      let customSession = session.fromPartition(customPartitionName, {
        cache: false
      })
      let requestIsIntercepted = false
      customSession.webRequest.onBeforeRequest(
        function (details, callback) {
          if (details.url === `${server.url}${requestUrl}`) {
            requestIsIntercepted = true
            callback({
              redirectURL: `${server.url}${redirectUrl}`
            })
          } else {
            callback({
              cancel: false
            })
          }
        })

      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        partition: customPartitionName
      })
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          assert(requestIsRedirected, 'The server should receive a request to the forward URL')
          assert(requestIsIntercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should throw if given an invalid partition option', function (done) {
      const requestUrl = '/requestUrl'
      try {
        const urlRequest = net.request({
          url: `${server.url}${requestUrl}`,
          partition: 1
        })
        urlRequest
      } catch (exception) {
        done()
      }
    })

    it('should be able to create a request with options', function (done) {
      const requestUrl = '/'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            assert.equal(request.method, 'GET')
            assert.equal(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      const serverUrl = url.parse(server.url)
      let options = {
        port: serverUrl.port,
        hostname: '127.0.0.1',
        headers: {}
      }
      options.headers[customHeaderName] = customHeaderValue
      const urlRequest = net.request(options)
      urlRequest.on('response', function (response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should be able to pipe a readable stream into a net request', function (done) {
      const nodeRequestUrl = '/nodeRequestUrl'
      const netRequestUrl = '/netRequestUrl'
      const bodyData = randomString(kOneMegaByte)
      let netRequestReceived = false
      let netRequestEnded = false
      server.on('request', function (request, response) {
        switch (request.url) {
          case nodeRequestUrl:
            response.write(bodyData)
            response.end()
            break
          case netRequestUrl:
            netRequestReceived = true
            let receivedBodyData = ''
            request.on('data', function (chunk) {
              receivedBodyData += chunk.toString()
            })
            request.on('end', function (chunk) {
              netRequestEnded = true
              if (chunk) {
                receivedBodyData += chunk.toString()
              }
              assert.equal(receivedBodyData, bodyData)
              response.end()
            })
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })

      let nodeRequest = http.request(`${server.url}${nodeRequestUrl}`)
      nodeRequest.on('response', function (nodeResponse) {
        const netRequest = net.request(`${server.url}${netRequestUrl}`)
        netRequest.on('response', function (netResponse) {
          assert.equal(netResponse.statusCode, 200)
          netResponse.pause()
          netResponse.on('data', function (chunk) {
          })
          netResponse.on('end', function () {
            assert(netRequestReceived)
            assert(netRequestEnded)
            done()
          })
          netResponse.resume()
        })
        nodeResponse.pipe(netRequest)
      })
      nodeRequest.end()
    })

    it('should emit error event on server socket close', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            request.socket.destroy()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      let requestErrorEventEmitted = false
      const urlRequest = net.request(`${server.url}${requestUrl}`)
      urlRequest.on('error', function (error) {
        assert(error)
        requestErrorEventEmitted = true
      })
      urlRequest.on('close', function () {
        assert(requestErrorEventEmitted)
        done()
      })
      urlRequest.end()
    })
  })

  describe('IncomingMessage API', function () {
    it('response object should implement the IncomingMessage API', function (done) {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.setHeader(customHeaderName, customHeaderValue)
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        const statusCode = response.statusCode
        assert(typeof statusCode === 'number')
        assert.equal(statusCode, 200)
        const statusMessage = response.statusMessage
        assert(typeof statusMessage === 'string')
        assert.equal(statusMessage, 'OK')
        const headers = response.headers
        assert(typeof headers === 'object')
        assert.deepEqual(headers[customHeaderName.toLowerCase()],
          [customHeaderValue])
        const httpVersion = response.httpVersion
        assert(typeof httpVersion === 'string')
        assert(httpVersion.length > 0)
        const httpVersionMajor = response.httpVersionMajor
        assert(typeof httpVersionMajor === 'number')
        assert(httpVersionMajor >= 1)
        const httpVersionMinor = response.httpVersionMinor
        assert(typeof httpVersionMinor === 'number')
        assert(httpVersionMinor >= 0)
        response.pause()
        response.on('data', function (chunk) {
        })
        response.on('end', function () {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should be able to pipe a net response into a writable stream', function (done) {
      const nodeRequestUrl = '/nodeRequestUrl'
      const netRequestUrl = '/netRequestUrl'
      const bodyData = randomString(kOneMegaByte)
      server.on('request', function (request, response) {
        switch (request.url) {
          case netRequestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(bodyData)
            response.end()
            break
          case nodeRequestUrl:
            let receivedBodyData = ''
            request.on('data', function (chunk) {
              receivedBodyData += chunk.toString()
            })
            request.on('end', function (chunk) {
              if (chunk) {
                receivedBodyData += chunk.toString()
              }
              assert.equal(receivedBodyData, bodyData)
              response.end()
            })
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      ipcRenderer.once('api-net-spec-done', function () {
        done()
      })
      // Execute below code directly within the browser context without
      // using the remote module.
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const http = require('http')
        const netRequest = net.request('${server.url}${netRequestUrl}')
        netRequest.on('response', function (netResponse) {
          const serverUrl = url.parse('${server.url}')
          const nodeOptions = {
            method: 'POST',
            path: '${nodeRequestUrl}',
            port: serverUrl.port
          }
          let nodeRequest = http.request(nodeOptions)
          nodeRequest.on('response', function (nodeResponse) {
            nodeResponse.on('data', function (chunk) {
            })
            nodeResponse.on('end', function (chunk) {
              event.sender.send('api-net-spec-done')
            })
          })
          netResponse.pipe(nodeRequest)
        })
        netRequest.end()
      `)
    })

    it('should not emit any event after close', function (done) {
      const requestUrl = '/requestUrl'
      let bodyData = randomString(kOneKiloByte)
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(bodyData)
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      let requestCloseEventEmitted = false
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', function (response) {
        assert(!requestCloseEventEmitted)
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function () {
        })
        response.on('end', function () {
        })
        response.resume()
        response.on('error', function () {
          assert(!requestCloseEventEmitted)
        })
        response.on('aborted', function () {
          assert(!requestCloseEventEmitted)
        })
      })
      urlRequest.on('finish', function () {
        assert(!requestCloseEventEmitted)
      })
      urlRequest.on('error', function () {
        assert(!requestCloseEventEmitted)
      })
      urlRequest.on('abort', function () {
        assert(!requestCloseEventEmitted)
      })
      urlRequest.on('close', function () {
        requestCloseEventEmitted = true
        // Wait so that all async events get scheduled.
        setTimeout(function () {
          done()
        }, 100)
      })
      urlRequest.end()
    })
  })

  describe('Stability and performance', function (done) {
    it('should free unreferenced, never-started request objects without crash', function (done) {
      const requestUrl = '/requestUrl'
      ipcRenderer.once('api-net-spec-done', function () {
        done()
      })
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const urlRequest = net.request('${server.url}${requestUrl}')
        process.nextTick(function () {
          const v8Util = process.atomBinding('v8_util')
          v8Util.requestGarbageCollectionForTesting()
          event.sender.send('api-net-spec-done')
        })
      `)
    })

    it('should not collect on-going requests without crash', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(randomString(kOneKiloByte))
            ipcRenderer.once('api-net-spec-resume', function () {
              response.write(randomString(kOneKiloByte))
              response.end()
            })
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      ipcRenderer.once('api-net-spec-done', function () {
        done()
      })
      // Execute below code directly within the browser context without
      // using the remote module.
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const urlRequest = net.request('${server.url}${requestUrl}')
        urlRequest.on('response', function (response) {
          response.on('data', function () {
          })
          response.on('end', function () {
            event.sender.send('api-net-spec-done')
          })
          process.nextTick(function () {
            // Trigger a garbage collection.
            const v8Util = process.atomBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
            event.sender.send('api-net-spec-resume')
          })
        })
        urlRequest.end()
      `)
    })

    it('should collect unreferenced, ended requests without crash', function (done) {
      const requestUrl = '/requestUrl'
      server.on('request', function (request, response) {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.fail(`Unexpected url: ${request.url}`)
        }
      })
      ipcRenderer.once('api-net-spec-done', function () {
        done()
      })
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const urlRequest = net.request('${server.url}${requestUrl}')
        urlRequest.on('response', function (response) {
          response.on('data', function () {
          })
          response.on('end', function () {
          })
        })
        urlRequest.on('close', function () {
          process.nextTick(function () {
            const v8Util = process.atomBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
            event.sender.send('api-net-spec-done')
          })
        })
        urlRequest.end()
      `)
    })
  })
})
