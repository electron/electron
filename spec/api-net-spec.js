const assert = require('assert')
const {remote} = require('electron')
const http = require('http')
const url = require('url')
const {net} = remote

describe.only('net module', function() {
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
            response.statusCode = 501
            response.statusMessage = 'Not Implemented'
            response.end()
        }
      })
      const urlRequest = net.request(`${server.url}${request_url}`)
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.on('end', function() {
          done()
        })
        response.on('data', function(chunk) {
        
        })
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
            response.statusCode = 501
            response.statusMessage = 'Not Implemented'
            response.end()
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.on('end', function() {
          done()
        })
        response.on('data', function(chunk) {
        
        })
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
            response.statusCode = 501
            response.statusMessage = 'Not Implemented'
            response.end()
        }
      })
      const urlRequest = net.request(`${server.url}${request_url}`)
      urlRequest.on('response', function(response) {
        let expected_body_data = '';
        assert.equal(response.statusCode, 200)
        response.on('end', function() {
          assert.equal(expected_body_data, body_data)
          done()
        })
        response.on('data', function(chunk) {
          expected_body_data += chunk.toString();
        })
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
            response.statusCode = 501
            response.statusMessage = 'Not Implemented'
            response.end()
        }
      })
      const urlRequest = net.request({
        method: 'POST',
        url: `${server.url}${request_url}`
      })
      urlRequest.on('response', function(response) {
        assert.equal(response.statusCode, 200)
        response.on('end', function() {
          done()
        })
        response.on('data', function(chunk) {
        })
      })
      urlRequest.write(body_data)
      urlRequest.end();
    })

  })
  describe('ClientRequest API', function() {
    it ('should emit ClientRequest events in a GET request', function(done) {
      this.timeout(30000);
      let response_event_emitted = false;
      let data_event_emitted = false;
      let end_event_emitted = false;
      let finish_event_emitted = false;
      const urlRequest =  net.request({
    method: 'GET',
    url: 'https://www.google.com'
  })
      urlRequest.on('response', function(response) {
        response_event_emitted = true;
        const statusCode = response.statusCode
        assert(typeof statusCode === 'number')
        assert.equal(statusCode, 200)
        const statusMessage = response.statusMessage
        const rawHeaders = response.rawHeaders
        assert(typeof rawHeaders === 'object')
        const httpVersion = response.httpVersion;
        assert(typeof httpVersion === 'string')
        assert(httpVersion.length > 0)
        const httpVersionMajor = response.httpVersionMajor;
        assert(typeof httpVersionMajor === 'number')
        assert(httpVersionMajor >= 1)
        const httpVersionMinor = response.httpVersionMinor;
        assert(typeof httpVersionMinor === 'number')
        assert(httpVersionMinor >= 0)
        let body = '';
        response.on('data', function(buffer) {
          data_event_emitted = true;
          body += buffer.toString()
          assert(typeof body === 'string')
          assert(body.length > 0)
  });
        response.on('end', function() {
          end_event_emitted = true;
  })
  });
      urlRequest.on('finish', function() {
        finish_event_emitted = true;
  })
      urlRequest.on('error', function(error) {
        assert.ifError(error);
  })
      urlRequest.on('close', function() {
        assert(response_event_emitted)
        assert(data_event_emitted)
        assert(end_event_emitted)
        assert(finish_event_emitted)
        done()
  })
      urlRequest.end();
  })

    it ('should emit ClientRequest events in a POST request', function(done) {
      this.timeout(20000);
      let response_event_emitted = false;
      let data_event_emitted = false;
      let end_event_emitted = false;
      let finish_event_emitted = false;
      const urlRequest =  net.request({
    method: 'POST',
    url: 'http://httpbin.org/post'
  });
      urlRequest.on('response', function(response) {
        response_event_emitted = true;
        const statusCode = response.statusCode
        assert(typeof statusCode === 'number')
        assert.equal(statusCode, 200)
        const statusMessage = response.statusMessage
        const rawHeaders = response.rawHeaders
        assert(typeof rawHeaders === 'object')
        const httpVersion = response.httpVersion;
        assert(typeof httpVersion === 'string')
        assert(httpVersion.length > 0)
        const httpVersionMajor = response.httpVersionMajor;
        assert(typeof httpVersionMajor === 'number')
        assert(httpVersionMajor >= 1)
        const httpVersionMinor = response.httpVersionMinor;
        assert(typeof httpVersionMinor === 'number')
        assert(httpVersionMinor >= 0)
        let body = '';
        response.on('end', function() {
          end_event_emitted = true;
          assert(response_event_emitted)
          assert(data_event_emitted)
          assert(end_event_emitted)
          assert(finish_event_emitted)
          done()
  })
        response.on('data', function(buffer) {
          data_event_emitted = true;
          body += buffer.toString()
          assert(typeof body === 'string')
          assert(body.length > 0)
  });

  });
      urlRequest.on('finish', function() {
        finish_event_emitted = true;
  })
      urlRequest.on('error', function(error) {
        assert.ifError(error);
  })
      urlRequest.on('close', function() {

  })
      for (let i = 0; i < 100; ++i) {
        urlRequest.write('Hello World!');
  }
      urlRequest.end();
  })

    it ('should be able to set a custom HTTP header', function() {
      assert(false)
  })
    it ('should be able to abort an HTTP request', function() {
      assert(false)
  })
    it ('should be able to pipe into a request', function() {
      assert(false)
  })
    it ('should be able to create a request with options', function() {
      assert(false)
  })
    it ('should be able to specify a custom session', function() {
      assert(false)
  })
    it ('should support chunked encoding', function() {
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