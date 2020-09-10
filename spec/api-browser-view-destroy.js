const {app, BrowserWindow, BrowserView} = require('electron')

function createWindow () {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    },
  })
  const view = new BrowserView();
  mainWindow.addBrowserView(view);
  view.webContents.destroy()
  setTimeout(() => view.setBounds({ x: 0, y: 0, width: 300, height: 300 }), 10000) //Asynchronous Test
  //view.setBounds({ x: 0, y: 0, width: 300, height: 300 }) //Synchronous Test
}

app.on('ready', createWindow)

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', function () {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow()
  }
})
