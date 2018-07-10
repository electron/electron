const {app} = require('electron')
const net = require('net')

const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-relaunch' : '/tmp/electron-app-relaunch'

process.on('uncaughtException', () => {
  app.exit(1)
})

app.once('ready', () => {
  let lastArg = process.argv[process.argv.length - 1]
  const client = net.connect(socketPath)
  client.once('connect', () => {
    client.end(String(lastArg === '--second'))
  })
  client.once('end', () => {
    app.exit(0)
  })

  if (lastArg !== '--second') {
    app.relaunch({args: process.argv.slice(1).concat('--second')})
  }
})
