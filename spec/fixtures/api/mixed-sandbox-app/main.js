const { app, BrowserWindow, dialog } = require('electron')
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
      dialog.showErrorBox('connected', app.getAppPath())
      let window = new BrowserWindow({
        show: true,
        webPreferences: {
         preload: path.join(app.getAppPath(), 'electron-app-mixed-sandbox-preload.js'),
         sandbox: false
        }
      })
      
      window.loadURL('data:,window')
      let argv = 'test'
      window.webContents.once('did-finish-load', () => {
        dialog.showErrorBox('finished-load', 'finished')
        window.webContents.executeJavaScript('window.argv', false, (result) => {
          dialog.showErrorBox('hello', result)
          client.end(String(result))
        })
      })
      // window.webContents.openDevTools()
      // window.webContents.executeJavaScript('window.argv', true).then((result) => {
      //   dialog.showErrorBox('hello', result)
      //   client.end(String(argv))
      // })
      
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
