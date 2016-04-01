process.on('uncaughtException', function (err) {
  process.send(err.message)
})

require('runas')
process.send('ok')
