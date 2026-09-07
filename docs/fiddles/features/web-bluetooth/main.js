const { app, BrowserWindow, ipcMain } = require('electron/main')
const path = require('node:path')

let bluetoothPinCallback
let selectBluetoothCallback

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  const pickTestDevice = (device) => {
    if (selectBluetoothCallback && device.deviceName === 'test') {
      selectBluetoothCallback(device.deviceId)
      selectBluetoothCallback = null
    }
  }

  mainWindow.webContents.session.on('select-bluetooth-device', (event, details, callback) => {
    event.preventDefault()
    selectBluetoothCallback = callback
    // Devices already discovered; more arrive through bluetooth-device-added
    // until the callback is called (or the user cancels the request).
    details.deviceList.forEach(pickTestDevice)
  })

  mainWindow.webContents.session.on('bluetooth-device-added', (event, details) => {
    pickTestDevice(details.device)
  })

  ipcMain.on('cancel-bluetooth-request', (event) => {
    if (selectBluetoothCallback) selectBluetoothCallback('')
    selectBluetoothCallback = null
  })

  // Listen for a message from the renderer to get the response for the Bluetooth pairing.
  ipcMain.on('bluetooth-pairing-response', (event, response) => {
    bluetoothPinCallback(response)
  })

  mainWindow.webContents.session.setBluetoothPairingHandler((details, callback) => {
    bluetoothPinCallback = callback
    // Send a message to the renderer to prompt the user to confirm the pairing.
    mainWindow.webContents.send('bluetooth-pairing-request', details)
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
