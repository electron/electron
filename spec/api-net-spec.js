const assert = require('assert')
const {net} = require('electron');

describe('net module', function() {
  describe('HTTP basics', function() {
    it ('should be able to fetch google.com', function(done) {
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
        assert(typeof rawHeaders === 'string')
        assert(rawHeaders.length > 0)
        const httpVersion = response.httpVersion;
        assert(typeof httpVersion === 'string')
        assert(httpVersion.length > 0)
        const httpVersionMajor = response.httpVersionMajor;
        assert(typeof httpVersionMajor === 'number')
        assert(httpVersionMajor >= 1)
        const httpVersionMinor = response.httpVersionMinor;
        assert(typeof rawHeaders === 'number')
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
        asset(response_event_emitted)
        assert(data_event_emitted)
        assert(end_event_emitted)
        assert(finish_event_emitted)
        done()
      })
      urlRequest.end();
    })

    it ('should be able to post data', function(done) {
      let response_event_emitted = false;
      let data_event_emitted = false;
      let end_event_emitted = false;
      let finish_event_emitted = false;
      let urlRequest =  net.request({
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
        assert(typeof rawHeaders === 'string')
        assert(rawHeaders.length > 0)
        const httpVersion = response.httpVersion;
        assert(typeof httpVersion === 'string')
        assert(httpVersion.length > 0)
        const httpVersionMajor = response.httpVersionMajor;
        assert(typeof httpVersionMajor === 'number')
        assert(httpVersionMajor >= 1)
        const httpVersionMinor = response.httpVersionMinor;
        assert(typeof rawHeaders === 'number')
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
        asset(response_event_emitted)
        assert(data_event_emitted)
        assert(end_event_emitted)
        assert(finish_event_emitted)
        done()
      })
      for (let i = 0; i < 100; ++i) {
        urlRequest.write('Hello World!');
      }
      urlRequest.end();
    })
  })
  describe('ClientRequest API', function() {
    it ('should be able to set a custom HTTP header', function() {
      assert(false)
    })
    it ('should be able to abort an HTTP request', function() {
      assert(false)
    })
    it ('should be able to pipe into a request', function() {
    
    })
    it ('should be able to create a request with options', function() {
    
    })
    it ('should be able to specify a custom session', function() {
    })
    it ('should support chunked encoding', function() {
    
    })

  })
})