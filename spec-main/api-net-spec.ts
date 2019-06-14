import { expect } from 'chai'
import { net } from 'electron'
import * as http from 'http'
import { AddressInfo } from 'net'

const kOneKiloByte = 1024

function randomBuffer (size: number, start: number = 0, end: number = 255) {
  const range = 1 + end - start
  const buffer = Buffer.allocUnsafe(size)
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random() * range)
  }
  return buffer
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

describe('net module', () => {
  describe('HTTP basics', () => {
    it('should be able to issue a basic GET request', (done) => {
      const requestUrl = '/requestUrl'
      respondOnce.toURL(requestUrl, (request, response) => {
        expect(request.method).to.equal('GET')
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(`${serverUrl}${requestUrl}`)
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
      const requestUrl = '/requestUrl'
      respondOnce.toURL(requestUrl, (request, response) => {
        expect(request.method).to.equal('POST')
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request({
          method: 'POST',
          url: `${serverUrl}${requestUrl}`
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
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      respondOnce.toURL(requestUrl, (request, response) => {
        expect(request.method).to.equal('GET')
        response.write(bodyData)
        response.end()
      }).then(serverUrl => {
        const urlRequest = net.request(`${serverUrl}${requestUrl}`)
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
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      respondOnce.toURL(requestUrl, (request, response) => {
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
          url: `${serverUrl}${requestUrl}`
        })
        urlRequest.on('response', (response) => {
          expect(response.statusCode).to.equal(200)
          response.on('data', (chunk) => {})
          response.on('end', () => {
            done()
          })
        })
        urlRequest.write(bodyData)
        urlRequest.end()
      })
    })

    it('should support chunked encoding', (done) => {
      const requestUrl = '/requestUrl'
      respondOnce.toURL(requestUrl, (request, response) => {
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
          url: `${serverUrl}${requestUrl}`
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
})
