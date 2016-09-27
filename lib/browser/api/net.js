'use strict'

const {EventEmitter} = require('events')
const binding = process.atomBinding('net')
const {net, Net} = binding
const {URLRequest} = net

Object.setPrototypeOf(Net.prototype, EventEmitter.prototype)
Object.setPrototypeOf(URLRequest.prototype, EventEmitter.prototype)

class IncomingMessage extends EventEmitter {
  constructor(url_request) {
    super();
    this._url_request = url_request;
  }

  get statusCode() {
    return this._url_request.statusCode;
  }

  get statusMessage() {
    return this._url_request.statusMessage;
  }

  get headers() {
    return this._url_request.rawResponseHeaders;
  }

  get httpVersion() {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`;
  }

  get httpVersionMajor() {
    return this._url_request.httpVersionMajor;
  }

  get httpVersionMinor() {
    return this._url_request.httpVersionMinor;
  }

  get rawHeaders() {
    return this._url_request.rawResponseHeaders;
  }

}

URLRequest.prototype._emitRequestEvent = function() {
  this._request.emit.apply(this._request, arguments);
}

URLRequest.prototype._emitResponseEvent = function() {
  this._response.emit.apply(this._response, arguments);
}

class ClientRequest extends EventEmitter {

  constructor(options, callback) {
    super();

    // Flag to prevent writings after end.
    this._finished = false; 

    // Set when the request uses chuned encoding. Can be switched
    // to true only once and never set back to false.
    this._chunkedEncoding = false;

    // Flag to prevent request's headers modifications after 
    // headers flush.
    this._headersSent = false; 

    // This is a copy of the extra headers structure held by the native
    // net::URLRequest. The main reason is to keep the getHeader API synchronous
    // after the request starts.
    this._extra_headers = {};

    let url = options.url;
    let method = options.method;
    let session = options.session;

    let url_request = new URLRequest({
      url: url,
      method: method,
      session: session
    });

    this._url_request = url_request;
    url_request._request = this;

    url_request.on('response', ()=> {
      let response = new IncomingMessage(url_request);
      url_request._response = response;
      this.emit('response', response);
    });

    if (callback) {
      this.once('response', callback)
    }
  }


  setHeader(name, value) {
    if (typeof name !== 'string')
      throw new TypeError('`name` should be a string in setHeader(name, value).');
    if (value === undefined)
      throw new Error('`value` required in setHeader("' + name + '", value).');
    if (this._headersSent)
      throw new Error('Can\'t set headers after they are sent.');

    let key = name.toLowerCase();
    this._extra_headers[key] = value;
    this._url_request.setExtraHeader(name, value)
  }


  getHeader(name) {
    if (arguments.length < 1) {
      throw new Error('`name` is required for getHeader(name).');
    }

    if (!this._extra_headers) {
      return;
    }

    var key = name.toLowerCase();
    return this._extra_headers[key];
  }


  removeHeader(name) {
    if (arguments.length < 1) {
      throw new Error('`name` is required for removeHeader(name).');
    }

    if (this._headersSent) {
      throw new Error('Can\'t remove headers after they are sent.');
    }

    var key = name.toLowerCase();
    delete this._extra_headers[key];
    this._url_request.removeExtraHeader(name);
  }


  _write(chunk, encoding, callback, is_last) {
  
    let chunk_is_string = typeof chunk === 'string';
    let chunk_is_buffer = chunk instanceof Buffer;
    if (!chunk_is_string && !chunk_is_buffer) {
      throw new TypeError('First argument must be a string or Buffer.');
    }

    if (chunk_is_string) {
      // We convert all strings into binary buffers.
      chunk = Buffer.from(chunk, encoding);
    }

    // Headers are assumed to be sent on first call to _writeBuffer,
    // i.e. after the first call to write or end.
    let result = this._url_request.writeBuffer(chunk, is_last);

    // Since writing to the network is asynchronous, we conservatively 
    // assume that request headers are written after delivering the first
    // buffer to the network IO thread.
    if (!this._headersSent) {
      this._url_request.setChunkedUpload(this._chunkedEncoding);
      this._headersSent = true;
    }

    // The write callback is fired asynchronously to mimic Node.js.
    if (callback) {
      process.nextTick(callback);
    }

    return result;
  }

  write(data, encoding, callback) {
   
    if (this._finished) {
      let error = new Error('Write after end.');
      process.nextTick(writeAfterEndNT, this, error, callback);
      return true;
    }

    return this._write(data, encoding, callback, false);
  }


  end(data, encoding, callback) {
    if (this._finished) {
      return false;
    }
   
    this._finished = true;

    if (typeof data === 'function') {
      callback = data;
      encoding = null;
      data = null;
    } else if (typeof encoding === 'function') {
      callback = encoding;
      encoding = null;
    }

    data = data || '';

    return this._write(data, encoding, callback, true);
  }

}

function writeAfterEndNT(self, error, callback) {
  self.emit('error', error);
  if (callback) callback(error);
}


Net.prototype.request = function(options, callback) {
  return new ClientRequest(options, callback);
}

net.ClientRequest = ClientRequest;

module.exports = net
 