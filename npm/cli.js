#!/usr/bin/env node

var electron = require('./')

var proc = require('child_process')

var options = parseInt(process.versions.node) >= 11 ? {stdio: 'inherit', windowsHide: false}: {stdio: 'inherit'}
var child = proc.spawn(electron, process.argv.slice(2), options)
child.on('close', function (code) {
  process.exit(code)
})

const handleTerminationSignal = function (signal) {
  process.on(signal, function signalHandler () {
    if (!child.killed) {
      child.kill(signal)
    }
  })
}

handleTerminationSignal('SIGINT')
handleTerminationSignal('SIGTERM')
