import * as path from 'path'

import { deprecate, Menu } from 'electron'
import { EventEmitter } from 'events'

const bindings = process.electronBinding('app')
const commandLine = process.electronBinding('command_line')
const { app, App } = bindings

// Only one app object permitted.
export default app

let dockMenu: Electron.Menu | null = null

// App is an EventEmitter.
Object.setPrototypeOf(App.prototype, EventEmitter.prototype)
EventEmitter.call(app as any)

Object.assign(app, {
  commandLine: {
    hasSwitch: (theSwitch: string) => commandLine.hasSwitch(String(theSwitch)),
    getSwitchValue: (theSwitch: string) => commandLine.getSwitchValue(String(theSwitch)),
    appendSwitch: (theSwitch: string, value?: string) => commandLine.appendSwitch(String(theSwitch), typeof value === 'undefined' ? value : String(value)),
    appendArgument: (arg: string) => commandLine.appendArgument(String(arg))
  } as Electron.CommandLine
})

// we define this here because it'd be overly complicated to
// do in native land
Object.defineProperty(app, 'applicationMenu', {
  get () {
    return Menu.getApplicationMenu()
  },
  set (menu: Electron.Menu | null) {
    return Menu.setApplicationMenu(menu)
  }
})

app.isPackaged = (() => {
  const execFile = path.basename(process.execPath).toLowerCase()
  if (process.platform === 'win32') {
    return execFile !== 'electron.exe'
  }
  return execFile !== 'electron'
})()

app._setDefaultAppPaths = (packagePath) => {
  // Set the user path according to application's name.
  app.setPath('userData', path.join(app.getPath('appData'), app.name!))
  app.setPath('userCache', path.join(app.getPath('cache'), app.name!))
  app.setAppPath(packagePath)

  // Add support for --user-data-dir=
  if (app.commandLine.hasSwitch('user-data-dir')) {
    const userDataDir = app.commandLine.getSwitchValue('user-data-dir')
    if (path.isAbsolute(userDataDir)) app.setPath('userData', userDataDir)
  }
}

if (process.platform === 'darwin') {
  const setDockMenu = app.dock!.setMenu
  app.dock!.setMenu = (menu) => {
    dockMenu = menu
    setDockMenu(menu)
  }
  app.dock!.getMenu = () => dockMenu
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (const name of events) {
  app.on(name as 'login', (event, webContents, ...args: any[]) => {
    webContents.emit(name, event, ...args)
  })
}

// Property Deprecations
deprecate.fnToProperty(App.prototype, 'accessibilitySupportEnabled', '_isAccessibilitySupportEnabled', '_setAccessibilitySupportEnabled')
deprecate.fnToProperty(App.prototype, 'badgeCount', '_getBadgeCount', '_setBadgeCount')
deprecate.fnToProperty(App.prototype, 'name', '_getName', '_setName')

// Wrappers for native classes.
const { DownloadItem } = process.electronBinding('download_item')
Object.setPrototypeOf(DownloadItem.prototype, EventEmitter.prototype)
