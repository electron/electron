#!/usr/bin/env node

const electron = require('./')

const proc = require('child_process')

const child = proc.spawn(electron, process.argv.slice(2), { stdio: 'inherit' })
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
