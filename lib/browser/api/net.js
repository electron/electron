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
    return this.request.statusCode;
  }

  get statusMessage() {
    return this.request.statusMessage;
  }

  get headers() {
    return this.request.rawResponseHeaders;
  }

  get httpVersion() {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`;
  }

  get httpVersionMajor() {
    return this.request.httpVersionMajor;
  }

  get httpVersionMinor() {
    return this.request.httpVersionMinor;
  }

  get rawHeaders() {
    return this.request.rawResponseHeaders;
  }

}

Net.prototype.request = function(options, callback) {
  let request = new URLRequest(options)

  if (callback) {
    request.once('response', callback)
  }


  return request
}

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


module.exports = net
 