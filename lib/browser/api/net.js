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

URLRequest.prototype._init = function() {
  this._finished = false;
  this._hasBody = true;
  this._chunkedEncoding = false;
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


URLRequest.prototype.write = function(chunk, encoding, callback) {
   
  if (this.finished) {
    var err = new Error('write after end');
    process.nextTick(writeAfterEndNT, this, err, callback);

    return true;
  }

 /* TODO
  if (!this._header) {
    this._implicitHeader();
  }
  */

  if (!this._hasBody) {
    //debug('This type of response MUST NOT have a body. ' +
    //      'Ignoring write() calls.');
    return true;
  }
  

  if (typeof chunk !== 'string' && !(chunk instanceof Buffer)) {
    throw new TypeError('First argument must be a string or Buffer');
  }


  // If we get an empty string or buffer, then just do nothing, and
  // signal the user to keep writing.
  if (chunk.length === 0) return true;

  var len, ret;
  if (this.chunkedEncoding) {
    if (typeof chunk === 'string' &&
        encoding !== 'hex' &&
        encoding !== 'base64' &&
        encoding !== 'binary') {
      len = Buffer.byteLength(chunk, encoding);
      chunk = len.toString(16) + CRLF + chunk + CRLF;
      ret = this._send(chunk, encoding, callback);
    } else {
      // buffer, or a non-toString-friendly encoding
      if (typeof chunk === 'string')
        len = Buffer.byteLength(chunk, encoding);
      else
        len = chunk.length;

      if (this.connection && !this.connection.corked) {
        this.connection.cork();
        process.nextTick(connectionCorkNT, this.connection);
      }
      this._send(len.toString(16), 'binary', null);
      this._send(crlf_buf, null, null);
      this._send(chunk, encoding, null);
      ret = this._send(crlf_buf, null, callback);
    }
  } else {
    ret = this._send(chunk, encoding, callback);
  }

  //debug('write ret = ' + ret);
  return ret;
};



URLRequest.prototype.end = function() {
  this._end();
};


function writeAfterEndNT(self, err, callback) {
  self.emit('error', err);
  if (callback) callback(err);
}


module.exports = net
 