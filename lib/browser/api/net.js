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
        return this.request.responseHeaders;
    }

    get httpVersion() {
        return this.request.responseHttpVersion;
    }



}

Net.prototype.request = function(options, callback) {
    let request = new URLRequest(options)

    if (callback) {
        request.once('response', callback)
    }


    return request
}

URLRequest.prototype._emitResponse = function() {
    this.response = new URLResponse(this);
    this.emit('response', this.response);
}

module.exports = net
 