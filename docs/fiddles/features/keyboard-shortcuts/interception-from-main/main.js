const { app, BrowserWindow } = require('electron')

app.whenReady().then(() => {
  const win = new BrowserWindow({ width: 800, height: 600 })

  win.loadFile('index.html')
  
  let preventNextKeyUp = false

  win.webContents.on('before-input-event', (event, input) => {
    if (input.control && input.key.toLowerCase() === 'i') {
      console.log('Pressed Control+I')
      event.preventDefault()
      preventNextKeyUp = true
    }
  })
  

 win.webContents.on('keyup', (event) => {
    if (preventNextKeyUp) {
      preventNextKeyUp = false
      event.preventDefault()
    }
  })
})