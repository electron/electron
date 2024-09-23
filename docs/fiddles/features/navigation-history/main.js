const { app, BrowserWindow, BrowserView, ipcMain } = require('electron')
const path = require('path')

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 1000,
    height: 800,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true
    }
  })

  mainWindow.loadFile('index.html')

  const view = new BrowserView()
  mainWindow.setBrowserView(view)
  view.setBounds({ x: 0, y: 100, width: 1000, height: 800 })
  view.setAutoResize({ width: true, height: true })

  const navigationHistory = view.webContents.navigationHistory
  ipcMain.handle('nav:back', () =>
    navigationHistory.goBack()
  )

  ipcMain.handle('nav:forward', () => {
    navigationHistory.goForward()
  })

  ipcMain.handle('nav:canGoBack', () => navigationHistory.canGoBack())
  ipcMain.handle('nav:canGoForward', () => navigationHistory.canGoForward())
  ipcMain.handle('nav:loadURL', (_, url) =>
    view.webContents.loadURL(url)
  )
  ipcMain.handle('nav:getCurrentURL', () => view.webContents.getURL())
  ipcMain.handle('nav:getHistory', () => {
    return navigationHistory.getAllEntries()
  })

  view.webContents.on('did-navigate', () => {
    mainWindow.webContents.send('nav:updated')
  })

  view.webContents.on('did-navigate-in-page', () => {
    mainWindow.webContents.send('nav:updated')
  })
}

app.whenReady().then(createWindow)

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow()
})
