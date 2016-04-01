process.on('uncaughtException', function (error) {
  process.send(error.message)
  process.exit(1)
})

process.on('message', function () {
  setImmediate(function () {
    process.send('ok')
    process.exit(0)
  })
})
