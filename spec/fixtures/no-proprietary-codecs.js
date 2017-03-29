// Verifies that Electron cannot play a video that uses proprietary codecs
//
// This application should be run with the ffmpeg that does not include
// proprietary codecs to ensure Electron uses it instead of the version
// that does include proprietary codecs.

const {app, BrowserWindow, ipcMain} = require('electron')
const path = require('path')
const url = require('url')

const MEDIA_ERR_SRC_NOT_SUPPORTED = 4

let window

app.once('ready', () => {
  window = new BrowserWindow({
    show: false
  })

  window.loadURL(url.format({
    protocol: 'file',
    slashed: true,
    pathname: path.resolve(__dirname, 'asar', 'video.asar', 'index.html')
  }))

  ipcMain.on('asar-video', function (event, message, error) {
    if (message === 'ended') {
      console.log('Video played, proprietary codecs are included')
      app.exit(1)
      return
    }

    if (message === 'error' && error === MEDIA_ERR_SRC_NOT_SUPPORTED) {
      console.log('Video format not supported, proprietary codecs are not included')
      app.exit(0)
      return
    }

    console.log(`Unexpected error: ${error}`)
    app.exit(1)
  })
})
