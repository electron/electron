import { app, BrowserWindow, BrowserWindowConstructorOptions } from 'electron'
import * as path from 'path'

let mainWindow: BrowserWindow | null = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

export const load = async (appUrl: string) => {
  await app.whenReady()

  const options: BrowserWindowConstructorOptions = {
    width: 900,
    height: 600,
    autoHideMenuBar: true,
    backgroundColor: '#FFFFFF',
    webPreferences: {
      contextIsolation: true,
      preload: path.resolve(__dirname, 'renderer.js'),
      webviewTag: false
    },
    useContentSize: true,
    show: false
  }

  if (process.platform === 'linux') {
    options.icon = path.join(__dirname, 'icon.png')
  }

  mainWindow = new BrowserWindow(options)

  mainWindow.on('ready-to-show', () => mainWindow!.show())

  mainWindow.loadURL(appUrl)
  mainWindow.focus()
}
