const assert = require('assert')
const {remote} = require('electron')
const {net} = remote

describe.only('net module', function() {
  describe('HTTP basics', function() {
    it ('should be able to fetch google.com', function(done) {
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

    it ('should be able to post data', function(done) {
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
  })
  describe('ClientRequest API', function() {
    it ('should be able to set a custom HTTP header', function() {
      assert(false)
    })
    it ('should be able to abort an HTTP request', function() {
      assert(false)
    })
    it ('should be able to pipe into a request', function() {
      assert(false)
    })
    it ('should be able to pipe from a response', function() {
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
    it ('should not emit any event after close', function() {
      assert(false)
    })
  })
})