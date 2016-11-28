const {app, BrowserWindow,TouchBar} = require('electron')
const path = require('path')

let mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

exports.load = (appUrl) => {
  app.on('ready', () => {
    const options = {
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true
    }
    if (process.platform === 'linux') {
      options.icon = path.join(__dirname, 'icon.png')
    }

    mainWindow = new BrowserWindow(options)
    mainWindow.loadURL(appUrl)
    mainWindow.focus()

    mainWindow.setTouchBar(new TouchBar([
      new (TouchBar.Button)({
        label: 'Hello World!',
        backgroundColor: "DDDDDD",
        labelColor: "000000",
        click: () => {
          console.log('Hello World Clicked')
        }
      }),
      new (TouchBar.Label)({
        label: 'This is a Label'
      }),
      new (TouchBar.ColorPicker)({
        change: (newColor) => {
          console.log('Color was changed', newColor)
        }
      }),
      new (TouchBar.Slider)({
        label: 'Slider 123',
        minValue: 50,
        maxValue: 1000,
        change: (newVal) => {
          console.log('Slider was changed', newVal, typeof newVal)
        }
      }),
    ]))
  })
}
