const {app, BrowserWindow} = require('electron')
const path = require('path')

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600
  })

  mainWindow.webContents.on('select-bluetooth-device', (event, deviceList, callback) => {
    event.preventDefault()
    if (deviceList && deviceList.length > 0) {
      callback(deviceList[0].deviceId)
    } 
  })

  mainWindow.webContents.session.setBluetoothPairingHandler((details, callback) => {
    const response = {}
  
    switch (details.pairingKind) {
      case 'confirm': {
        response.confirmed = confirm(`Do you want to connect to device ${details.deviceId}`)
        break
      }
      case 'confirmPin': {
        response.confirmed = confirm(`Does the pin ${details.pin} match the pin displayed on device ${details.deviceId}?`)
        break
      }
      case 'providePin': {
        const pin = prompt(`Please provide a pin for ${details.deviceId}`)
        if (pin) {
          response.pin = pin
          response.confirmed = true
        } else {
          response.confirmed = false
        }
      }
    }
    callback(response)
  })  

  mainWindow.loadFile('index.html')
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
