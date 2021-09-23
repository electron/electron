const {app, BrowserWindow} = require('electron')
const path = require('path')

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600
  })
  
  mainWindow.webContents.session.on('select-serial-port', (event, portList, webContents, callback) => {
    event.preventDefault()
    if (portList && portList.length > 0) {
      callback(portList[0].portId)
    } else {
      callback('') //Could not find any matching devices
    }
  })

  mainWindow.webContents.session.on('serial-port-added', (event, port) => {
    console.log('serial-port-added FIRED WITH', port)
  })

  mainWindow.webContents.session.on('serial-port-removed', (event, port) => {
    console.log('serial-port-removed FIRED WITH', port)
  })

  mainWindow.webContents.session.setPermissionCheckHandler((webContents, permission, requestingOrigin, details) => {
    if (permission === 'serial' && details.securityOrigin === 'file:///') {
      return true
    }
  })

  mainWindow.webContents.session.setDevicePermissionHandler((details) => {
    if (details.deviceType === 'serial' && details.origin === 'file://') {
      return true
    }
  })
  
  mainWindow.loadFile('index.html')

  mainWindow.webContents.openDevTools()
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
