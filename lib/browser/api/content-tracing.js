'use strict'
const { deprecate } = require('electron')
const contentTracing = process.atomBinding('content_tracing')

contentTracing.getCategories = deprecate.promisify(contentTracing.getCategories)

module.exports = contentTracing
