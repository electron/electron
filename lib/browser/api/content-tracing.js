'use strict'
const { deprecate } = require('electron')
const contentTracing = process.atomBinding('content_tracing')

contentTracing.startRecording = deprecate.promisify(contentTracing.startRecording)
contentTracing.stopRecording = deprecate.promisify(contentTracing.stopRecording)
contentTracing.getCategories = deprecate.promisify(contentTracing.getCategories)

module.exports = contentTracing
