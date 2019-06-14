import { expect } from 'chai'
import { net, session, ClientRequest } from 'electron'
import * as http from 'http'
import { AddressInfo } from 'net'

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

function respondOnce(fn: http.RequestListener) {
  return new Promise((resolve) => {
    const server = http.createServer((request, response) => {
      fn(request, response)
      server.close()
    })
    server.listen(0, '127.0.0.1', () => {
      resolve(`http://127.0.0.1:${(server.address() as AddressInfo).port}`)
    })
  })
}

respondOnce.toURL = (url: string, fn: http.RequestListener) => {
  return respondOnce((request, response) => {
    if (request.url === url) {
      fn(request, response)
    } else {
      response.statusCode = 500
      response.end()
      expect.fail(`Unexpected URL: ${request.url}`)
    }
  })
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
        response.write(bodyData)
        response.end()
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
        const sentChunks: Array<Buffer> = []
        const receivedChunks: Array<Buffer> = []
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          response.on('data', (chunk) => {
            receivedChunks.push(chunk)
          })
          response.on('end', () => {
            const sentData = Buffer.concat(sentChunks)
            const receivedData = Buffer.concat(receivedChunks)
            expect(sentData.toString()).to.equal(receivedData.toString())
            expect(chunkIndex).to.be.equal(chunkCount)
            done()
          })
        })
        urlRequest.chunkedEncoding = true
        while (chunkIndex < chunkCount) {
          chunkIndex += 1
          const chunk = randomBuffer(kOneKiloByte)
          sentChunks.push(chunk)
          expect(urlRequest.write(chunk)).to.equal(true)
        }
        urlRequest.end()
      })
    })
  })

  describe('ClientRequest API', () => {
    it('request/response objects should emit expected events', (done) => {
      const bodyData = randomString(kOneMegaByte)
      respondOnce.toSingleURL((request, response) => {
        response.statusCode = 200
        response.statusMessage = 'OK'
        response.write(bodyData)
        response.end()
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

    it('should be able to set a non-string object as a header value', (done) => {
      const customHeaderName = 'Some-Integer-Value'
      const customHeaderValue = 900
      respondOnce.toSingleURL((request, response) => {
        expect(request.headers[customHeaderName.toLowerCase()]).to.equal(customHeaderValue.toString())
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
        expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
        urlRequest.write('')
        expect(urlRequest.getHeader(customHeaderName)).to.equal(customHeaderValue)
        expect(urlRequest.getHeader(customHeaderName.toLowerCase())).to.equal(customHeaderValue)
        urlRequest.end()
      })
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

    it('should be able to abort an HTTP request before first write', (done) => {
      respondOnce.toSingleURL((request, response) => {
        response.end()
        expect.fail('Unexpected request event')
      }).then(serverUrl => {
        let requestAbortEventEmitted = false

        const urlRequest = net.request(serverUrl)
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
          expect(requestAbortEventEmitted).to.equal(true)
          done()
        })
        urlRequest.abort()
        expect(urlRequest.write('')).to.equal(false)
        urlRequest.end()
      })
    })

    it('it should be able to abort an HTTP request before request end', (done) => {
      let requestReceivedByServer = false
      let urlRequest: ClientRequest | null = null
      respondOnce.toSingleURL((request, response) => {
        requestReceivedByServer = true
        urlRequest!.abort()
      }).then(serverUrl => {
        let requestAbortEventEmitted = false

        urlRequest = net.request(serverUrl)
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
          expect(requestReceivedByServer).to.equal(true)
          expect(requestAbortEventEmitted).to.equal(true)
          done()
        })

        urlRequest.chunkedEncoding = true
        urlRequest.write(randomString(kOneKiloByte))
      })
    })

  })
})
