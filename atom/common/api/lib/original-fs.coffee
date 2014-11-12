fs = require 'fs'

copied = {}
copied[k] = v for k, v of fs

module.exports = copied
