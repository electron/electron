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
        click: () => {
          console.log('Hello World Clicked')
        }
      }),
      new (TouchBar.Label)({
        label: 'This is a Label'
      }),
      new (TouchBar.ColorPicker)({
        change: (...args) => {
          console.log('Color was changed', ...args)
        }
      })
    ]))
  })
}
