'use strict'

const url = require('url')
const {EventEmitter} = require('events')
const util = require('util')
const {Readable} = require('stream')
const {app} = require('electron')
const {net, Net} = process.atomBinding('net')
const {URLRequest} = net

Object.setPrototypeOf(Net.prototype, EventEmitter.prototype)
Object.setPrototypeOf(URLRequest.prototype, EventEmitter.prototype)

const kSupportedProtocols = new Set(['http:', 'https:'])

class IncomingMessage extends Readable {
  constructor (urlRequest) {
    super()
    this.urlRequest = urlRequest
    this.shouldPush = false
    this.data = []
    this.urlRequest.on('data', (event, chunk) => {
      this._storeInternalData(chunk)
      this._pushInternalData()
    })
    this.urlRequest.on('end', () => {
      this._storeInternalData(null)
      this._pushInternalData()
    })
  }

  get statusCode () {
    return this.urlRequest.statusCode
  }

  get statusMessage () {
    return this.urlRequest.statusMessage
  }

  get headers () {
    return this.urlRequest.rawResponseHeaders
  }

  get httpVersion () {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`
  }

  get httpVersionMajor () {
    return this.urlRequest.httpVersionMajor
  }

  get httpVersionMinor () {
    return this.urlRequest.httpVersionMinor
  }

  get rawTrailers () {
    throw new Error('HTTP trailers are not supported.')
  }

  get trailers () {
    throw new Error('HTTP trailers are not supported.')
  }

  _storeInternalData (chunk) {
    this.data.push(chunk)
  }

  _pushInternalData () {
    while (this.shouldPush && this.data.length > 0) {
      const chunk = this.data.shift()
      this.shouldPush = this.push(chunk)
    }
  }

  _read () {
    this.shouldPush = true
    this._pushInternalData()
  }

}

URLRequest.prototype._emitRequestEvent = function (isAsync, ...rest) {
  if (isAsync) {
    process.nextTick(() => {
      this.clientRequest.emit.apply(this.clientRequest, rest)
    })
  } else {
    this.clientRequest.emit.apply(this.clientRequest, rest)
  }
}

URLRequest.prototype._emitResponseEvent = function (isAsync, ...rest) {
  if (isAsync) {
    process.nextTick(() => {
      this._response.emit.apply(this._response, rest)
    })
  } else {
    this._response.emit.apply(this._response, rest)
  }
}

class ClientRequest extends EventEmitter {

  constructor (options, callback) {
    super()

    if (!app.isReady()) {
      throw new Error('net module can only be used after app is ready')
    }

    if (typeof options === 'string') {
      options = url.parse(options)
    } else {
      options = util._extend({}, options)
    }

    const method = (options.method || 'GET').toUpperCase()
    let urlStr = options.url

    if (!urlStr) {
      let urlObj = {}
      const protocol = options.protocol || 'http:'
      if (!kSupportedProtocols.has(protocol)) {
        throw new Error('Protocol "' + protocol + '" not supported. ')
      }
      urlObj.protocol = protocol

      if (options.host) {
        urlObj.host = options.host
      } else {
        if (options.hostname) {
          urlObj.hostname = options.hostname
        } else {
          urlObj.hostname = 'localhost'
        }

        if (options.port) {
          urlObj.port = options.port
        }
      }

      if (options.path && / /.test(options.path)) {
        // The actual regex is more like /[^A-Za-z0-9\-._~!$&'()*+,;=/:@]/
        // with an additional rule for ignoring percentage-escaped characters
        // but that's a) hard to capture in a regular expression that performs
        // well, and b) possibly too restrictive for real-world usage. That's
        // why it only scans for spaces because those are guaranteed to create
        // an invalid request.
        throw new TypeError('Request path contains unescaped characters.')
      }
      let pathObj = url.parse(options.path || '/')
      urlObj.pathname = pathObj.pathname
      urlObj.search = pathObj.search
      urlObj.hash = pathObj.hash
      urlStr = url.format(urlObj)
    }

    const sessionName = options.session || ''
    let urlRequest = new URLRequest({
      method: method,
      url: urlStr,
      session: sessionName
    })

    // Set back and forward links.
    this.urlRequest = urlRequest
    urlRequest.clientRequest = this

    // This is a copy of the extra headers structure held by the native
    // net::URLRequest. The main reason is to keep the getHeader API synchronous
    // after the request starts.
    this.extraHeaders = {}

    if (options.headers) {
      for (let key in options.headers) {
        this.setHeader(key, options.headers[key])
      }
    }

    // Set when the request uses chunked encoding. Can be switched
    // to true only once and never set back to false.
    this.chunkedEncodingEnabled = false

    urlRequest.on('response', () => {
      const response = new IncomingMessage(urlRequest)
      urlRequest._response = response
      this.emit('response', response)
    })

    urlRequest.on('login', (event, authInfo, callback) => {
      this.emit('login', authInfo, (username, password) => {
        // If null or undefined usrename/password, force to empty string.
        if (username === null || username === undefined) {
          username = ''
        }
        if (typeof username !== 'string') {
          throw new Error('username must be a string')
        }
        if (password === null || password === undefined) {
          password = ''
        }
        if (typeof password !== 'string') {
          throw new Error('password must be a string')
        }
        callback(username, password)
      })
    })

    if (callback) {
      this.once('response', callback)
    }
  }

  get chunkedEncoding () {
    return this.chunkedEncodingEnabled
  }

  set chunkedEncoding (value) {
    if (!this.urlRequest.notStarted) {
      throw new Error('Can\'t set the transfer encoding, headers have been sent.')
    }
    this.chunkedEncodingEnabled = value
  }

  setHeader (name, value) {
    if (typeof name !== 'string') {
      throw new TypeError('`name` should be a string in setHeader(name, value).')
    }
    if (value === undefined) {
      throw new Error('`value` required in setHeader("' + name + '", value).')
    }
    if (!this.urlRequest.notStarted) {
      throw new Error('Can\'t set headers after they are sent.')
    }

    const key = name.toLowerCase()
    this.extraHeaders[key] = value
    this.urlRequest.setExtraHeader(name, value)
  }

  getHeader (name) {
    if (arguments.length < 1) {
      throw new Error('`name` is required for getHeader(name).')
    }

    if (!this.extraHeaders) {
      return
    }

    const key = name.toLowerCase()
    return this.extraHeaders[key]
  }

  removeHeader (name) {
    if (arguments.length < 1) {
      throw new Error('`name` is required for removeHeader(name).')
    }

    if (!this.urlRequest.notStarted) {
      throw new Error('Can\'t remove headers after they are sent.')
    }

    const key = name.toLowerCase()
    delete this.extraHeaders[key]
    this.urlRequest.removeExtraHeader(name)
  }

  _write (chunk, encoding, callback, isLast) {
    let chunkIsString = typeof chunk === 'string'
    let chunkIsBuffer = chunk instanceof Buffer
    if (!chunkIsString && !chunkIsBuffer) {
      throw new TypeError('First argument must be a string or Buffer.')
    }

    if (chunkIsString) {
      // We convert all strings into binary buffers.
      chunk = Buffer.from(chunk, encoding)
    }

    // Since writing to the network is asynchronous, we conservatively
    // assume that request headers are written after delivering the first
    // buffer to the network IO thread.
    if (this.urlRequest.notStarted) {
      this.urlRequest.setChunkedUpload(this.chunkedEncoding)
    }

    // Headers are assumed to be sent on first call to _writeBuffer,
    // i.e. after the first call to write or end.
    let result = this.urlRequest.write(chunk, isLast)

    // The write callback is fired asynchronously to mimic Node.js.
    if (callback) {
      process.nextTick(callback)
    }

    return result
  }

  write (data, encoding, callback) {
    if (this.urlRequest.finished) {
      let error = new Error('Write after end.')
      process.nextTick(writeAfterEndNT, this, error, callback)
      return true
    }

    return this._write(data, encoding, callback, false)
  }

  end (data, encoding, callback) {
    if (this.urlRequest.finished) {
      return false
    }

    if (typeof data === 'function') {
      callback = data
      encoding = null
      data = null
    } else if (typeof encoding === 'function') {
      callback = encoding
      encoding = null
    }

    data = data || ''

    return this._write(data, encoding, callback, true)
  }

  abort () {
    this.urlRequest.cancel()
  }

}

function writeAfterEndNT (self, error, callback) {
  self.emit('error', error)
  if (callback) callback(error)
}

Net.prototype.request = function (options, callback) {
  return new ClientRequest(options, callback)
}

net.ClientRequest = ClientRequest

module.exports = net
