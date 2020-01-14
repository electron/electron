'use strict'

const url = require('url')
const { EventEmitter } = require('events')
const { Readable, Writable } = require('stream')
const { app } = require('electron')
const { Session } = process.electronBinding('session')
const { net, Net, _isValidHeaderName, _isValidHeaderValue } = process.electronBinding('net')
const { URLLoader } = net

Object.setPrototypeOf(URLLoader.prototype, EventEmitter.prototype)

const kSupportedProtocols = new Set(['http:', 'https:'])

// set of headers that Node.js discards duplicates for
// see https://nodejs.org/api/http.html#http_message_headers
const discardableDuplicateHeaders = new Set([
  'content-type',
  'content-length',
  'user-agent',
  'referer',
  'host',
  'authorization',
  'proxy-authorization',
  'if-modified-since',
  'if-unmodified-since',
  'from',
  'location',
  'max-forwards',
  'retry-after',
  'etag',
  'last-modified',
  'server',
  'age',
  'expires'
])

class IncomingMessage extends Readable {
  constructor (responseHead) {
    super()
    this._shouldPush = false
    this._data = []
    this._responseHead = responseHead
  }

  get statusCode () {
    return this._responseHead.statusCode
  }

  get statusMessage () {
    return this._responseHead.statusMessage
  }

  get headers () {
    const filteredHeaders = {}
    const { rawHeaders } = this._responseHead
    rawHeaders.forEach(header => {
      if (Object.prototype.hasOwnProperty.call(filteredHeaders, header.key) &&
          discardableDuplicateHeaders.has(header.key)) {
        // do nothing with discardable duplicate headers
      } else {
        if (header.key === 'set-cookie') {
          // keep set-cookie as an array per Node.js rules
          // see https://nodejs.org/api/http.html#http_message_headers
          if (Object.prototype.hasOwnProperty.call(filteredHeaders, header.key)) {
            filteredHeaders[header.key].push(header.value)
          } else {
            filteredHeaders[header.key] = [header.value]
          }
        } else {
          // for non-cookie headers, the values are joined together with ', '
          if (Object.prototype.hasOwnProperty.call(filteredHeaders, header.key)) {
            filteredHeaders[header.key] += `, ${header.value}`
          } else {
            filteredHeaders[header.key] = header.value
          }
        }
      }
    })
    return filteredHeaders
  }

  get httpVersion () {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`
  }

  get httpVersionMajor () {
    return this._responseHead.httpVersion.major
  }

  get httpVersionMinor () {
    return this._responseHead.httpVersion.minor
  }

  get rawTrailers () {
    throw new Error('HTTP trailers are not supported')
  }

  get trailers () {
    throw new Error('HTTP trailers are not supported')
  }

  _storeInternalData (chunk) {
    this._data.push(chunk)
    this._pushInternalData()
  }

  _pushInternalData () {
    while (this._shouldPush && this._data.length > 0) {
      const chunk = this._data.shift()
      this._shouldPush = this.push(chunk)
    }
  }

  _read () {
    this._shouldPush = true
    this._pushInternalData()
  }
}

/** Writable stream that buffers up everything written to it. */
class SlurpStream extends Writable {
  constructor () {
    super()
    this._data = Buffer.alloc(0)
  }
  _write (chunk, encoding, callback) {
    this._data = Buffer.concat([this._data, chunk])
    callback()
  }
  data () { return this._data }
}

class ChunkedBodyStream extends Writable {
  constructor (clientRequest) {
    super()
    this._clientRequest = clientRequest
  }

  _write (chunk, encoding, callback) {
    if (this._downstream) {
      this._downstream.write(chunk).then(callback, callback)
    } else {
      // the contract of _write is that we won't be called again until we call
      // the callback, so we're good to just save a single chunk.
      this._pendingChunk = chunk
      this._pendingCallback = callback

      // The first write to a chunked body stream begins the request.
      this._clientRequest._startRequest()
    }
  }

  _final (callback) {
    this._downstream.done()
    callback()
  }

  startReading (pipe) {
    if (this._downstream) {
      throw new Error('two startReading calls???')
    }
    this._downstream = pipe
    if (this._pendingChunk) {
      const doneWriting = (maybeError) => {
        const cb = this._pendingCallback
        delete this._pendingCallback
        delete this._pendingChunk
        cb(maybeError)
      }
      this._downstream.write(this._pendingChunk).then(doneWriting, doneWriting)
    }
  }
}

function parseOptions (options) {
  if (typeof options === 'string') {
    options = url.parse(options)
  } else {
    options = { ...options }
  }

  const method = (options.method || 'GET').toUpperCase()
  let urlStr = options.url

  if (!urlStr) {
    const urlObj = {}
    const protocol = options.protocol || 'http:'
    if (!kSupportedProtocols.has(protocol)) {
      throw new Error('Protocol "' + protocol + '" not supported')
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
      throw new TypeError('Request path contains unescaped characters')
    }
    const pathObj = url.parse(options.path || '/')
    urlObj.pathname = pathObj.pathname
    urlObj.search = pathObj.search
    urlObj.hash = pathObj.hash
    urlStr = url.format(urlObj)
  }

  const redirectPolicy = options.redirect || 'follow'
  if (!['follow', 'error', 'manual'].includes(redirectPolicy)) {
    throw new Error('redirect mode should be one of follow, error or manual')
  }

  if (options.headers != null && typeof options.headers !== 'object') {
    throw new TypeError('headers must be an object')
  }

  const urlLoaderOptions = {
    method: method,
    url: urlStr,
    redirectPolicy,
    extraHeaders: options.headers || {}
  }
  for (const [name, value] of Object.entries(urlLoaderOptions.extraHeaders)) {
    if (!_isValidHeaderName(name)) {
      throw new Error(`Invalid header name: '${name}'`)
    }
    if (!_isValidHeaderValue(value.toString())) {
      throw new Error(`Invalid value for header '${name}': '${value}'`)
    }
  }
  if (options.session) {
    if (options.session instanceof Session) {
      urlLoaderOptions.session = options.session
    } else {
      throw new TypeError('`session` should be an instance of the Session class')
    }
  } else if (options.partition) {
    if (typeof options.partition === 'string') {
      urlLoaderOptions.partition = options.partition
    } else {
      throw new TypeError('`partition` should be a string')
    }
  }
  return urlLoaderOptions
}

class ClientRequest extends Writable {
  constructor (options, callback) {
    super({ autoDestroy: true })

    if (!app.isReady()) {
      throw new Error('net module can only be used after app is ready')
    }

    if (callback) {
      this.once('response', callback)
    }

    const { redirectPolicy, ...urlLoaderOptions } = parseOptions(options)
    this._urlLoaderOptions = urlLoaderOptions
    this._redirectPolicy = redirectPolicy
    this._started = false
  }

  set chunkedEncoding (value) {
    if (this._started) {
      throw new Error('chunkedEncoding can only be set before the request is started')
    }
    if (typeof this._chunkedEncoding !== 'undefined') {
      throw new Error('chunkedEncoding can only be set once')
    }
    this._chunkedEncoding = !!value
    if (this._chunkedEncoding) {
      this._body = new ChunkedBodyStream(this)
      this._urlLoaderOptions.body = (pipe) => {
        this._body.startReading(pipe)
      }
    }
  }

  setHeader (name, value) {
    if (typeof name !== 'string') {
      throw new TypeError('`name` should be a string in setHeader(name, value)')
    }
    if (value == null) {
      throw new Error('`value` required in setHeader("' + name + '", value)')
    }
    if (this._started || this._firstWrite) {
      throw new Error('Can\'t set headers after they are sent')
    }
    if (!_isValidHeaderName(name)) {
      throw new Error(`Invalid header name: '${name}'`)
    }
    if (!_isValidHeaderValue(value.toString())) {
      throw new Error(`Invalid value for header '${name}': '${value}'`)
    }

    const key = name.toLowerCase()
    this._urlLoaderOptions.extraHeaders[key] = value
  }

  getHeader (name) {
    if (name == null) {
      throw new Error('`name` is required for getHeader(name)')
    }

    const key = name.toLowerCase()
    return this._urlLoaderOptions.extraHeaders[key]
  }

  removeHeader (name) {
    if (name == null) {
      throw new Error('`name` is required for removeHeader(name)')
    }

    if (this._started || this._firstWrite) {
      throw new Error('Can\'t remove headers after they are sent')
    }

    const key = name.toLowerCase()
    delete this._urlLoaderOptions.extraHeaders[key]
  }

  _write (chunk, encoding, callback) {
    this._firstWrite = true
    if (!this._body) {
      this._body = new SlurpStream()
      this._body.on('finish', () => {
        this._urlLoaderOptions.body = this._body.data()
        this._startRequest()
      })
    }
    // TODO: is this the right way to forward to another stream?
    this._body.write(chunk, encoding, callback)
  }

  _final (callback) {
    if (this._body) {
      // TODO: is this the right way to forward to another stream?
      this._body.end(callback)
    } else {
      // end() called without a body, go ahead and start the request
      this._startRequest()
      callback()
    }
  }

  _startRequest () {
    this._started = true
    const stringifyValues = (obj) => {
      const ret = {}
      for (const k in obj) {
        ret[k] = obj[k].toString()
      }
      return ret
    }
    const opts = { ...this._urlLoaderOptions, extraHeaders: stringifyValues(this._urlLoaderOptions.extraHeaders) }
    this._urlLoader = new URLLoader(opts)
    this._urlLoader.on('response-started', (event, finalUrl, responseHead) => {
      const response = this._response = new IncomingMessage(responseHead)
      this.emit('response', response)
    })
    this._urlLoader.on('data', (event, data) => {
      this._response._storeInternalData(Buffer.from(data))
    })
    this._urlLoader.on('complete', () => {
      if (this._response) { this._response._storeInternalData(null) }
    })
    this._urlLoader.on('error', (event, netErrorString) => {
      const error = new Error(netErrorString)
      if (this._response) this._response.destroy(error)
      this._die(error)
    })

    this._urlLoader.on('login', (event, authInfo, callback) => {
      const handled = this.emit('login', authInfo, callback)
      if (!handled) {
        // If there were no listeners, cancel the authentication request.
        callback()
      }
    })

    this._urlLoader.on('redirect', (event, redirectInfo, headers) => {
      const { statusCode, newMethod, newUrl } = redirectInfo
      if (this._redirectPolicy === 'error') {
        this._die(new Error(`Attempted to redirect, but redirect policy was 'error'`))
      } else if (this._redirectPolicy === 'manual') {
        let _followRedirect = false
        this._followRedirectCb = () => { _followRedirect = true }
        try {
          this.emit('redirect', statusCode, newMethod, newUrl, headers)
        } finally {
          this._followRedirectCb = null
          if (!_followRedirect && !this._aborted) {
            this._die(new Error('Redirect was cancelled'))
          }
        }
      } else if (this._redirectPolicy === 'follow') {
        // Calling followRedirect() when the redirect policy is 'follow' is
        // allowed but does nothing. (Perhaps it should throw an error
        // though...? Since the redirect will happen regardless.)
        try {
          this._followRedirectCb = () => {}
          this.emit('redirect', statusCode, newMethod, newUrl, headers)
        } finally {
          this._followRedirectCb = null
        }
      } else {
        this._die(new Error(`Unexpected redirect policy '${this._redirectPolicy}'`))
      }
    })

    this._urlLoader.on('upload-progress', (event, position, total) => {
      this._uploadProgress = { active: true, started: true, current: position, total }
      this.emit('upload-progress', position, total) // Undocumented, for now
    })

    this._urlLoader.on('download-progress', (event, current) => {
      if (this._response) {
        this._response.emit('download-progress', current) // Undocumented, for now
      }
    })
  }

  followRedirect () {
    if (this._followRedirectCb) {
      this._followRedirectCb()
    } else {
      throw new Error('followRedirect() called, but was not waiting for a redirect')
    }
  }

  abort () {
    if (!this._aborted) {
      process.nextTick(() => { this.emit('abort') })
    }
    this._aborted = true
    this._die()
  }

  _die (err) {
    this.destroy(err)
    if (this._urlLoader) {
      this._urlLoader.cancel()
      if (this._response) this._response.destroy(err)
    }
  }

  getUploadProgress () {
    return this._uploadProgress ? { ...this._uploadProgress } : { active: false }
  }
}

Net.prototype.request = function (options, callback) {
  return new ClientRequest(options, callback)
}

net.ClientRequest = ClientRequest

module.exports = net
