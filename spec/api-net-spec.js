const assert = require('assert')
const { remote } = require('electron')
const { ipcRenderer } = require('electron')
const http = require('http')
const url = require('url')
const { net } = remote
const { session } = remote

/* The whole net API doesn't use standard callbacks */
/* eslint-disable standard/no-callback-literal */

function randomBuffer (size, start, end) {
  start = start || 0
  end = end || 255
  const range = 1 + end - start
  const buffer = Buffer.allocUnsafe(size)
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random() * range)
  }
  return buffer
}

function randomString (length) {
  const buffer = randomBuffer(length, '0'.charCodeAt(0), 'z'.charCodeAt(0))
  return buffer.toString()
}

const kOneKiloByte = 1024
const kOneMegaByte = kOneKiloByte * kOneKiloByte

describe('net module', () => {
  let server
  const connections = new Set()

  beforeEach((done) => {
    server = http.createServer()
    server.listen(0, '127.0.0.1', () => {
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

  afterEach((done) => {
    for (const connection of connections) {
      connection.destroy()
    }
    server.close(() => {
      server = null
      done()
    })
  })

  describe('HTTP basics', () => {
    it('should be able to issue a basic GET request', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.method, 'GET')
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request(`${server.url}${requestUrl}`)
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should be able to issue a basic POST request', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.method, 'POST')
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should fetch correct data in a GET request', (done) => {
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.method, 'GET')
            response.write(bodyData)
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request(`${server.url}${requestUrl}`)
      urlRequest.on('response', (response) => {
        let expectedBodyData = ''
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
          expectedBodyData += chunk.toString()
        })
        response.on('end', () => {
          assert.strictEqual(expectedBodyData, bodyData)
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should post the correct data in a POST request', (done) => {
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      server.on('request', (request, response) => {
        let postedBodyData = ''
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.method, 'POST')
            request.on('data', (chunk) => {
              postedBodyData += chunk.toString()
            })
            request.on('end', () => {
              assert.strictEqual(postedBodyData, bodyData)
              response.end()
            })
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {})
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.write(bodyData)
      urlRequest.end()
    })

    it('should support chunked encoding', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.chunkedEncoding = true
            assert.strictEqual(request.method, 'POST')
            assert.strictEqual(request.headers['transfer-encoding'], 'chunked')
            assert(!request.headers['content-length'])
            request.on('data', (chunk) => {
              response.write(chunk)
            })
            request.on('end', (chunk) => {
              response.end(chunk)
            })
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${requestUrl}`
      })

      let chunkIndex = 0
      const chunkCount = 100
      const sentChunks = []
      const receivedChunks = []
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
          receivedChunks.push(chunk)
        })
        response.on('end', () => {
          const sentData = Buffer.concat(sentChunks)
          const receivedData = Buffer.concat(receivedChunks)
          assert.strictEqual(sentData.toString(), receivedData.toString())
          assert.strictEqual(chunkIndex, chunkCount)
          done()
        })
        response.resume()
      })
      urlRequest.chunkedEncoding = true
      while (chunkIndex < chunkCount) {
        chunkIndex += 1
        const chunk = randomBuffer(kOneKiloByte)
        sentChunks.push(chunk)
        assert(urlRequest.write(chunk))
      }
      urlRequest.end()
    })
  })

  describe('ClientRequest API', () => {
    afterEach(() => {
      session.defaultSession.webRequest.onBeforeRequest(null)
    })

    it('request/response objects should emit expected events', (done) => {
      const requestUrl = '/requestUrl'
      const bodyData = randomString(kOneMegaByte)
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(bodyData)
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
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
      urlRequest.on('response', (response) => {
        requestResponseEventEmitted = true
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        const buffers = []
        response.pause()
        response.on('data', (chunk) => {
          buffers.push(chunk)
          responseDataEventEmitted = true
        })
        response.on('end', () => {
          const receivedBodyData = Buffer.concat(buffers)
          assert(receivedBodyData.toString() === bodyData)
          responseEndEventEmitted = true
          maybeDone(done)
        })
        response.resume()
        response.on('error', (error) => {
          assert.ifError(error)
        })
        response.on('aborted', () => {
          assert.fail('response aborted')
        })
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', (error) => {
        assert.ifError(error)
      })
      urlRequest.on('abort', () => {
        assert.fail('request aborted')
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        maybeDone(done)
      })
      urlRequest.end()
    })

    it('should be able to set a custom HTTP request header before first write', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.write('')
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.end()
    })

    it('should be able to set a non-string object as a header value', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Integer-Value'
      const customHeaderValue = 900
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue.toString())
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            assert.strictEqual(request.url, requestUrl)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.write('')
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName.toLowerCase()),
        customHeaderValue)
      urlRequest.end()
    })

    it('should not be able to set a custom HTTP request header after first write', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert(!request.headers[customHeaderName.toLowerCase()])
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
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

    it('should be able to remove a custom HTTP request header before first write', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert(!request.headers[customHeaderName.toLowerCase()])
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      urlRequest.removeHeader(customHeaderName)
      assert(!urlRequest.getHeader(customHeaderName))
      urlRequest.write('')
      urlRequest.end()
    })

    it('should not be able to remove a custom HTTP request header after first write', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      urlRequest.write('')
      assert.throws(() => {
        urlRequest.removeHeader(customHeaderName)
      })
      assert.strictEqual(urlRequest.getHeader(customHeaderName),
        customHeaderValue)
      urlRequest.end()
    })

    it('should be able to set cookie header line', (done) => {
      const requestUrl = '/requestUrl'
      const cookieHeaderName = 'Cookie'
      const cookieHeaderValue = 'test=12345'
      const customSession = session.fromPartition('test-cookie-header')
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.headers[cookieHeaderName.toLowerCase()],
              cookieHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      customSession.cookies.set({
        url: `${server.url}`,
        name: 'test',
        value: '11111'
      }).then(() => { // resolved
        const urlRequest = net.request({
          method: 'GET',
          url: `${server.url}${requestUrl}`,
          session: customSession
        })
        urlRequest.on('response', (response) => {
          const statusCode = response.statusCode
          assert.strictEqual(statusCode, 200)
          response.pause()
          response.on('data', (chunk) => {})
          response.on('end', () => {
            done()
          })
          response.resume()
        })
        urlRequest.setHeader(cookieHeaderName, cookieHeaderValue)
        assert.strictEqual(urlRequest.getHeader(cookieHeaderName),
          cookieHeaderValue)
        urlRequest.end()
      }, (error) => {
        done(error)
      })
    })

    it('should be able to abort an HTTP request before first write', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        response.end()
        assert.fail('Unexpected request event')
      })

      let requestAbortEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        assert.fail('Unexpected finish event')
      })
      urlRequest.on('error', () => {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        assert(requestAbortEventEmitted)
        assert(requestCloseEventEmitted)
        done()
      })
      urlRequest.abort()
      assert(!urlRequest.write(''))
      urlRequest.end()
    })

    it('it should be able to abort an HTTP request before request end', (done) => {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            cancelRequest()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      let requestAbortEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        assert.fail('Unexpected finish event')
      })
      urlRequest.on('error', () => {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
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

    it('it should be able to abort an HTTP request after request end and before response', (done) => {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', (request, response) => {
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
            handleUnexpectedURL(request, response)
        }
      })

      let requestAbortEventEmitted = false
      let requestFinishEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
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

    it('it should be able to abort an HTTP request after response start', (done) => {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(randomString(kOneKiloByte))
            break
          default:
            handleUnexpectedURL(request, response)
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
      urlRequest.on('response', (response) => {
        requestResponseEventEmitted = true
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          assert.fail('Unexpected end event')
        })
        response.resume()
        response.on('error', () => {
          assert.fail('Unexpected error event')
        })
        response.on('aborted', () => {
          responseAbortedEventEmitted = true
        })
        urlRequest.abort()
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
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

    it('abort event should be emitted at most once', (done) => {
      const requestUrl = '/requestUrl'
      let requestReceivedByServer = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            requestReceivedByServer = true
            cancelRequest()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      let requestFinishEventEmitted = false
      let requestAbortEventCount = 0
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', () => {
        assert.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        assert.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        ++requestAbortEventCount
        urlRequest.abort()
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        // Let all pending async events to be emitted
        setTimeout(() => {
          assert(requestFinishEventEmitted)
          assert(requestReceivedByServer)
          assert.strictEqual(requestAbortEventCount, 1)
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

    it('Requests should be intercepted by webRequest module', (done) => {
      const requestUrl = '/requestUrl'
      const redirectUrl = '/redirectUrl'
      let requestIsRedirected = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case redirectUrl:
            requestIsRedirected = true
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      let requestIsIntercepted = false
      session.defaultSession.webRequest.onBeforeRequest(
        (details, callback) => {
          if (details.url === `${server.url}${requestUrl}`) {
            requestIsIntercepted = true
            // Disabled due to false positive in StandardJS
            // eslint-disable-next-line standard/no-callback-literal
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

      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          assert(requestIsRedirected, 'The server should receive a request to the forward URL')
          assert(requestIsIntercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should to able to create and intercept a request using a custom session object', (done) => {
      const requestUrl = '/requestUrl'
      const redirectUrl = '/redirectUrl'
      const customPartitionName = 'custom-partition'
      let requestIsRedirected = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case redirectUrl:
            requestIsRedirected = true
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      session.defaultSession.webRequest.onBeforeRequest((details, callback) => {
        assert.fail('Request should not be intercepted by the default session')
      })

      const customSession = session.fromPartition(customPartitionName, { cache: false })
      let requestIsIntercepted = false
      customSession.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === `${server.url}${requestUrl}`) {
          requestIsIntercepted = true
          // Disabled due to false positive in StandardJS
          // eslint-disable-next-line standard/no-callback-literal
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
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          assert(requestIsRedirected, 'The server should receive a request to the forward URL')
          assert(requestIsIntercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should throw if given an invalid redirect mode', () => {
      const requestUrl = '/requestUrl'
      const options = {
        url: `${server.url}${requestUrl}`,
        redirect: 'custom'
      }
      assert.throws(() => {
        net.request(options)
      }, 'redirect mode should be one of follow, error or manual')
    })

    it('should throw when calling getHeader without a name', () => {
      assert.throws(() => {
        net.request({ url: `${server.url}/requestUrl` }).getHeader()
      }, /`name` is required for getHeader\(name\)\./)

      assert.throws(() => {
        net.request({ url: `${server.url}/requestUrl` }).getHeader(null)
      }, /`name` is required for getHeader\(name\)\./)
    })

    it('should throw when calling removeHeader without a name', () => {
      assert.throws(() => {
        net.request({ url: `${server.url}/requestUrl` }).removeHeader()
      }, /`name` is required for removeHeader\(name\)\./)

      assert.throws(() => {
        net.request({ url: `${server.url}/requestUrl` }).removeHeader(null)
      }, /`name` is required for removeHeader\(name\)\./)
    })

    it('should follow redirect when no redirect mode is provided', (done) => {
      const requestUrl = '/301'
      server.on('request', (request, response) => {
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
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        done()
      })
      urlRequest.end()
    })

    it('should follow redirect chain when no redirect mode is provided', (done) => {
      const requestUrl = '/redirectChain'
      server.on('request', (request, response) => {
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
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        done()
      })
      urlRequest.end()
    })

    it('should not follow redirect when mode is error', (done) => {
      const requestUrl = '/301'
      server.on('request', (request, response) => {
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
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        redirect: 'error'
      })
      urlRequest.on('error', (error) => {
        assert.strictEqual(error.message, 'Request cannot follow redirect with the current redirect mode')
      })
      urlRequest.on('close', () => {
        done()
      })
      urlRequest.end()
    })

    it('should allow follow redirect when mode is manual', (done) => {
      const requestUrl = '/redirectChain'
      let redirectCount = 0
      server.on('request', (request, response) => {
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
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        redirect: 'manual'
      })
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        assert.strictEqual(redirectCount, 2)
        done()
      })
      urlRequest.on('redirect', (status, method, url) => {
        if (url === `${server.url}/301` || url === `${server.url}/200`) {
          redirectCount += 1
          urlRequest.followRedirect()
        }
      })
      urlRequest.end()
    })

    it('should allow cancelling redirect when mode is manual', (done) => {
      const requestUrl = '/redirectChain'
      let redirectCount = 0
      server.on('request', (request, response) => {
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
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        url: `${server.url}${requestUrl}`,
        redirect: 'manual'
      })
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          urlRequest.abort()
        })
        response.resume()
      })
      urlRequest.on('close', () => {
        assert.strictEqual(redirectCount, 1)
        done()
      })
      urlRequest.on('redirect', (status, method, url) => {
        if (url === `${server.url}/redirect/1`) {
          redirectCount += 1
          urlRequest.followRedirect()
        }
      })
      urlRequest.end()
    })

    it('should throw if given an invalid session option', (done) => {
      const requestUrl = '/requestUrl'
      try {
        const urlRequest = net.request({
          url: `${server.url}${requestUrl}`,
          session: 1
        })

        // eslint-disable-next-line
        urlRequest
      } catch (exception) {
        done()
      }
    })

    it('should to able to create and intercept a request using a custom partition name', (done) => {
      const requestUrl = '/requestUrl'
      const redirectUrl = '/redirectUrl'
      const customPartitionName = 'custom-partition'
      let requestIsRedirected = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case redirectUrl:
            requestIsRedirected = true
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      session.defaultSession.webRequest.onBeforeRequest((details, callback) => {
        assert.fail('Request should not be intercepted by the default session')
      })

      const customSession = session.fromPartition(customPartitionName, {
        cache: false
      })
      let requestIsIntercepted = false
      customSession.webRequest.onBeforeRequest((details, callback) => {
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
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          assert(requestIsRedirected, 'The server should receive a request to the forward URL')
          assert(requestIsIntercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should throw if given an invalid partition option', (done) => {
      const requestUrl = '/requestUrl'
      try {
        const urlRequest = net.request({
          url: `${server.url}${requestUrl}`,
          partition: 1
        })

        // eslint-disable-next-line
        urlRequest
      } catch (exception) {
        done()
      }
    })

    it('should be able to create a request with options', (done) => {
      const requestUrl = '/'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            assert.strictEqual(request.method, 'GET')
            assert.strictEqual(request.headers[customHeaderName.toLowerCase()],
              customHeaderValue)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      const serverUrl = url.parse(server.url)
      const options = {
        port: serverUrl.port,
        hostname: '127.0.0.1',
        headers: {}
      }
      options.headers[customHeaderName] = customHeaderValue
      const urlRequest = net.request(options)
      urlRequest.on('response', (response) => {
        assert.strictEqual(response.statusCode, 200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should be able to pipe a readable stream into a net request', (done) => {
      const nodeRequestUrl = '/nodeRequestUrl'
      const netRequestUrl = '/netRequestUrl'
      const bodyData = randomString(kOneMegaByte)
      let netRequestReceived = false
      let netRequestEnded = false
      server.on('request', (request, response) => {
        switch (request.url) {
          case nodeRequestUrl:
            response.write(bodyData)
            response.end()
            break
          case netRequestUrl:
            netRequestReceived = true
            let receivedBodyData = ''
            request.on('data', (chunk) => {
              receivedBodyData += chunk.toString()
            })
            request.on('end', (chunk) => {
              netRequestEnded = true
              if (chunk) {
                receivedBodyData += chunk.toString()
              }
              assert.strictEqual(receivedBodyData, bodyData)
              response.end()
            })
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })

      const nodeRequest = http.request(`${server.url}${nodeRequestUrl}`)
      nodeRequest.on('response', (nodeResponse) => {
        const netRequest = net.request(`${server.url}${netRequestUrl}`)
        netRequest.on('response', (netResponse) => {
          assert.strictEqual(netResponse.statusCode, 200)
          netResponse.pause()
          netResponse.on('data', (chunk) => {})
          netResponse.on('end', () => {
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

    it('should emit error event on server socket close', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            request.socket.destroy()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      let requestErrorEventEmitted = false
      const urlRequest = net.request(`${server.url}${requestUrl}`)
      urlRequest.on('error', (error) => {
        assert(error)
        requestErrorEventEmitted = true
      })
      urlRequest.on('close', () => {
        assert(requestErrorEventEmitted)
        done()
      })
      urlRequest.end()
    })
  })

  describe('IncomingMessage API', () => {
    it('response object should implement the IncomingMessage API', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.setHeader(customHeaderName, customHeaderValue)
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        assert(typeof statusCode === 'number')
        assert.strictEqual(statusCode, 200)
        const statusMessage = response.statusMessage
        assert(typeof statusMessage === 'string')
        assert.strictEqual(statusMessage, 'OK')
        const headers = response.headers
        assert(typeof headers === 'object')
        assert.deepStrictEqual(headers[customHeaderName.toLowerCase()],
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
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.end()
    })

    it('should be able to pipe a net response into a writable stream', (done) => {
      const nodeRequestUrl = '/nodeRequestUrl'
      const netRequestUrl = '/netRequestUrl'
      const bodyData = randomString(kOneMegaByte)
      server.on('request', (request, response) => {
        switch (request.url) {
          case netRequestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(bodyData)
            response.end()
            break
          case nodeRequestUrl:
            let receivedBodyData = ''
            request.on('data', (chunk) => {
              receivedBodyData += chunk.toString()
            })
            request.on('end', (chunk) => {
              if (chunk) {
                receivedBodyData += chunk.toString()
              }
              assert.strictEqual(receivedBodyData, bodyData)
              response.end()
            })
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      ipcRenderer.once('api-net-spec-done', () => {
        done()
      })
      // Execute below code directly within the browser context without
      // using the remote module.
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const http = require('http')
        const url = require('url')
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
            nodeResponse.on('data', (chunk) => {
            })
            nodeResponse.on('end', (chunk) => {
              event.sender.send('api-net-spec-done')
            })
          })
          netResponse.pipe(nodeRequest)
        })
        netRequest.end()
      `)
    })

    it('should not emit any event after close', (done) => {
      const requestUrl = '/requestUrl'
      const bodyData = randomString(kOneKiloByte)
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(bodyData)
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      let requestCloseEventEmitted = false
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        assert(!requestCloseEventEmitted)
        const statusCode = response.statusCode
        assert.strictEqual(statusCode, 200)
        response.pause()
        response.on('data', () => {
        })
        response.on('end', () => {
        })
        response.resume()
        response.on('error', () => {
          assert(!requestCloseEventEmitted)
        })
        response.on('aborted', () => {
          assert(!requestCloseEventEmitted)
        })
      })
      urlRequest.on('finish', () => {
        assert(!requestCloseEventEmitted)
      })
      urlRequest.on('error', () => {
        assert(!requestCloseEventEmitted)
      })
      urlRequest.on('abort', () => {
        assert(!requestCloseEventEmitted)
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        // Wait so that all async events get scheduled.
        setTimeout(() => {
          done()
        }, 100)
      })
      urlRequest.end()
    })
  })

  describe('Stability and performance', () => {
    it('should free unreferenced, never-started request objects without crash', (done) => {
      const requestUrl = '/requestUrl'
      ipcRenderer.once('api-net-spec-done', () => {
        done()
      })
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const urlRequest = net.request('${server.url}${requestUrl}')
        process.nextTick(() => {
          const v8Util = process.atomBinding('v8_util')
          v8Util.requestGarbageCollectionForTesting()
          event.sender.send('api-net-spec-done')
        })
      `)
    })

    it('should not collect on-going requests without crash', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(randomString(kOneKiloByte))
            ipcRenderer.once('api-net-spec-resume', () => {
              response.write(randomString(kOneKiloByte))
              response.end()
            })
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      ipcRenderer.once('api-net-spec-done', () => {
        done()
      })
      // Execute below code directly within the browser context without
      // using the remote module.
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const urlRequest = net.request('${server.url}${requestUrl}')
        urlRequest.on('response', (response) => {
          response.on('data', () => {
          })
          response.on('end', () => {
            event.sender.send('api-net-spec-done')
          })
          process.nextTick(() => {
            // Trigger a garbage collection.
            const v8Util = process.atomBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
            event.sender.send('api-net-spec-resume')
          })
        })
        urlRequest.end()
      `)
    })

    it('should collect unreferenced, ended requests without crash', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
      ipcRenderer.once('api-net-spec-done', () => {
        done()
      })
      ipcRenderer.send('eval', `
        const {net} = require('electron')
        const urlRequest = net.request('${server.url}${requestUrl}')
        urlRequest.on('response', (response) => {
          response.on('data', () => {
          })
          response.on('end', () => {
          })
        })
        urlRequest.on('close', () => {
          process.nextTick(() => {
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

function handleUnexpectedURL (request, response) {
  response.statusCode = '500'
  response.end()
  assert.fail(`Unexpected URL: ${request.url}`)
}
