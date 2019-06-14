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
