'use strict'
const { deprecate } = require('electron')
const contentTracing = process.atomBinding('content_tracing')

contentTracing.getCategories = deprecate.promisify(contentTracing.getCategories)
contentTracing.startRecording = deprecate.promisify(contentTracing.startRecording)
contentTracing.stopRecording = deprecate.promisify(contentTracing.stopRecording)
contentTracing.getTraceBufferUsage = deprecate.promisifyMultiArg(
  contentTracing.getTraceBufferUsage,
  (value) => [value.paths, value.bookmarks]
)

module.exports = contentTracing
