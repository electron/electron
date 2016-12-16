const {app, BrowserWindow, TouchBar} = require('electron')
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

    const slider = new (TouchBar.Slider)({
      label: 'Slider 123',
      minValue: 50,
      maxValue: 1000,
      initialValue: 300,
      change: (newVal) => {
        console.log('Slider was changed', newVal, typeof newVal)
      }
    });

    global.slider = slider;

    mainWindow.setTouchBar(new TouchBar([
      new (TouchBar.Button)({
        label: 'Hello World!',
        // image: '/path/to/image',
        backgroundColor: 'FF0000',
        labelColor: '0000FF',
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
      new (TouchBar.PopOver)({
        // image: '/path/to/image',
        label: 'foo',
        showCloseButton: true,
        touchBar: new TouchBar([
          new (TouchBar.Group)({
            items: new TouchBar(
              [1, 2, 3].map((i) => new (TouchBar.Button)({
                label: `Button ${i}`,
                click: () => {
                  console.log(`Button ${i} (group) Clicked`)
                }
              }))
            )
          })
        ])
      }),
      slider,
    ]))
  })
}
