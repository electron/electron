const { app, BrowserWindow } = require('electron/main')
const fs = require('node:fs')
const path = require('node:path')

app.disableHardwareAcceleration()

function createWindow () {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      offscreen: true,
      offscreenUseSharedTexture: true // or false
    }
  })

  win.loadURL('https://github.com')
  win.webContents.on('paint', async (event, dirty, image) => {
    if (event.texture) {
      // Import the shared texture handle to your own rendering world.
      // importSharedHandle(event.texture.textureInfo)

      // Example plugin to import shared texture by native addon:
      // https://github.com/electron/electron/tree/main/spec/fixtures/native-addon/osr-gpu

      // You can handle the event in async handler
      await new Promise(resolve => setTimeout(resolve, 50))

      // You can also pass the `textureInfo` to other processes (not `texture`, the `release` function is not passable)
      // You have to release the texture at this process when you are done with it
      event.texture.release()
    } else {
      // texture will be null when `offscreenUseSharedTexture` is false.
      fs.writeFileSync('ex.png', image.toPNG())
    }
  })
  win.webContents.setFrameRate(60)
  console.log(`The screenshot has been successfully saved to ${path.join(process.cwd(), 'ex.png')}`)
}

app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow()
    }
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})
