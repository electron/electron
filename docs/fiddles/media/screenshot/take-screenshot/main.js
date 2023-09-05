const { BrowserWindow, app, screen, ipcMain, desktopCapturer, shell } = require('electron/main')
const fs = require('node:fs').promises
const os = require('node:os')
const path = require('node:path')

let mainWindow = null

function determineScreenShotSize (devicePixelRatio) {
  const screenSize = screen.getPrimaryDisplay().workAreaSize
  const maxDimension = Math.max(screenSize.width, screenSize.height)
  return {
    width: maxDimension * devicePixelRatio,
    height: maxDimension * devicePixelRatio
  }
}

async function takeScreenshot (devicePixelRatio) {
  const thumbSize = determineScreenShotSize(devicePixelRatio)
  const options = { types: ['screen'], thumbnailSize: thumbSize }

  const sources = await desktopCapturer.getSources(options)
  for (const source of sources) {
    const sourceName = source.name.toLowerCase()
    if (sourceName === 'entire screen' || sourceName === 'screen 1') {
      const screenshotPath = path.join(os.tmpdir(), 'screenshot.png')

      await fs.writeFile(screenshotPath, source.thumbnail.toPNG())
      shell.openExternal(`file://${screenshotPath}`)

      return `Saved screenshot to: ${screenshotPath}`
    }
  }
}

ipcMain.handle('take-screenshot', (event, devicePixelRatio) => takeScreenshot(devicePixelRatio))

function createWindow () {
  const windowOptions = {
    width: 600,
    height: 300,
    title: 'Take a Screenshot',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  }

  mainWindow = new BrowserWindow(windowOptions)
  mainWindow.loadFile('index.html')

  mainWindow.on('closed', () => {
    mainWindow = null
  })
}

app.whenReady().then(() => {
  createWindow()
})
