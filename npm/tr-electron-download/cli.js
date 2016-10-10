#!/usr/bin/env node
var download = require('./')
var minimist = require('minimist')
var opts = minimist(process.argv.slice(2))

if (opts['strict-ssl'] === false) {
  opts.strictSSL = false
}

download(opts, function (err, zipPath) {
  if (err) throw err
  console.log('Downloaded zip:', zipPath)
  process.exit(0)
})
