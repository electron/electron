import { expect } from 'chai'
import { net } from 'electron'
import * as http from 'http'
import { AddressInfo, Socket } from 'net'

const kOneKiloByte = 1024

function randomBuffer (size: number, start: number = 0, end: number = 255) {
  const range = 1 + end - start
  const buffer = Buffer.allocUnsafe(size)
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random() * range)
  }
  return buffer
}

describe('net module', () => {
  let server: http.Server = null as unknown as http.Server
  let serverUrl: string = null as unknown as string
  const connections: Set<Socket> = new Set()
  beforeEach((done) => {
    server = http.createServer()
    server.listen(0, '127.0.0.1', () => {
      serverUrl = `http://127.0.0.1:${(server.address() as AddressInfo).port}`
      done()
    })
    // Without force-closing these connections, `server.close()` waits for a
    // timeout and slows down the afterEach hook significantly.
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
      server = null as unknown as http.Server
      done()
    })
  })

  describe('HTTP basics', () => {
    it('should be able to issue a basic GET request', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            expect(request.method).to.equal('GET')
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
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

    it('should be able to issue a basic POST request', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            expect(request.method).to.equal('POST')
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
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

    it('should fetch correct data in a GET request', (done) => {
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
            expect(request.method).to.equal('GET')
            response.write(bodyData)
            response.end()
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
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

    it('should post the correct data in a POST request', (done) => {
      const requestUrl = '/requestUrl'
      const bodyData = 'Hello World!'
      server.on('request', (request, response) => {
        let postedBodyData = ''
        switch (request.url) {
          case requestUrl:
            expect(request.method).to.equal('POST')
            request.on('data', (chunk: Buffer) => {
              postedBodyData += chunk.toString()
            })
            request.on('end', () => {
              expect(postedBodyData).to.equal(bodyData)
              response.end()
            })
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
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

    it('should support chunked encoding', (done) => {
      const requestUrl = '/requestUrl'
      server.on('request', (request, response) => {
        switch (request.url) {
          case requestUrl:
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
            break
          default:
            handleUnexpectedURL(request, response)
        }
      })
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

function handleUnexpectedURL (request: http.IncomingMessage, response: http.ServerResponse) {
  response.statusCode = 500
  response.end()
  expect.fail(`Unexpected URL: ${request.url}`)
}
