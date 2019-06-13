const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { remote } = require('electron')
const { ipcRenderer } = require('electron')
const http = require('http')
const url = require('url')
const { net } = remote
const { session } = remote

const { expect } = chai
chai.use(dirtyChai)

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

        expect(requestResponseEventEmitted).to.be.true()
        expect(requestFinishEventEmitted).to.be.true()
        expect(requestCloseEventEmitted).to.be.true()
        expect(responseDataEventEmitted).to.be.true()
        expect(responseEndEventEmitted).to.be.true()
        done()
      }

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        requestResponseEventEmitted = true
        const statusCode = response.statusCode
        expect(statusCode).to.equal(200)
        const buffers = []
        response.pause()
        response.on('data', (chunk) => {
          buffers.push(chunk)
          responseDataEventEmitted = true
        })
        response.on('end', () => {
          const receivedBodyData = Buffer.concat(buffers)
          expect(receivedBodyData.toString()).to.equal(bodyData)
          responseEndEventEmitted = true
          maybeDone(done)
        })
        response.resume()
        response.on('error', (error) => {
          expect(error).to.be.an('Error')
        })
        response.on('aborted', () => {
          expect.fail('response aborted')
        })
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', (error) => {
        expect(error).to.be.an('Error')
      })
      urlRequest.on('abort', () => {
        expect.fail('request aborted')
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
            expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue)
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
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
      urlRequest.write('')
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
      urlRequest.end()
    })

    it('should be able to set a non-string object as a header value', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Integer-Value'
      const customHeaderValue = 900
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue.toString())
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end()
            break
          default:
            expect(request.url).to.equal(requestUrl)
        }
      })
      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        const statusCode = response.statusCode
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
      urlRequest.write('')
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
      urlRequest.end()
    })

    it('should not be able to set a custom HTTP request header after first write', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            expect(request.headers[customHeaderName.toLowerCase()]).to.be.undefined()
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
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.write('')
      expect(() => {
        urlRequest.setHeader(customHeaderName, customHeaderValue)
      }).to.throw()
      expect(urlRequest.getHeader(customHeaderName)).to.be.undefined()
      urlRequest.end()
    })

    it('should be able to remove a custom HTTP request header before first write', (done) => {
      const requestUrl = '/requestUrl'
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            expect(request.headers[customHeaderName.toLowerCase()]).to.be.undefined()
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
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      urlRequest.removeHeader(customHeaderName)
      expect(urlRequest.getHeader(customHeaderName)).to.be.undefined()
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
            expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue)
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
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      urlRequest.write('')
      expect(() => {
        urlRequest.removeHeader(customHeaderName)
      }).to.throw()
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
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
            expect(request.headers[cookieHeaderName.toLowerCase()]).to.equal(cookieHeaderValue)
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
          expect(statusCode).to.equal(200)
          response.pause()
          response.on('data', (chunk) => {})
          response.on('end', () => {
            done()
          })
          response.resume()
        })
        urlRequest.setHeader(cookieHeaderName, cookieHeaderValue)
        expect(urlRequest.getHeader(cookieHeaderName)).to.equal(cookieHeaderValue)
        urlRequest.end()
      }, (error) => {
        done(error)
      })
    })

    it('should be able to abort an HTTP request before first write', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        response.end()
        expect.fail('Unexpected request event')
      })

      let requestAbortEventEmitted = false
      let requestCloseEventEmitted = false

      const urlRequest = net.request({
        method: 'GET',
        url: `${server.url}${requestUrl}`
      })
      urlRequest.on('response', (response) => {
        expect.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        expect.fail('Unexpected finish event')
      })
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        expect(requestAbortEventEmitted).to.be.true()
        expect(requestCloseEventEmitted).to.be.true()
        done()
      })
      urlRequest.abort()
      expect(urlRequest.write('')).to.be.false()
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
        expect.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        expect.fail('Unexpected finish event')
      })
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        expect(requestReceivedByServer).to.be.true()
        expect(requestAbortEventEmitted).to.be.true()
        expect(requestCloseEventEmitted).to.be.true()
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
        expect.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        expect(requestFinishEventEmitted).to.be.true()
        expect(requestReceivedByServer).to.be.true()
        expect(requestAbortEventEmitted).to.be.true()
        expect(requestCloseEventEmitted).to.be.true()
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
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          expect.fail('Unexpected end event')
        })
        response.resume()
        response.on('error', () => {
          expect.fail('Unexpected error event')
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
        expect.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        requestAbortEventEmitted = true
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        expect(requestFinishEventEmitted).to.be.true('request should emit "finish" event')
        expect(requestReceivedByServer).to.be.true('request should be received by the server')
        expect(requestResponseEventEmitted).to.be.true('"response" event should be emitted')
        expect(requestAbortEventEmitted).to.be.true('request should emit "abort" event')
        expect(responseAbortedEventEmitted).to.be.true('response should emit "aborted" event')
        expect(requestCloseEventEmitted).to.be.true('request should emit "close" event')
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
        expect.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event')
      })
      urlRequest.on('abort', () => {
        ++requestAbortEventCount
        urlRequest.abort()
      })
      urlRequest.on('close', () => {
        requestCloseEventEmitted = true
        // Let all pending async events to be emitted
        setTimeout(() => {
          expect(requestFinishEventEmitted).to.be.true()
          expect(requestReceivedByServer).to.be.true()
          expect(requestAbortEventCount).to.equal(1)
          expect(requestCloseEventEmitted).to.be.true()
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
        expect(response.statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL')
          expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module')
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
        expect.fail('Request should not be intercepted by the default session')
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
        expect(response.statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL')
          expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module')
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
      expect(() => {
        net.request(options)
      }).to.throw('redirect mode should be one of follow, error or manual')
    })

    it('should throw when calling getHeader without a name', () => {
      expect(() => {
        net.request({ url: `${server.url}/requestUrl` }).getHeader()
      }).to.throw(/`name` is required for getHeader\(name\)\./)

      expect(() => {
        net.request({ url: `${server.url}/requestUrl` }).getHeader(null)
      }).to.throw(/`name` is required for getHeader\(name\)\./)
    })

    it('should throw when calling removeHeader without a name', () => {
      expect(() => {
        net.request({ url: `${server.url}/requestUrl` }).removeHeader()
      }).to.throw(/`name` is required for removeHeader\(name\)\./)

      expect(() => {
        net.request({ url: `${server.url}/requestUrl` }).removeHeader(null)
      }).to.throw(/`name` is required for removeHeader\(name\)\./)
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
        expect(response.statusCode).to.equal(200)
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
        expect(response.statusCode).to.equal(200)
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
        expect(error.message).to.equal('Request cannot follow redirect with the current redirect mode')
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
        expect(response.statusCode).to.equal(200)
        expect(redirectCount).to.equal(2)
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
        expect(response.statusCode).that.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          urlRequest.abort()
        })
        response.resume()
      })
      urlRequest.on('close', () => {
        expect(redirectCount).to.equal(1)
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
        expect.fail('Request should not be intercepted by the default session')
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
        expect(response.statusCode).to.equal(200)
        response.pause()
        response.on('data', (chunk) => {
        })
        response.on('end', () => {
          expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL')
          expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module')
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
            expect(request.method).to.equal('GET')
            expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue)
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
        expect(response.statusCode).to.be.equal(200)
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
              expect(receivedBodyData).to.be.equal(bodyData)
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
          expect(netResponse.statusCode).to.be.equal(200)
          netResponse.pause()
          netResponse.on('data', (chunk) => {})
          netResponse.on('end', () => {
            expect(netRequestReceived).to.be.true()
            expect(netRequestEnded).to.be.true()
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
        expect(error).to.be.an('Error')
        requestErrorEventEmitted = true
      })
      urlRequest.on('close', () => {
        expect(requestErrorEventEmitted).to.be.true()
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
        expect(response.statusCode).to.equal(200)
        expect(response.statusMessage).to.equal('OK')

        const headers = response.headers
        expect(headers).to.be.an('object')
        const headerValue = headers[customHeaderName.toLowerCase()]
        expect(headerValue).to.equal(customHeaderValue)

        const httpVersion = response.httpVersion
        expect(httpVersion).to.be.a('string').and.to.have.lengthOf.at.least(1)

        const httpVersionMajor = response.httpVersionMajor
        expect(httpVersionMajor).to.be.a('number').and.to.be.at.least(1)

        const httpVersionMinor = response.httpVersionMinor
        expect(httpVersionMinor).to.be.a('number').and.to.be.at.least(0)

        response.pause()
        response.on('data', chunk => {})
        response.on('end', () => { done() })
        response.resume()
      })
      urlRequest.end()
    })

    it('should discard duplicate headers', (done) => {
      const requestUrl = '/duplicateRequestUrl'
      const includedHeader = 'max-forwards'
      const discardableHeader = 'Max-Forwards'

      const includedHeaderValue = 'max-fwds-val'
      const discardableHeaderValue = 'max-fwds-val-two'

      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.setHeader(discardableHeader, discardableHeaderValue)
            response.setHeader(includedHeader, includedHeaderValue)
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

      urlRequest.on('response', response => {
        expect(response.statusCode).to.equal(200)
        expect(response.statusMessage).to.equal('OK')

        const headers = response.headers
        expect(headers).to.be.an('object')

        expect(headers).to.have.property(includedHeader)
        expect(headers).to.not.have.property(discardableHeader)
        expect(headers[includedHeader]).to.equal(includedHeaderValue)

        response.pause()
        response.on('data', chunk => {})
        response.on('end', () => { done() })
        response.resume()
      })
      urlRequest.end()
    })

    it('should join repeated non-discardable value with ,', (done) => {
      const requestUrl = '/requestUrl'

      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.setHeader('referrer-policy', ['first-text', 'second-text'])
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

      urlRequest.on('response', response => {
        expect(response.statusCode).to.equal(200)
        expect(response.statusMessage).to.equal('OK')

        const headers = response.headers
        expect(headers).to.be.an('object')
        expect(headers).to.have.a.property('referrer-policy')
        expect(headers['referrer-policy']).to.equal('first-text, second-text')

        response.pause()
        response.on('data', chunk => {})
        response.on('end', () => { done() })
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
              expect(receivedBodyData).to.equal(bodyData)
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
        expect(requestCloseEventEmitted).to.be.false()
        const statusCode = response.statusCode
        expect(statusCode).to.equal(200)
        response.pause()
        response.on('data', () => {
        })
        response.on('end', () => {
        })
        response.resume()
        response.on('error', () => {
          expect(requestCloseEventEmitted).to.be.false()
        })
        response.on('aborted', () => {
          expect(requestCloseEventEmitted).to.be.false()
        })
      })
      urlRequest.on('finish', () => {
        expect(requestCloseEventEmitted).to.be.false()
      })
      urlRequest.on('error', () => {
        expect(requestCloseEventEmitted).to.be.false()
      })
      urlRequest.on('abort', () => {
        expect(requestCloseEventEmitted).to.be.false()
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
          const v8Util = process.electronBinding('v8_util')
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
            const v8Util = process.electronBinding('v8_util')
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
            const v8Util = process.electronBinding('v8_util')
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
  expect.fail(`Unexpected URL: ${request.url}`)
}
