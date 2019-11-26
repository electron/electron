import { app, dialog, BrowserWindow, shell, ipcMain } from 'electron'
import * as path from 'path'

let mainWindow: BrowserWindow | null = null

app.allowRendererProcessReuse = true

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

function decorateURL (url: string) {
  // safely add `?utm_source=default_app
  const parsedUrl = new URL(url)
  parsedUrl.searchParams.append('utm_source', 'default_app')
  return parsedUrl.toString()
}

// Find the shortest path to the electron binary
const absoluteElectronPath = process.execPath
const relativeElectronPath = path.relative(process.cwd(), absoluteElectronPath)
const electronPath = absoluteElectronPath.length < relativeElectronPath.length
  ? absoluteElectronPath
  : relativeElectronPath

const indexPath = path.resolve(app.getAppPath(), 'index.html')

function isTrustedSender (webContents: Electron.WebContents) {
  if (webContents !== (mainWindow && mainWindow.webContents)) {
    return false
  }

  const parsedUrl = new URL(webContents.getURL())
  const urlPath = process.platform === 'win32'
    // Strip the prefixed "/" that occurs on windows
    ? path.resolve(parsedUrl.pathname.substr(1))
    : parsedUrl.pathname
  return parsedUrl.protocol === 'file:' && urlPath === indexPath
}

ipcMain.handle('bootstrap', (event) => {
  return isTrustedSender(event.sender) ? electronPath : null
})

async function createWindow () {
  await app.whenReady()

  const options: Electron.BrowserWindowConstructorOptions = {
    width: 900,
    height: 600,
    autoHideMenuBar: true,
    backgroundColor: '#FFFFFF',
    webPreferences: {
      preload: path.resolve(__dirname, 'preload.js'),
      contextIsolation: true,
      sandbox: true,
      enableRemoteModule: false
    },
    useContentSize: true,
    show: false
  }

  if (process.platform === 'linux') {
    options.icon = path.join(__dirname, 'icon.png')
  }

  mainWindow = new BrowserWindow(options)
  mainWindow.on('ready-to-show', () => mainWindow!.show())

  mainWindow.webContents.on('new-window', (event, url) => {
    event.preventDefault()
    shell.openExternal(decorateURL(url))
  })

  mainWindow.webContents.session.setPermissionRequestHandler((webContents, permission, done) => {
    const parsedUrl = new URL(webContents.getURL())

    const options: Electron.MessageBoxOptions = {
      title: 'Permission Request',
      message: `Allow '${parsedUrl.origin}' to access '${permission}'?`,
      buttons: ['OK', 'Cancel'],
      cancelId: 1
    }

    dialog.showMessageBox(mainWindow!, options).then(({ response }) => {
      done(response === 0)
    })
  })

  return mainWindow
}

export const loadURL = async (appUrl: string) => {
  mainWindow = await createWindow()
  mainWindow.loadURL(appUrl)
  mainWindow.focus()
}

export const loadFile = async (appPath: string) => {
  mainWindow = await createWindow()
  mainWindow.loadFile(appPath)
  mainWindow.focus()
}
