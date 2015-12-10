var app = require('electron').app

app.on('ready', function () {
  app.exit(123)
})

process.on('exit', function (code) {
  console.log('Exit event with code: ' + code)
})
