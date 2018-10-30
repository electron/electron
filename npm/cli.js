#!/usr/bin/env node

var electron = require('./')

var proc = require('child_process')

var child = proc.spawn(electron, process.argv.slice(2), { stdio: 'inherit', windowsHide: false })
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
