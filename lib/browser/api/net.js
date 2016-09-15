'use strict'

const binding = process.atomBinding('net')
const {URLRequest} = binding

// Public API.
Object.defineProperties(exports, {
    URLRequest: {
        enumerable: true,
        value: URLRequest
    }
})
 