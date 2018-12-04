'use strict'

const { markPromisified } = require('@electron/internal/common/promise-utils')

module.exports = process.electronBinding('shell')

// Mark promisifed APIs
markPromisified(module.exports.openExternal)
