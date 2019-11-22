import { expect } from 'chai'
import { net, session, ClientRequest, BrowserWindow } from 'electron'
import * as http from 'http'
import * as url from 'url'
import { AddressInfo } from 'net'
import { emittedOnce } from './events-helpers'

const kOneKiloByte = 1024
const kOneMegaByte = kOneKiloByte * kOneKiloByte

function randomBuffer (size: number, start: number = 0, end: number = 255) {
  const range = 1 + end - start
  const buffer = Buffer.allocUnsafe(size)
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random() * range)
  }
  return buffer
}

function randomString (length: number) {
  const buffer = randomBuffer(length, '0'.charCodeAt(0), 'z'.charCodeAt(0))
  return buffer.toString()
}

function respondOnce (fn: http.RequestListener): Promise<string> {
  return new Promise((resolve) => {
    const server = http.createServer((request, response) => {
      fn(request, response)
      // don't close if a redirect was returned
      if (response.statusCode < 300 || response.statusCode >= 399) { server.close() }
    })
    server.listen(0, '127.0.0.1', () => {
      resolve(`http://127.0.0.1:${(server.address() as AddressInfo).port}`)
    })
  })
}

respondOnce.toRoutes = (routes: Record<string, http.RequestListener>) => {
  return respondOnce((request, response) => {
    if (routes.hasOwnProperty(request.url || '')) {
      routes[request.url || ''](request, response)
    } else {
      response.statusCode = 500
      response.end()
      expect.fail(`Unexpected URL: ${request.url}`)
    }
  })
}

respondOnce.toURL = (url: string, fn: http.RequestListener) => {
  return respondOnce.toRoutes({ [url]: fn })
}

respondOnce.toSingleURL = (fn: http.RequestListener) => {
  const requestUrl = '/requestUrl'
  return respondOnce.toURL(requestUrl, fn).then(url => `${url}${requestUrl}`)
}

describe('net module', () => {
  describe('HTTP basics', () => {
    it('should be able to issue a basic GET request', (done) => {
      respondOnce.toSingleURL((request, response) => {
        expect(request.method).to.equal('GET')
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            done()
          })
        })
        urlRequest.end()
      })
    })

    it('should be able to issue a basic POST request', (done) => {
      respondOnce.toSingleURL((request, response) => {
        expect(request.method).to.equal('POST')
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request({
          method: 'POST',
          url: serverUrl
        })
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          response.on('data', () => { })
          response.on('end', () => {
            done()
          })
        })
        urlRequest.end()
      })
    })

    it('should fetch correct data in a GET request', (done) => {
      const bodyData = 'Hello World!'
      respondOnce.toSingleURL((request, response) => {
        expect(request.method).to.equal('GET')
        response.end(bodyData)
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          let expectedBodyData = ''
          expect(response.statusCode).to.equal(200)
          response.on('data', (chunk) => {
            expectedBodyData += chunk.toString()
          })
          response.on('end', () => {
            expect(expectedBodyData).to.equal(bodyData)
            done()
          })
        })
        urlRequest.end()
      })
    })

    it('should post the correct data in a POST request', (done) => {
      const bodyData = 'Hello World!'
      respondOnce.toSingleURL((request, response) => {
        let postedBodyData = ''
        expect(request.method).to.equal('POST')
        request.on('data', (chunk: Buffer) => {
          postedBodyData += chunk.toString()
        })
        request.on('end', () => {
          expect(postedBodyData).to.equal(bodyData)
          response.end()
        })
      }).then(serverUrl => {
        const urlRequest = net.request({
          method: 'POST',
          url: serverUrl
        })
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            done()
          })
        })
        urlRequest.write(bodyData)
        urlRequest.end()
      })
    })

    it('should support chunked encoding', (done) => {
      respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.chunkedEncoding = true
        expect(request.method).to.equal('POST')
        expect(request.headers['transfer-encoding']).to.equal('chunked')
        expect(request.headers['content-length']).to.equal(undefined)
        request.on('data', (chunk: Buffer) => {
          response.write(chunk)
        })
        request.on('end', (chunk: Buffer) => {
          response.end(chunk)
        })
      }).then(serverUrl => {
        const urlRequest = net.request({
          method: 'POST',
          url: serverUrl
        })

        let chunkIndex = 0
        const chunkCount = 100
        let sent = Buffer.alloc(0)
        let received = Buffer.alloc(0)
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          response.on('data', (chunk) => {
            received = Buffer.concat([received, chunk])
          })
          response.on('end', () => {
            expect(sent.equals(received)).to.be.true()
            expect(chunkIndex).to.be.equal(chunkCount)
            done()
          })
        })
        urlRequest.chunkedEncoding = true
        while (chunkIndex < chunkCount) {
          chunkIndex += 1
          const chunk = randomBuffer(kOneKiloByte)
          sent = Buffer.concat([sent, chunk])
          urlRequest.write(chunk)
        }
        urlRequest.end()
      })
    })

    it('should emit the login event when 401', async () => {
      const [user, pass] = ['user', 'pass']
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end()
        }
        response.writeHead(200).end('ok')
      })
      let loginAuthInfo: any
      await new Promise((resolve, reject) => {
        const request = net.request({ method: 'GET', url: serverUrl })
        request.on('response', (response) => {
          response.on('error', reject)
          response.on('data', () => {})
          response.on('end', () => resolve())
        })
        request.on('login', (authInfo, cb) => {
          loginAuthInfo = authInfo
          cb(user, pass)
        })
        request.on('error', reject)
        request.end()
      })
      expect(loginAuthInfo.realm).to.equal('Foo')
      expect(loginAuthInfo.scheme).to.equal('basic')
    })

    it('should produce an error on the response object when cancelling authentication', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        if (!request.headers.authorization) {
          return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end()
        }
        response.writeHead(200).end('ok')
      })
      await expect(new Promise((resolve, reject) => {
        const request = net.request({ method: 'GET', url: serverUrl })
        request.on('response', (response) => {
          response.on('error', reject)
          response.on('data', () => {})
          response.on('end', () => resolve())
        })
        request.on('login', (authInfo, cb) => {
          cb()
        })
        request.on('error', reject)
        request.end()
      })).to.eventually.be.rejectedWith('net::ERR_HTTP_RESPONSE_CODE_FAILURE')
    })

    it('should share credentials with WebContents', async () => {
      const [user, pass] = ['user', 'pass']
      const serverUrl = await new Promise<string>((resolve) => {
        const server = http.createServer((request, response) => {
          if (!request.headers.authorization) {
            return response.writeHead(401, { 'WWW-Authenticate': 'Basic realm="Foo"' }).end()
          }
          return response.writeHead(200).end('ok')
        })
        server.listen(0, '127.0.0.1', () => {
          resolve(`http://127.0.0.1:${(server.address() as AddressInfo).port}`)
        })
        after(() => { server.close() })
      })
      const bw = new BrowserWindow({ show: false })
      const loaded = bw.loadURL(serverUrl)
      bw.webContents.on('login', (event, details, authInfo, cb) => {
        event.preventDefault()
        cb(user, pass)
      })
      await loaded
      bw.close()
      await new Promise((resolve, reject) => {
        const request = net.request({ method: 'GET', url: serverUrl })
        request.on('response', (response) => {
          response.on('error', reject)
          response.on('data', () => {})
          response.on('end', () => resolve())
        })
        request.on('login', () => {
          // we shouldn't receive a login event, because the credentials should
          // be cached.
          reject(new Error('unexpected login event'))
        })
        request.on('error', reject)
        request.end()
      })
    })

    it('should share proxy credentials with WebContents', async () => {
      const [user, pass] = ['user', 'pass']
      const proxyPort = await new Promise<number>((resolve) => {
        const server = http.createServer((request, response) => {
          if (!request.headers['proxy-authorization']) {
            return response.writeHead(407, { 'Proxy-Authenticate': 'Basic realm="Foo"' }).end()
          }
          return response.writeHead(200).end('ok')
        })
        server.listen(0, '127.0.0.1', () => {
          resolve((server.address() as AddressInfo).port)
        })
        after(() => { server.close() })
      })
      const customSession = session.fromPartition(`net-proxy-test-${Math.random()}`)
      await customSession.setProxy({ proxyRules: `127.0.0.1:${proxyPort}`, proxyBypassRules: '<-loopback>' })
      const bw = new BrowserWindow({ show: false, webPreferences: { session: customSession } })
      const loaded = bw.loadURL('http://127.0.0.1:9999')
      bw.webContents.on('login', (event, details, authInfo, cb) => {
        event.preventDefault()
        cb(user, pass)
      })
      await loaded
      bw.close()
      await new Promise((resolve, reject) => {
        const request = net.request({ method: 'GET', url: 'http://127.0.0.1:9999', session: customSession })
        request.on('response', (response) => {
          response.on('error', reject)
          response.on('data', () => {})
          response.on('end', () => resolve())
        })
        request.on('login', () => {
          // we shouldn't receive a login event, because the credentials should
          // be cached.
          reject(new Error('unexpected login event'))
        })
        request.on('error', reject)
        request.end()
      })
    })
  })

  describe('ClientRequest API', () => {
    it('request/response objects should emit expected events', (done) => {
      const bodyData = randomString(kOneKiloByte)
      respondOnce.toSingleURL((request, response) => {
        response.end(bodyData)
      }).then(serverUrl => {
        let requestResponseEventEmitted = false
        let requestFinishEventEmitted = false
        let requestCloseEventEmitted = false
        let responseDataEventEmitted = false
        let responseEndEventEmitted = false

        function maybeDone (done: () => void) {
          if (!requestCloseEventEmitted || !responseEndEventEmitted) {
            return
          }

          expect(requestResponseEventEmitted).to.equal(true)
          expect(requestFinishEventEmitted).to.equal(true)
          expect(requestCloseEventEmitted).to.equal(true)
          expect(responseDataEventEmitted).to.equal(true)
          expect(responseEndEventEmitted).to.equal(true)
          done()
        }

        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          requestResponseEventEmitted = true
          const statusCode = response.statusCode
          expect(statusCode).to.equal(200)
          const buffers: Buffer[] = []
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
          response.on('error', (error: Error) => {
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
    })

    it('should be able to set a custom HTTP request header before first write', (done) => {
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue)
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          const statusCode = response.statusCode
          expect(statusCode).to.equal(200)
          response.on('data', () => {
          })
          response.on('end', () => {
            done()
          })
        })
        urlRequest.setHeader(customHeaderName, customHeaderValue)
        expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
        expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
        urlRequest.write('')
        expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
        expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
        urlRequest.end()
      })
    })

    it('should be able to set a non-string object as a header value', async () => {
      const customHeaderName = 'Some-Integer-Value'
      const customHeaderValue = 900
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue.toString())
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      })
      const urlRequest = net.request(serverUrl)
      const complete = new Promise<number>(resolve => {
        urlRequest.on('response', (response) => {
          resolve(response.statusCode)
          response.on('data', () => {})
          response.on('end', () => {})
        })
      })
      urlRequest.setHeader(customHeaderName, customHeaderValue as any)
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
      urlRequest.write('')
      expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
      expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
      urlRequest.end()
      expect(await complete).to.equal(200)
    })

    it('should not be able to set a custom HTTP request header after first write', (done) => {
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(undefined)
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          const statusCode = response.statusCode
          expect(statusCode).to.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            done()
          })
        })
        urlRequest.write('')
        expect(() => {
          urlRequest.setHeader(customHeaderName, customHeaderValue)
        }).to.throw()
        expect(urlRequest.getHeader(customHeaderName)).to.equal(undefined)
        urlRequest.end()
      })
    })

    it('should be able to remove a custom HTTP request header before first write', (done) => {
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(undefined)
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          const statusCode = response.statusCode
          expect(statusCode).to.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            done()
          })
        })
        urlRequest.setHeader(customHeaderName, customHeaderValue)
        expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
        urlRequest.removeHeader(customHeaderName)
        expect(urlRequest.getHeader(customHeaderName)).to.equal(undefined)
        urlRequest.write('')
        urlRequest.end()
      })
    })

    it('should not be able to remove a custom HTTP request header after first write', (done) => {
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue)
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          const statusCode = response.statusCode
          expect(statusCode).to.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            done()
          })
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
    })

    it('should be able to set cookie header line', (done) => {
      const cookieHeaderName = 'Cookie'
      const cookieHeaderValue = 'test=12345'
      const customSession = session.fromPartition('test-cookie-header')
      respondOnce.toSingleURL((request, response) => {
        expect(request.headers[cookieHeaderName.toLowerCase()]).to.equal(cookieHeaderValue)
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      }).then(serverUrl => {
        customSession.cookies.set({
          url: `${serverUrl}`,
          name: 'test',
          value: '11111'
        }).then(() => { // resolved
          const urlRequest = net.request({
            method: 'GET',
            url: serverUrl,
            session: customSession
          })
          urlRequest.on('response', (response) => {
            const statusCode = response.statusCode
            expect(statusCode).to.equal(200)
            response.on('data', () => {})
            response.on('end', () => {
              done()
            })
          })
          urlRequest.setHeader(cookieHeaderName, cookieHeaderValue)
          expect(urlRequest.getHeader(cookieHeaderName)).to.equal(cookieHeaderValue)
          urlRequest.end()
        }, (error) => {
          done(error)
        })
      })
    })

    it('should be able to abort an HTTP request before first write', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end()
        expect.fail('Unexpected request event')
      })

      const urlRequest = net.request(serverUrl)
      urlRequest.on('response', () => {
        expect.fail('unexpected response event')
      })
      urlRequest.abort()
      urlRequest.write('')
      urlRequest.end()
      await emittedOnce(urlRequest, 'abort')
    })

    it('it should be able to abort an HTTP request before request end', (done) => {
      let requestReceivedByServer = false
      let urlRequest: ClientRequest | null = null
      respondOnce.toSingleURL(() => {
        requestReceivedByServer = true
        urlRequest!.abort()
      }).then(serverUrl => {
        let requestAbortEventEmitted = false

        urlRequest = net.request(serverUrl)
        urlRequest.on('response', () => {
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
          expect(requestReceivedByServer).to.equal(true)
          expect(requestAbortEventEmitted).to.equal(true)
          done()
        })

        urlRequest.chunkedEncoding = true
        urlRequest.write(randomString(kOneKiloByte))
      })
    })

    it('it should be able to abort an HTTP request after request end and before response', async () => {
      let requestReceivedByServer = false
      let urlRequest: ClientRequest | null = null
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        requestReceivedByServer = true
        urlRequest!.abort()
        process.nextTick(() => {
          response.statusCode = 200
          response.statusMessage = 'OK'
          response.end()
        })
      })
      let requestFinishEventEmitted = false

      urlRequest = net.request(serverUrl)
      urlRequest.on('response', () => {
        expect.fail('Unexpected response event')
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event')
      })
      urlRequest.end(randomString(kOneKiloByte))
      await emittedOnce(urlRequest, 'abort')
      expect(requestFinishEventEmitted).to.equal(true)
      expect(requestReceivedByServer).to.equal(true)
    })

    it('it should be able to abort an HTTP request after response start', async () => {
      let requestReceivedByServer = false
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        requestReceivedByServer = true
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.write(randomString(kOneKiloByte))
      })
      let requestFinishEventEmitted = false
      let requestResponseEventEmitted = false
      let responseCloseEventEmitted = false

      const urlRequest = net.request(serverUrl)
      urlRequest.on('response', (response) => {
        requestResponseEventEmitted = true
        const statusCode = response.statusCode
        expect(statusCode).to.equal(200)
        response.on('data', () => {})
        response.on('end', () => {
          expect.fail('Unexpected end event')
        })
        response.on('error', () => {
          expect.fail('Unexpected error event')
        })
        response.on('close' as any, () => {
          responseCloseEventEmitted = true
        })
        urlRequest.abort()
      })
      urlRequest.on('finish', () => {
        requestFinishEventEmitted = true
      })
      urlRequest.on('error', () => {
        expect.fail('Unexpected error event')
      })
      urlRequest.end(randomString(kOneKiloByte))
      await emittedOnce(urlRequest, 'abort')
      expect(requestFinishEventEmitted).to.be.true('request should emit "finish" event')
      expect(requestReceivedByServer).to.be.true('request should be received by the server')
      expect(requestResponseEventEmitted).to.be.true('"response" event should be emitted')
      expect(responseCloseEventEmitted).to.be.true('response should emit "close" event')
    })

    it('abort event should be emitted at most once', async () => {
      let requestReceivedByServer = false
      let urlRequest: ClientRequest | null = null
      const serverUrl = await respondOnce.toSingleURL(() => {
        requestReceivedByServer = true
        urlRequest!.abort()
        urlRequest!.abort()
      })
      let requestFinishEventEmitted = false
      let abortsEmitted = 0

      urlRequest = net.request(serverUrl)
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
        abortsEmitted++
      })
      urlRequest.end(randomString(kOneKiloByte))
      await emittedOnce(urlRequest, 'abort')
      expect(requestFinishEventEmitted).to.be.true('request should emit "finish" event')
      expect(requestReceivedByServer).to.be.true('request should be received by server')
      expect(abortsEmitted).to.equal(1, 'request should emit exactly 1 "abort" event')
    })

    describe('webRequest', () => {
      afterEach(() => {
        session.defaultSession.webRequest.onBeforeRequest(null)
      })

      it('Should throw when invalid filters are passed', () => {
        expect(() => {
          session.defaultSession.webRequest.onBeforeRequest(
            { urls: ['*://www.googleapis.com'] },
            (details, callback) => { callback({ cancel: false }) }
          )
        }).to.throw('Invalid url pattern *://www.googleapis.com: Empty path.')

        expect(() => {
          session.defaultSession.webRequest.onBeforeRequest(
            { urls: [ '*://www.googleapis.com/', '*://blahblah.dev' ] },
            (details, callback) => { callback({ cancel: false }) }
          )
        }).to.throw('Invalid url pattern *://blahblah.dev: Empty path.')
      })

      it('Should not throw when valid filters are passed', () => {
        expect(() => {
          session.defaultSession.webRequest.onBeforeRequest(
            { urls: ['*://www.googleapis.com/'] },
            (details, callback) => { callback({ cancel: false }) }
          )
        }).to.not.throw()
      })

      it('Requests should be intercepted by webRequest module', (done) => {
        const requestUrl = '/requestUrl'
        const redirectUrl = '/redirectUrl'
        let requestIsRedirected = false
        respondOnce.toURL(redirectUrl, (request, response) => {
          requestIsRedirected = true
          response.end()
        }).then(serverUrl => {
          let requestIsIntercepted = false
          session.defaultSession.webRequest.onBeforeRequest(
            (details, callback) => {
              if (details.url === `${serverUrl}${requestUrl}`) {
                requestIsIntercepted = true
                // Disabled due to false positive in StandardJS
                // eslint-disable-next-line standard/no-callback-literal
                callback({
                  redirectURL: `${serverUrl}${redirectUrl}`
                })
              } else {
                callback({
                  cancel: false
                })
              }
            })

          const urlRequest = net.request(`${serverUrl}${requestUrl}`)

          urlRequest.on('response', (response) => {
            expect(response.statusCode).to.equal(200)
            response.on('data', () => {})
            response.on('end', () => {
              expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL')
              expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module')
              done()
            })
          })
          urlRequest.end()
        })
      })

      it('should to able to create and intercept a request using a custom session object', (done) => {
        const requestUrl = '/requestUrl'
        const redirectUrl = '/redirectUrl'
        const customPartitionName = 'custom-partition'
        let requestIsRedirected = false
        respondOnce.toURL(redirectUrl, (request, response) => {
          requestIsRedirected = true
          response.end()
        }).then(serverUrl => {
          session.defaultSession.webRequest.onBeforeRequest(() => {
            expect.fail('Request should not be intercepted by the default session')
          })

          const customSession = session.fromPartition(customPartitionName, { cache: false })
          let requestIsIntercepted = false
          customSession.webRequest.onBeforeRequest((details, callback) => {
            if (details.url === `${serverUrl}${requestUrl}`) {
              requestIsIntercepted = true
              // Disabled due to false positive in StandardJS
              // eslint-disable-next-line standard/no-callback-literal
              callback({
                redirectURL: `${serverUrl}${redirectUrl}`
              })
            } else {
              callback({
                cancel: false
              })
            }
          })

          const urlRequest = net.request({
            url: `${serverUrl}${requestUrl}`,
            session: customSession
          })
          urlRequest.on('response', (response) => {
            expect(response.statusCode).to.equal(200)
            response.on('data', () => {})
            response.on('end', () => {
              expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL')
              expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module')
              done()
            })
          })
          urlRequest.end()
        })
      })

      it('should to able to create and intercept a request using a custom session object', (done) => {
        const requestUrl = '/requestUrl'
        const redirectUrl = '/redirectUrl'
        const customPartitionName = 'custom-partition'
        let requestIsRedirected = false
        respondOnce.toURL(redirectUrl, (request, response) => {
          requestIsRedirected = true
          response.end()
        }).then(serverUrl => {
          session.defaultSession.webRequest.onBeforeRequest(() => {
            expect.fail('Request should not be intercepted by the default session')
          })

          const customSession = session.fromPartition(customPartitionName, { cache: false })
          let requestIsIntercepted = false
          customSession.webRequest.onBeforeRequest((details, callback) => {
            if (details.url === `${serverUrl}${requestUrl}`) {
              requestIsIntercepted = true
              // Disabled due to false positive in StandardJS
              // eslint-disable-next-line standard/no-callback-literal
              callback({
                redirectURL: `${serverUrl}${redirectUrl}`
              })
            } else {
              callback({
                cancel: false
              })
            }
          })

          const urlRequest = net.request({
            url: `${serverUrl}${requestUrl}`,
            partition: customPartitionName
          })
          urlRequest.on('response', (response) => {
            expect(response.statusCode).to.equal(200)
            response.on('data', () => {})
            response.on('end', () => {
              expect(requestIsRedirected).to.be.true('The server should receive a request to the forward URL')
              expect(requestIsIntercepted).to.be.true('The request should be intercepted by the webRequest module')
              done()
            })
          })
          urlRequest.end()
        })
      })
    })

    it('should throw if given an invalid redirect mode', () => {
      expect(() => {
        net.request({
          url: 'https://test',
          redirect: 'custom'
        })
      }).to.throw('redirect mode should be one of follow, error or manual')
    })

    it('should throw when calling getHeader without a name', () => {
      expect(() => {
        (net.request({ url: 'https://test' }).getHeader as any)()
      }).to.throw(/`name` is required for getHeader\(name\)/)

      expect(() => {
        net.request({ url: 'https://test' }).getHeader(null as any)
      }).to.throw(/`name` is required for getHeader\(name\)/)
    })

    it('should throw when calling removeHeader without a name', () => {
      expect(() => {
        (net.request({ url: 'https://test' }).removeHeader as any)()
      }).to.throw(/`name` is required for removeHeader\(name\)/)

      expect(() => {
        net.request({ url: 'https://test' }).removeHeader(null as any)
      }).to.throw(/`name` is required for removeHeader\(name\)/)
    })

    it('should follow redirect when no redirect mode is provided', (done) => {
      const requestUrl = '/301'
      respondOnce.toRoutes({
        '/301': (request, response) => {
          response.statusCode = 301
          response.setHeader('Location', '/200')
          response.end()
        },
        '/200': (request, response) => {
          response.statusCode = 200
          response.end()
        }
      }).then(serverUrl => {
        const urlRequest = net.request({
          url: `${serverUrl}${requestUrl}`
        })
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          done()
        })
        urlRequest.end()
      })
    })

    it('should follow redirect chain when no redirect mode is provided', (done) => {
      respondOnce.toRoutes({
        '/redirectChain': (request, response) => {
          response.statusCode = 301
          response.setHeader('Location', '/301')
          response.end()
        },
        '/301': (request, response) => {
          response.statusCode = 301
          response.setHeader('Location', '/200')
          response.end()
        },
        '/200': (request, response) => {
          response.statusCode = 200
          response.end()
        }
      }).then(serverUrl => {
        const urlRequest = net.request({
          url: `${serverUrl}/redirectChain`
        })
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          done()
        })
        urlRequest.end()
      })
    })

    it.skip('should not follow redirect when mode is error', (done) => {
      respondOnce.toSingleURL((request, response) => {
        response.statusCode = 301
        response.setHeader('Location', '/200')
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request({
          url: serverUrl,
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
    })

    it('should allow follow redirect when mode is manual', (done) => {
      respondOnce.toRoutes({
        '/redirectChain': (request, response) => {
          response.statusCode = 301
          response.setHeader('Location', '/301')
          response.end()
        },
        '/301': (request, response) => {
          response.statusCode = 301
          response.setHeader('Location', '/200')
          response.end()
        },
        '/200': (request, response) => {
          response.statusCode = 200
          response.end()
        }
      }).then(serverUrl => {
        const urlRequest = net.request({
          url: `${serverUrl}/redirectChain`,
          redirect: 'manual'
        })
        let redirectCount = 0
        urlRequest.on('response', (response) => {
          try {
            expect(response.statusCode).to.equal(200)
            expect(redirectCount).to.equal(2)
            done()
          } catch (e) {
            done(e)
          }
        })
        urlRequest.on('redirect', (status, method, url) => {
          if (url === `${serverUrl}/301` || url === `${serverUrl}/200`) {
            redirectCount += 1
            urlRequest.followRedirect()
          }
        })
        urlRequest.end()
      })
    })

    it('should allow cancelling redirect when mode is manual', (done) => {
      respondOnce.toRoutes({
        '/redirect': (request, response) => {
          response.statusCode = 301
          response.setHeader('Location', '/200')
          response.end()
        },
        '/200': (request, response) => {
          response.statusCode = 200
          response.end()
        }
      }).then(serverUrl => {
        const urlRequest = net.request({
          url: `${serverUrl}/redirect`,
          redirect: 'manual'
        })
        urlRequest.on('response', (response) => {
          expect(response.statusCode).that.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            urlRequest.abort()
          })
        })
        let redirectCount = 0
        urlRequest.on('close', () => {
          try {
            expect(redirectCount).to.equal(1)
            done()
          } catch (e) {
            done(e)
          }
        })
        urlRequest.on('redirect', (status, method, url) => {
          if (url === `${serverUrl}/200`) {
            redirectCount += 1
            urlRequest.followRedirect()
          }
        })
        urlRequest.end()
      })
    })

    it('should throw if given an invalid session option', () => {
      expect(() => {
        net.request({
          url: 'https://foo',
          session: 1 as any
        })
      }).to.throw('`session` should be an instance of the Session class')
    })

    it('should throw if given an invalid partition option', () => {
      expect(() => {
        net.request({
          url: 'https://foo',
          partition: 1 as any
        })
      }).to.throw('`partition` should be a string')
    })

    it('should be able to create a request with options', (done) => {
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'
      respondOnce.toURL('/', (request, response) => {
        try {
          expect(request.method).to.equal('GET')
          expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue)
        } catch (e) {
          return done(e)
        }
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.end()
      }).then(serverUrlUnparsed => {
        const serverUrl = url.parse(serverUrlUnparsed)
        const options = {
          port: serverUrl.port ? parseInt(serverUrl.port, 10) : undefined,
          hostname: '127.0.0.1',
          headers: { [customHeaderName]: customHeaderValue }
        }
        const urlRequest = net.request(options)
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.be.equal(200)
          response.on('data', () => {})
          response.on('end', () => {
            done()
          })
        })
        urlRequest.end()
      })
    })

    it('should be able to pipe a readable stream into a net request', (done) => {
      const bodyData = randomString(kOneMegaByte)
      let netRequestReceived = false
      let netRequestEnded = false

      Promise.all([
        respondOnce.toSingleURL((request, response) => response.end(bodyData)),
        respondOnce.toSingleURL((request, response) => {
          netRequestReceived = true
          let receivedBodyData = ''
          request.on('data', (chunk) => {
            receivedBodyData += chunk.toString()
          })
          request.on('end', (chunk: Buffer | undefined) => {
            netRequestEnded = true
            if (chunk) {
              receivedBodyData += chunk.toString()
            }
            expect(receivedBodyData).to.be.equal(bodyData)
            response.end()
          })
        })
      ]).then(([nodeServerUrl, netServerUrl]) => {
        const nodeRequest = http.request(nodeServerUrl)
        nodeRequest.on('response', (nodeResponse) => {
          const netRequest = net.request(netServerUrl)
          netRequest.on('response', (netResponse) => {
            expect(netResponse.statusCode).to.equal(200)
            netResponse.on('data', () => {})
            netResponse.on('end', () => {
              expect(netRequestReceived).to.be.true('net request received')
              expect(netRequestEnded).to.be.true('net request ended')
              done()
            })
          })
          nodeResponse.pipe(netRequest)
        })
        nodeRequest.end()
      })
    })

    it('should emit error event on server socket destroy', async () => {
      const serverUrl = await respondOnce.toSingleURL((request) => {
        request.socket.destroy()
      })
      const urlRequest = net.request(serverUrl)
      urlRequest.end()
      const [error] = await emittedOnce(urlRequest, 'error')
      expect(error.message).to.equal('net::ERR_EMPTY_RESPONSE')
    })

    it('should emit error event on server request destroy', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        request.destroy()
        response.end()
      })
      const urlRequest = net.request(serverUrl)
      urlRequest.end(randomBuffer(kOneMegaByte))
      const [error] = await emittedOnce(urlRequest, 'error')
      expect(error.message).to.equal('net::ERR_CONNECTION_RESET')
    })

    it('should not emit any event after close', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.end()
      })

      const urlRequest = net.request(serverUrl)
      urlRequest.end()

      await emittedOnce(urlRequest, 'close')
      await new Promise((resolve, reject) => {
        ['finish', 'abort', 'close', 'error'].forEach(evName => {
          urlRequest.on(evName as any, () => {
            reject(new Error(`Unexpected ${evName} event`))
          })
        })
        setTimeout(resolve, 50)
      })
    })
  })

  describe('IncomingMessage API', () => {
    it('response object should implement the IncomingMessage API', async () => {
      const customHeaderName = 'Some-Custom-Header-Name'
      const customHeaderValue = 'Some-Customer-Header-Value'

      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.setHeader(customHeaderName, customHeaderValue)
        response.end()
      })

      const urlRequest = net.request(serverUrl)
      urlRequest.end()
      const [response] = await emittedOnce(urlRequest, 'response')

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

      response.on('data', () => {})
      await emittedOnce(response, 'end')
    })

    it('should discard duplicate headers', async () => {
      const includedHeader = 'max-forwards'
      const discardableHeader = 'Max-Forwards'

      const includedHeaderValue = 'max-fwds-val'
      const discardableHeaderValue = 'max-fwds-val-two'

      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.setHeader(discardableHeader, discardableHeaderValue)
        response.setHeader(includedHeader, includedHeaderValue)
        response.end()
      })
      const urlRequest = net.request(serverUrl)
      urlRequest.end()

      const [response] = await emittedOnce(urlRequest, 'response')
      expect(response.statusCode).to.equal(200)
      expect(response.statusMessage).to.equal('OK')

      const headers = response.headers
      expect(headers).to.be.an('object')

      expect(headers).to.have.property(includedHeader)
      expect(headers).to.not.have.property(discardableHeader)
      expect(headers[includedHeader]).to.equal(includedHeaderValue)

      response.on('data', () => {})
      await emittedOnce(response, 'end')
    })

    it('should join repeated non-discardable value with ,', async () => {
      const serverUrl = await respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.setHeader('referrer-policy', ['first-text', 'second-text'])
        response.end()
      })
      const urlRequest = net.request(serverUrl)
      urlRequest.end()

      const [response] = await emittedOnce(urlRequest, 'response')
      expect(response.statusCode).to.equal(200)
      expect(response.statusMessage).to.equal('OK')

      const headers = response.headers
      expect(headers).to.be.an('object')
      expect(headers).to.have.property('referrer-policy')
      expect(headers['referrer-policy']).to.equal('first-text, second-text')

      response.on('data', () => {})
      await emittedOnce(response, 'end')
    })

    it('should be able to pipe a net response into a writable stream', (done) => {
      const bodyData = randomString(kOneKiloByte)
      Promise.all([
        respondOnce.toSingleURL((request, response) => response.end(bodyData)),
        respondOnce.toSingleURL((request, response) => {
          let receivedBodyData = ''
          request.on('data', (chunk) => {
            receivedBodyData += chunk.toString()
          })
          request.on('end', (chunk: Buffer | undefined) => {
            if (chunk) {
              receivedBodyData += chunk.toString()
            }
            expect(receivedBodyData).to.be.equal(bodyData)
            response.end()
          })
        })
      ]).then(([netServerUrl, nodeServerUrl]) => {
        const netRequest = net.request(netServerUrl)
        netRequest.on('response', (netResponse) => {
          const serverUrl = url.parse(nodeServerUrl)
          const nodeOptions = {
            method: 'POST',
            path: serverUrl.path,
            port: serverUrl.port
          }
          const nodeRequest = http.request(nodeOptions, res => {
            res.on('data', () => {})
            res.on('end', () => {
              done()
            })
          });
          // TODO: IncomingMessage should properly extend ReadableStream in the
          // docs
          (netResponse as any).pipe(nodeRequest)
        })
        netRequest.end()
      })
    })
  })

  describe('Stability and performance', () => {
    it('should free unreferenced, never-started request objects without crash', (done) => {
      net.request('https://test')
      process.nextTick(() => {
        const v8Util = process.electronBinding('v8_util')
        v8Util.requestGarbageCollectionForTesting()
        done()
      })
    })

    it('should collect on-going requests without crash', (done) => {
      let finishResponse: (() => void) | null = null
      respondOnce.toSingleURL((request, response) => {
        response.write(randomString(kOneKiloByte))
        finishResponse = () => {
          response.write(randomString(kOneKiloByte))
          response.end()
        }
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          response.on('data', () => { })
          response.on('end', () => {
            done()
          })
          process.nextTick(() => {
            // Trigger a garbage collection.
            const v8Util = process.electronBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
            finishResponse!()
          })
        })
        urlRequest.end()
      })
    })

    it('should collect unreferenced, ended requests without crash', (done) => {
      respondOnce.toSingleURL((request, response) => {
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          response.on('data', () => {})
          response.on('end', () => {})
        })
        urlRequest.on('close', () => {
          process.nextTick(() => {
            const v8Util = process.electronBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
            done()
          })
        })
        urlRequest.end()
      })
    })

    it('should finish sending data when urlRequest is unreferenced', (done) => {
      respondOnce.toSingleURL((request, response) => {
        let received = Buffer.alloc(0)
        request.on('data', (data) => {
          received = Buffer.concat([received, data])
        })
        request.on('end', () => {
          response.end()
          expect(received.length).to.equal(kOneMegaByte)
          done()
        })
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          response.on('data', () => {})
          response.on('end', () => {})
        })
        urlRequest.on('close', () => {
          process.nextTick(() => {
            const v8Util = process.electronBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
          })
        })
        urlRequest.end(randomBuffer(kOneMegaByte))
      })
    })

    it('should finish sending data when urlRequest is unreferenced for chunked encoding', (done) => {
      respondOnce.toSingleURL((request, response) => {
        let received = Buffer.alloc(0)
        request.on('data', (data) => {
          received = Buffer.concat([received, data])
        })
        request.on('end', () => {
          response.end()
          expect(received.length).to.equal(kOneMegaByte)
          done()
        })
      }).then(serverUrl => {
        const urlRequest = net.request(serverUrl)
        urlRequest.on('response', (response) => {
          response.on('data', () => {})
          response.on('end', () => {})
        })
        urlRequest.on('close', () => {
          process.nextTick(() => {
            const v8Util = process.electronBinding('v8_util')
            v8Util.requestGarbageCollectionForTesting()
          })
        })
        urlRequest.chunkedEncoding = true
        urlRequest.end(randomBuffer(kOneMegaByte))
      })
    })
  })
})
