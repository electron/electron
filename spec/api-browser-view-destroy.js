// Modules to control application life and create native browser window
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
  setTimeout(() => view.setBounds({ x: 0, y: 0, width: 300, height: 300 }), 10000) //Asynchronous Test Failure
  //view.setBounds({ x: 0, y: 0, width: 300, height: 300 }) //Synchronous Test Failure
  
  // and load the index.html of the app.
  mainWindow.loadFile('index.html')
  // Open the DevTools.
  mainWindow.webContents.openDevTools()
}
// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)
// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})
app.on('activate', function () {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow()
  }
})
// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.