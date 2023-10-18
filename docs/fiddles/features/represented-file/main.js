const { app, BrowserWindow } = require('electron/main')
const os = require('node:os')

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.setRepresentedFilename(os.homedir())
  win.setDocumentEdited(true)

  win.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow()
    }
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})
