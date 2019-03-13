import * as path from 'path'

import * as electron from 'electron'
import { EventEmitter } from 'events'

const bindings = process.electronBinding('app')
const commandLine = process.electronBinding('command_line')
const { app, App } = bindings

// Only one app object permitted.
export default app

const { deprecate, Menu } = electron

let dockMenu: Electron.Menu | null = null

// App is an EventEmitter.
Object.setPrototypeOf(App.prototype, EventEmitter.prototype)
EventEmitter.call(app as any)

Object.assign(app, {
  // TODO(codebytere): remove in 7.0
  setApplicationMenu (menu: Electron.Menu | null) {
    return Menu.setApplicationMenu(menu)
  },
  // TODO(codebytere): remove in 7.0
  getApplicationMenu () {
    return Menu.getApplicationMenu()
  },
  commandLine: {
    hasSwitch: (theSwitch: string) => commandLine.hasSwitch(String(theSwitch)),
    getSwitchValue: (theSwitch: string) => commandLine.getSwitchValue(String(theSwitch)),
    appendSwitch: (theSwitch: string, value?: string) => commandLine.appendSwitch(String(theSwitch), typeof value === 'undefined' ? value : String(value)),
    appendArgument: (arg: string) => commandLine.appendArgument(String(arg))
  } as Electron.CommandLine,
  enableMixedSandbox () {
    deprecate.log(`'enableMixedSandbox' is deprecated. Mixed-sandbox mode is now enabled by default. You can safely remove the call to enableMixedSandbox().`)
  }
})

app.getFileIcon = deprecate.promisify(app.getFileIcon)

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

if (process.platform === 'darwin') {
  const setDockMenu = app.dock.setMenu
  app.dock.setMenu = (menu) => {
    dockMenu = menu
    setDockMenu(menu)
  }
  app.dock.getMenu = () => dockMenu
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (const name of events) {
  app.on(name as 'login', (event, webContents, ...args: any[]) => {
    webContents.emit(name, event, ...args)
  })
}

// Function Deprecations
app.getFileIcon = deprecate.promisify(app.getFileIcon)

// Property Deprecations
const [
  deprecatedGetter,
  deprecatedSetter
] = deprecate.fnToProperty('accessibilitySupportEnabled', app.isAccessibilitySupportEnabled, app.setAccessibilitySupportEnabled)

app.isAccessibilitySupportEnabled = deprecatedGetter
app.setAccessibilitySupportEnabled = deprecatedSetter

// Wrappers for native classes.
const { DownloadItem } = process.electronBinding('download_item')
Object.setPrototypeOf(DownloadItem.prototype, EventEmitter.prototype)
