// Retrieve information about screen size, displays, cursor position, etc.
//
// For more info, see:
// https://www.electronjs.org/docs/latest/api/screen

const { app, BrowserWindow, screen } = require('electron/main')

let mainWindow = null

app.whenReady().then(() => {
  // Create a window that fills the screen's available work area.
  const primaryDisplay = screen.getPrimaryDisplay()
  const { width, height } = primaryDisplay.workAreaSize

  mainWindow = new BrowserWindow({ width, height })
  mainWindow.loadURL('https://electronjs.org')
})
