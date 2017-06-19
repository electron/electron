const { app, BrowserWindow, ipcMain } = require('electron')
const net = require('net')
const path = require('path')

const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-app-mixed-sandbox' : '/tmp/electron-app-mixed-sandbox'

process.on('uncaughtException', () => {
  app.exit(1)
})

app.once('ready', () => {
  let lastArg = process.argv[process.argv.length - 1]
  const client = net.connect(socketPath)
  client.once('connect', () => {
    if (lastArg === '--enable-mixed-sandbox') {
      ipcMain.on('processArgs', (event, args) => {
        client.end(String(args.indexOf('--no-sandbox') >= 0))
      })
      let window = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(app.getAppPath(), 'electron-app-mixed-sandbox-preload.js'),
          sandbox: false
        }
      })

      window.loadURL('data:,window')
    } else {
      client.end(String(false))
    }
  })
  client.once('end', () => {
    app.exit(0)
  })

  if (lastArg !== '--enable-mixed-sandbox') {
    app.relaunch({ args: process.argv.slice(1).concat('--enable-mixed-sandbox') })
  }
})
