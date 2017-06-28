const {app, BrowserWindow, ipcMain} = require('electron')
const net = require('net')
const path = require('path')

process.on('uncaughtException', () => {
  app.exit(1)
})

if (!process.argv.includes('--enable-mixed-sandbox')) {
  app.enableMixedSandbox()
}

let sandboxWindow
let noSandboxWindow

app.once('ready', () => {
  sandboxWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(app.getAppPath(), 'electron-app-mixed-sandbox-preload.js'),
      sandbox: true
    }
  })
  sandboxWindow.loadURL('about:blank')

  noSandboxWindow = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(app.getAppPath(), 'electron-app-mixed-sandbox-preload.js'),
      sandbox: false
    }
  })
  noSandboxWindow.loadURL('about:blank')

  const argv = {
    sandbox: null,
    noSandbox: null
  }

  ipcMain.on('argv', (event, value) => {
    if (event.sender === sandboxWindow.webContents) {
      argv.sandbox = value
    } else if (event.sender === noSandboxWindow.webContents) {
      argv.noSandbox = value
    }

    if (argv.sandbox != null && argv.noSandbox != null) {
      const socketPath = process.platform === 'win32' ? '\\\\.\\pipe\\electron-mixed-sandbox' : '/tmp/electron-mixed-sandbox'
      const client = net.connect(socketPath)
      client.once('connect', () => {
        client.end(JSON.stringify(argv))
      })
      client.once('end', () => {
        app.exit(0)
      })
    }
  })
})
