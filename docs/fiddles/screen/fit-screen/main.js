// Retrieve information about screen size, displays, cursor position, etc.
//
// For more info, see:
// https://electronjs.org/docs/api/screen

const { app, BrowserWindow } = require('electron')

let mainWindow = null

app.whenReady().then(() => {
  // We cannot require the screen module until the app is ready.
  const { screen } = require('electron')

  // Create a window that fills the screen's available work area.
  const primaryDisplay = screen.getPrimaryDisplay()
  const { width, height } = primaryDisplay.workAreaSize

  mainWindow = new BrowserWindow({ width, height })
  mainWindow.loadURL('https://electronjs.org')
})
