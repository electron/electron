const assert = require('assert')
const {remote} = require('electron')
const http = require('http')
const url = require('url')
const {net} = remote
const {session} = remote

function randomBuffer(size, start, end) {
  start = start || 0
  end = end || 255
  let range = 1 + end - start
  const buffer = Buffer.allocUnsafe(size)
  for (let i = 0; i < size; ++i) {
    buffer[i] = start + Math.floor(Math.random()*range)
  }
  return buffer;
}

function randomString(length) {
  let buffer = randomBuffer(length, '0'.charCodeAt(0), 'z'.charCodeAt(0))
  return buffer.toString();
}

const kOneKiloByte = 1024
const kOneMegaByte = kOneKiloByte * kOneKiloByte


describe('net module', function() {
  this.timeout(0)
  describe('HTTP basics', function() {

    let server
    beforeEach(function (done) {
      server = http.createServer()
      server.listen(0, '127.0.0.1', function () {
        server.url = 'http://127.0.0.1:' + server.address().port
        done()
      })
    })

    afterEach(function () {
      server.close(function() {
      })
      server = null
    })

    it('should be able to issue a basic GET request', function(done) {
      const request_url = '/request_url'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert.equal(request.method, 'GET')
            response.end();
            break;
          default:
            assert(false)
        }
      })
      const urlRequest = net.request(`${server.url}${request_url}`)
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })

    it('should be able to issue a basic POST request', function(done) {
      const request_url = '/request_url'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert.equal(request.method, 'POST')
            response.end();
            break;
          default:
            assert(false)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })

    it('should fetch correct data in a GET request', function(done) {
      const request_url = '/request_url'
      const body_data = "Hello World!"
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert.equal(request.method, 'GET')
            response.write(body_data)
            response.end();
            break;
          default:
            assert(false)
        }
      })
      const urlRequest = net.request(`${server.url}${request_url}`)
      urlRequest.on('response', function(response) {
        let expected_body_data = '';
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
          expected_body_data += chunk.toString();
        })
        response.on('end', function() {
          assert.equal(expected_body_data, body_data)
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })

    it('should post the correct data in a POST request', function(done) {
      const request_url = '/request_url'
      const body_data = "Hello World!"
      server.on('request', function(request, response) {
        let posted_body_data = ''
        switch (request.url) {
          case request_url:
            assert.equal(request.method, 'POST')
            request.on('data', function(chunk) {
              posted_body_data += chunk.toString()
            })
            request.on('end', function() {
              assert.equal(posted_body_data, body_data)
              response.end();
            })
            break;
          default:
            assert(false)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.write(body_data)
      urlRequest.end();
    })

    it('should support chunked encoding', function(done) {
      const request_url = '/request_url'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.chunkedEncoding = true
            assert.equal(request.method, 'POST')
            assert.equal(request.headers['transfer-encoding'], 'chunked')
            assert(!request.headers['content-length'])
            request.on('data', function(chunk) {
              response.write(chunk)
            })
            request.on('end', function(chunk) {
              response.end(chunk);
            })
            break;
          default:
            assert(false)
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${request_url}`
      })

      let chunk_index = 0
      let chunk_count = 100
      let sent_chunks = [];
      let received_chunks = [];
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
          received_chunks.push(chunk)
        })
        response.on('end', function() {
          let sent_data = Buffer.concat(sent_chunks)
          let received_data = Buffer.concat(received_chunks)
          assert.equal(sent_data.toString(), received_data.toString())
          assert.equal(chunk_index, chunk_count)
          done()
        })
        response.resume()
      })
      urlRequest.chunkedEncoding = true
      while (chunk_index < chunk_count) {
        ++chunk_index
        let chunk = randomBuffer(kOneKiloByte)
        sent_chunks.push(chunk)
        assert(urlRequest.write(chunk))
      }
      urlRequest.end();
    })
  })

  describe('ClientRequest API', function() {

    let server
    beforeEach(function (done) {
      server = http.createServer()
      server.listen(0, '127.0.0.1', function () {
        server.url = 'http://127.0.0.1:' + server.address().port
        done()
      })
    })

    afterEach(function () {
      server.close(function() {
      })
      server = null
    })

    it ('response object should implement the IncomingMessage API', function(done) {
      const request_url = '/request_url'
      const custom_header_name = 'Some-Custom-Header-Name'
      const custom_header_value = 'Some-Customer-Header-Value'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.setHeader(custom_header_name, custom_header_value)
            response.end();
            break;
          default:
            assert(false)
        }
      })
      let response_event_emitted = false;
      let data_event_emitted = false;
      let end_event_emitted = false;
      let finish_event_emitted = false;
      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        response_event_emitted = true;
        const statusCode = response.statusCode
        assert(typeof statusCode === 'number')
        assert.equal(statusCode, 200)
        const statusMessage = response.statusMessage
        assert(typeof statusMessage === 'string')
        assert.equal(statusMessage, 'OK')
        const rawHeaders = response.rawHeaders
        assert(typeof rawHeaders === 'object')
        assert(rawHeaders[custom_header_name] === 
          custom_header_value)
        const httpVersion = response.httpVersion;
        assert(typeof httpVersion === 'string')
        assert(httpVersion.length > 0)
        const httpVersionMajor = response.httpVersionMajor;
        assert(typeof httpVersionMajor === 'number')
        assert(httpVersionMajor >= 1)
        const httpVersionMinor = response.httpVersionMinor;
        assert(typeof httpVersionMinor === 'number')
        assert(httpVersionMinor >= 0)
        response.pause()
        response.on('data', function(chunk) {
        });
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })



    it('request/response objects should emit expected events', function(done) {

      const request_url = '/request_url'
      let body_data = randomString(kOneMegaByte)
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.write(body_data)
            response.end();
            break;
          default:
            assert(false)
        }
      })

      let request_response_event_emitted = false
      let request_finish_event_emitted = false
      let request_close_event_emitted = false
      let response_data_event_emitted = false
      let response_end_event_emitted = false
      let response_close_event_emitted = false

      function maybeDone(done) {
        if (!request_close_event_emitted || !response_end_event_emitted) {
          return
        }

        assert(request_response_event_emitted)
        assert(request_finish_event_emitted)
        assert(request_close_event_emitted)
        assert(response_data_event_emitted)
        assert(response_end_event_emitted)
        done()
      }

      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        request_response_event_emitted = true;
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        let buffers = [];
        response.pause();
        response.on('data', function(chunk) {
          buffers.push(chunk)
          response_data_event_emitted = true
        })
        response.on('end', function() {
          let received_body_data = Buffer.concat(buffers);
          assert(received_body_data.toString() === body_data)
          response_end_event_emitted = true
          maybeDone(done)
        })
        response.resume();
        response.on('error', function(error) {
          assert.ifError(error);
        })
        response.on('aborted', function() {
          assert(false)
        })
      })
      urlRequest.on('finish', function() {
        request_finish_event_emitted = true
      })
      urlRequest.on('error', function(error) {
        assert.ifError(error);
      })
      urlRequest.on('abort', function() {
        assert(false);
      })
      urlRequest.on('close', function() {
        request_close_event_emitted = true
        maybeDone(done)
      })
      urlRequest.end();
    })

    it('should be able to set a custom HTTP request header before first write', function(done) {
      const request_url = '/request_url'
      const custom_header_name = 'Some-Custom-Header-Name'
      const custom_header_value = 'Some-Customer-Header-Value'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert.equal(request.headers[custom_header_name.toLowerCase()], 
              custom_header_value)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end();
            break;
          default:
            assert(false)
        }
      })
      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        });
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(custom_header_name, custom_header_value)
      assert.equal(urlRequest.getHeader(custom_header_name), 
        custom_header_value)
      assert.equal(urlRequest.getHeader(custom_header_name.toLowerCase()), 
        custom_header_value)
      urlRequest.write('');
      assert.equal(urlRequest.getHeader(custom_header_name), 
        custom_header_value)
      assert.equal(urlRequest.getHeader(custom_header_name.toLowerCase()), 
        custom_header_value)
      urlRequest.end();
    })

    it('should not be able to set a custom HTTP request header after first write', function(done) {
      const request_url = '/request_url'
      const custom_header_name = 'Some-Custom-Header-Name'
      const custom_header_value = 'Some-Customer-Header-Value'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert(!request.headers[custom_header_name.toLowerCase()])
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end();
            break;
          default:
            assert(false)
        }
      })
      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        });
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.write('');
      assert.throws( () => {
        urlRequest.setHeader(custom_header_name, custom_header_value)
      })
      assert(!urlRequest.getHeader(custom_header_name))
      urlRequest.end();
    })

    it('should be able to remove a custom HTTP request header before first write', function(done) {
      const request_url = '/request_url'
      const custom_header_name = 'Some-Custom-Header-Name'
      const custom_header_value = 'Some-Customer-Header-Value'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert(!request.headers[custom_header_name.toLowerCase()])
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end();
            break;
          default:
          assert(false)
        }
      })
      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        });
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(custom_header_name, custom_header_value)
      assert.equal(urlRequest.getHeader(custom_header_name), 
        custom_header_value)
      urlRequest.removeHeader(custom_header_name)
      assert(!urlRequest.getHeader(custom_header_name))
      urlRequest.write('');
      urlRequest.end();
    })

    it('should not be able to remove a custom HTTP request header after first write', function(done) {
      const request_url = '/request_url'
      const custom_header_name = 'Some-Custom-Header-Name'
      const custom_header_value = 'Some-Customer-Header-Value'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert.equal(request.headers[custom_header_name.toLowerCase()], 
              custom_header_value)
            response.statusCode = 200
            response.statusMessage = 'OK'
            response.end();
            break;
          default:
            assert(false)
        }
      })
      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        });
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.setHeader(custom_header_name, custom_header_value)
      assert.equal(urlRequest.getHeader(custom_header_name), 
        custom_header_value)
      urlRequest.write('');
      assert.throws(function() {
        urlRequest.removeHeader(custom_header_name)
      })
      assert.equal(urlRequest.getHeader(custom_header_name), 
        custom_header_value)
      urlRequest.end();
    })

    it('should be able to abort an HTTP request before first write', function() {
      const request_url = '/request_url'
      server.on('request', function(request, response) {
        assert(false)
      })

      let request_abort_event_emitted = false
      let request_close_event_emitted = false

      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert(false)
      })
      urlRequest.on('finish', function() {
        assert(false);
      })
      urlRequest.on('error', function(error) {
        assert(false);
      })
      urlRequest.on('abort', function() {
        request_abort_event_emitted = true
      })
      urlRequest.on('close', function() {
        request_close_event_emitted = true
        assert(request_abort_event_emitted)
        assert(request_close_event_emitted)
        done();
      })
      urlRequest.abort()
      assert(!urlRequest.write(''))
      urlRequest.end();
    })

    
    it('it should be able to abort an HTTP request before request end', function(done) {
      const request_url = '/request_url'
      let request_received_by_server = false
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            request_received_by_server = true;
            cancelRequest();
            break;
          default:
            assert(false)
        }
      })

      let request_abort_event_emitted = false
      let request_close_event_emitted = false

      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert(false)
      })
      urlRequest.on('finish', function() {
        assert(false)
      })
      urlRequest.on('error', function(error) {
        assert(false);
      })
      urlRequest.on('abort', function() {
        request_abort_event_emitted = true
      })
      urlRequest.on('close', function() {
        request_close_event_emitted = true
        assert(request_received_by_server)
        assert(request_abort_event_emitted)
        assert(request_close_event_emitted)
        done()
      })

      urlRequest.chunkedEncoding = true
      urlRequest.write(randomString(kOneKiloByte))
      function cancelRequest() {
        urlRequest.abort()
      }
    })

    it('it should be able to abort an HTTP request after request end and before response', function(done) {
      const request_url = '/request_url'
      let request_received_by_server = false
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            request_received_by_server = true;
            cancelRequest();
            process.nextTick( () => {
              response.statusCode = 200
              response.statusMessage = 'OK'
              response.end();
            })
            break;
          default:
            assert(false)
        }
      })

      let request_abort_event_emitted = false
      let request_finish_event_emitted = false
      let request_close_event_emitted = false

      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert(false)
      })
      urlRequest.on('finish', function() {
        request_finish_event_emitted = true
      })
      urlRequest.on('error', function(error) {
        assert(false);
      })
      urlRequest.on('abort', function() {
        request_abort_event_emitted = true
      })
      urlRequest.on('close', function() {
        request_close_event_emitted = true
        assert(request_finish_event_emitted)
        assert(request_received_by_server)
        assert(request_abort_event_emitted)
        assert(request_close_event_emitted)
        done()
      })

      urlRequest.end(randomString(kOneKiloByte))
      function cancelRequest() {
        urlRequest.abort()
      }
    })

    it('it should be able to abort an HTTP request after response start', function(done) {
      const request_url = '/request_url'
      let request_received_by_server = false
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            request_received_by_server = true;
              response.statusCode = 200
              response.statusMessage = 'OK'
              response.write(randomString(kOneKiloByte))
            break;
          default:
            assert(false)
        }
      })

      let request_finish_event_emitted = false
      let request_response_event_emitted = false
      let request_abort_event_emitted = false
      let request_close_event_emitted = false
      let response_aborted_event_emitted = false


      const urlRequest =  net.request({
        method: 'GET',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        request_response_event_emitted = true
        const statusCode = response.statusCode
        assert.equal(statusCode, 200)
        response.pause();
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          assert(false)
        })
        response.resume();
        response.on('error', function(error) {
          assert(false)
        })
        response.on('aborted', function() {
          response_aborted_event_emitted = true
        })
        urlRequest.abort()
      })
      urlRequest.on('finish', function() {
        request_finish_event_emitted = true
      })
      urlRequest.on('error', function(error) {
        assert(false);
      })
      urlRequest.on('abort', function() {
        request_abort_event_emitted = true
      })
      urlRequest.on('close', function() {
        request_close_event_emitted = true
        assert(request_finish_event_emitted, 'request should emit "finish" event')
        assert(request_received_by_server, 'request should be received by the server')
        assert(request_response_event_emitted, '"response" event should be emitted')
        assert(request_abort_event_emitted, 'request should emit "abort" event')
        assert(response_aborted_event_emitted, 'response should emit "aborted" event')
        assert(request_close_event_emitted, 'request should emit "close" event')
        done()
      })
      urlRequest.end(randomString(kOneKiloByte))
    })

    it('Requests should be intercepted by webRequest module', function(done) {

      const request_url = '/request_url'
      const redirect_url = '/redirect_url'
      let request_is_redirected = false;
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert(false)
            break
          case redirect_url:
            request_is_redirected = true
            response.end();
            break
          default:
            assert(false)
        }
      })

      let request_is_intercepted = false
      session.defaultSession.webRequest.onBeforeRequest(
        function(details, callback){
          if (details.url === `${server.url}${request_url}`) {
            request_is_intercepted = true
            callback({
              redirectURL: `${server.url}${redirect_url}`
            })
          } else {
            callback( {
              cancel: false
            })
          }
        });

      const urlRequest = net.request(`${server.url}${request_url}`)

      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          assert(request_is_redirected, 'The server should receive a request to the forward URL')
          assert(request_is_intercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })

    it('should to able to create and intercept a request on a custom session', function(done) {
      const request_url = '/request_url'
      const redirect_url = '/redirect_url'
      const custom_session_name = 'custom-session'
      let request_is_redirected = false;
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert(false)
            break
          case redirect_url:
            request_is_redirected = true
            response.end();
            break
          default:
            assert(false)
        }
      })

      session.defaultSession.webRequest.onBeforeRequest(
        function(details, callback) {
          assert(false, 'Request should not be intercepted by the default session')
        });

      let custom_session = session.fromPartition(custom_session_name, {
        cache: false
      })
      let request_is_intercepted = false
      custom_session.webRequest.onBeforeRequest(
        function(details, callback){
          if (details.url === `${server.url}${request_url}`) {
            request_is_intercepted = true
            callback({
              redirectURL: `${server.url}${redirect_url}`
            })
          } else {
            callback( {
              cancel: false
            })
          }
        });

      const urlRequest = net.request({
        url: `${server.url}${request_url}`,
        session: custom_session_name
      })
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          assert(request_is_redirected, 'The server should receive a request to the forward URL')
          assert(request_is_intercepted, 'The request should be intercepted by the webRequest module')
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })

    it.only ('should be able to create a request with options', function() {
      const request_url = '/'
      const custom_header_name = 'Some-Custom-Header-Name'
      const custom_header_value = 'Some-Customer-Header-Value'
      server.on('request', function(request, response) {
        switch (request.url) {
          case request_url:
            assert.equal(request.method, 'GET')
            assert.equal(request.headers[custom_header_name.toLowerCase()], 
              custom_header_value)
            response.end();
            break;
          default:
            assert(false)
        }
      })

      const server_url = url.parse(server.url)
      let options = {
        port: server_url.port,
        headers: {}
      }
      options.headers[custom_header_name] = custom_header_value
      const urlRequest = net.request(options)
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.pause()
        response.on('data', function(chunk) {
        })
        response.on('end', function() {
          done()
        })
        response.resume()
      })
      urlRequest.end();
    })

    it('abort request should be emitted at most once', function() {
      assert(false)
    })

    it('headers cannot be manipulated after abort', function() {
      assert(false)
    })

    it ('should be able to pipe into a request', function() {
      assert(false)
    })


  })
  describe('IncomingMessage API', function() {
    it('should provide a Node.js-similar API', function() {
      assert(false)
    })
    it ('should not emit any event after close', function() {
      assert(false)
    })
    it ('should be able to pipe from a response', function() {
      assert(false)
    })
  })
})