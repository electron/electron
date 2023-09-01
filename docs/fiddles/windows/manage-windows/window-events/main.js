// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain, shell } = require('electron/main')
const path = require('node:path')

function createWindow () {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  // and load the index.html of the app.
  mainWindow.loadFile('index.html')

  // Open external links in the default browser
  mainWindow.webContents.on('will-navigate', (event, url) => {
    event.preventDefault()
    shell.openExternal(url)
  })

  let demoWindow
  ipcMain.on('show-demo-window', () => {
    if (demoWindow) {
      demoWindow.focus()
      return
    }
    demoWindow = new BrowserWindow({ width: 600, height: 400 })
    demoWindow.loadURL('https://electronjs.org')
    demoWindow.on('close', () => {
      demoWindow = undefined
      mainWindow.webContents.send('window-close')
    })
    demoWindow.on('focus', () => {
      mainWindow.webContents.send('window-focus')
    })
    demoWindow.on('blur', () => {
      mainWindow.webContents.send('window-blur')
    })
  })

  ipcMain.on('focus-demo-window', () => {
    if (demoWindow) demoWindow.focus()
  })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  createWindow()

  app.on('activate', function () {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
