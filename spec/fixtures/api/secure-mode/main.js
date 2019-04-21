const { app, BrowserWindow, webContents, ipcMain, session } = require('electron')
const path = require('path')
const { emittedOnce } = require('../../../events-helpers')

if (app.commandLine.hasSwitch('enable-secure-mode')) {
  app.enableSecureMode({
    configurableSandbox: true,
    configurableContextIsolation: true,
    configurableNativeWindowOpen: true,
    configurableRemoteModule: true
  })
}

process.on('uncaughtException', (error) => {
  console.error(error)
  app.exit(1)
})

app.once('ready', async () => {
  session.defaultSession.setPreloads([
    path.join(__dirname, 'preload-base.js')
  ])

  let win = new BrowserWindow({
    show: false,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      enableRemoteModule: true,
      webviewTag: true
    }
  })

  win.webContents.once('preload-error', (event, preloadPath, error) => {
    console.error(error)
    app.exit(1)
  })

  const promise = emittedOnce(ipcMain, 'result')
  win.loadURL('about:blank')

  const [, result] = await promise

  await win.webContents.executeJavaScript(`window.open()`)
  const blocksNewWindow = BrowserWindow.getAllWindows().length === 1

  await win.webContents.executeJavaScript(`document.createElement('webview').src = 'about:blank'`)
  const blocksWillAttachWebView = webContents.getAllWebContents().length === 1

  Object.assign(result, {
    blocksNewWindow,
    blocksWillAttachWebView
  })

  win = new BrowserWindow({
    show: false,
    webPreferences: {
      sandbox: false,
      contextIsolation: false,
      nodeIntegration: true,
      nodeIntegrationInWorker: true
    }
  })

  const promise2 = emittedOnce(ipcMain, 'result')
  win.loadFile('worker.html')

  const [, moreResult] = await promise2
  Object.assign(result, moreResult)

  console.log(JSON.stringify(result))
  app.exit(0)
})
