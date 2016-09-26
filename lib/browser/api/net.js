'use strict'

const {EventEmitter} = require('events')
const binding = process.atomBinding('net')
const {net, Net} = binding
const {URLRequest} = net

Object.setPrototypeOf(Net.prototype, EventEmitter.prototype)
Object.setPrototypeOf(URLRequest.prototype, EventEmitter.prototype)

class URLResponse extends EventEmitter {
  constructor(request) {
    super();
    this.request = request;
  }

  get statusCode() {
    return this.request._statusCode;
  }

  get statusMessage() {
    return this.request._statusMessage;
  }

  get headers() {
    return this.request._rawResponseHeaders;
  }

  get httpVersion() {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`;
  }

  get httpVersionMajor() {
    return this.request._httpVersionMajor;
  }

  get httpVersionMinor() {
    return this.request._httpVersionMinor;
  }

  get rawHeaders() {
    return this.request._rawResponseHeaders;
  }

}

Net.prototype.request = function(options, callback) {
  let request = new URLRequest(options)

  if (callback) {
    request.once('response', callback)
  }


  return request
}

URLRequest.prototype._init = function() {
  // Flag to prevent writings after end.
  this._finished = false; 

  // Set when the request uses chuned encoding. Can be switched
  // to true only once and never set back to false.
  this._chunkedEncoding = false;

  // Flag to prevent request's headers modifications after 
  // headers flush.
  this._headersSent = false; 
}


URLRequest.prototype.setHeader = function(name, value) {
  if (typeof name !== 'string')
    throw new TypeError('`name` should be a string in setHeader(name, value).');
  if (value === undefined)
    throw new Error('`value` required in setHeader("' + name + '", value).');
  if (this._headersSent)
    throw new Error('Can\'t set headers after they are sent.');

  this._setHeader(name, value)
};


URLRequest.prototype.getHeader = function(name) {
  if (arguments.length < 1) {
    throw new Error('`name` is required for getHeader(name).');
  }

  return this._getHeader(name);
};


URLRequest.prototype.removeHeader = function(name) {
  if (arguments.length < 1) {
    throw new Error('`name` is required for removeHeader(name).');
  }

  if (this._headersSent) {
    throw new Error('Can\'t remove headers after they are sent.');
  }

  this._removeHeader(name);
};


URLRequest.prototype._emitRequestEvent = function(name) {
  if (name === 'response') {
    this.response = new URLResponse(this);
    this.emit(name, this.response);
  } else {
    this.emit.apply(this, arguments);
  }
}

URLRequest.prototype._emitResponseEvent = function() {
  this.response.emit.apply(this.response, arguments);
}


URLRequest.prototype._write = function(chunk, encoding, callback, is_last) {
  
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
  let result = this._writeBuffer(chunk, is_last);

  // Since writing to the network is asynchronous, we conservatively 
  // assume that request headers are written after delivering the first
  // buffer to the network IO thread.
  this._headersSent = true;

  // The write callback is fired asynchronously to mimic Node.js.
  if (callback) {
    process.nextTick(callback);
  }

  return result;
}

URLRequest.prototype.write = function(data, encoding, callback) {
   
  if (this._finished) {
    let error = new Error('Write after end.');
    process.nextTick(writeAfterEndNT, this, error, callback);
    return true;
  }

  return this._write(data, encoding, callback, false);
};


URLRequest.prototype.end = function(data, encoding, callback) {
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
};


function writeAfterEndNT(self, error, callback) {
  self.emit('error', error);
  if (callback) callback(error);
}


module.exports = net
 