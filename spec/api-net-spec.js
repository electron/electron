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
