'use strict'
const { deprecate } = require('electron')
const { markPromisified } = require('@electron/internal/common/promise-utils')

module.exports = process.electronBinding('content_tracing')

// Mark promisifed APIs
markPromisified(module.exports.getCategories)
markPromisified(module.exports.startRecording)
markPromisified(module.exports.stopRecording)
markPromisified(module.exports.getTraceBufferUsage)
