const { app, BrowserWindow, Menu } = require('electron/main')

function createWindow () {
  const win = new BrowserWindow()
  // highlight-start
  const menu = Menu.buildFromTemplate([
    { role: 'copy' },
    { role: 'cut' },
    { role: 'paste' },
    { role: 'selectall' }
  ])
  win.webContents.on('context-menu', (_event, params) => {
    // only show the context menu if the element is editable
    if (params.isEditable) {
      menu.popup()
    }
  })
  // highlight-end
  win.loadFile('index.html')
}

app.whenReady().then(() => {
  createWindow()

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit()
})
