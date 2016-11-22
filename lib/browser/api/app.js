'use strict'

const url = require('url')

const bindings = process.atomBinding('app')
const path = require('path')
const {app, App} = bindings

// Only one app object permitted.
module.exports = app

const electron = require('electron')
const {deprecate, Menu} = electron
const {EventEmitter} = require('events')

let dockMenu = null
let readyPromise = null

// App is an EventEmitter.
Object.setPrototypeOf(App.prototype, EventEmitter.prototype)
EventEmitter.call(app)

Object.assign(app, {
  whenReady () {
    if (readyPromise !== null) {
      return readyPromise
    }

    if (app.isReady()) {
      readyPromise = Promise.resolve()
    } else {
      readyPromise = new Promise(resolve => {
        // XXX(alexeykuzmin): Explicitly ignore any data
        // passed to the event handler to avoid memory leaks.
        app.once('ready', () => resolve())
      })
    }

    return readyPromise
  },
  setApplicationMenu (menu) {
    return Menu.setApplicationMenu(menu)
  },
  getApplicationMenu () {
    return Menu.getApplicationMenu()
  },
  commandLine: {
    appendSwitch (...args) {
      const castedArgs = args.map((arg) => {
        return typeof arg !== 'string' ? `${arg}` : arg
      })
      return bindings.appendSwitch(...castedArgs)
    },
    appendArgument (...args) {
      const castedArgs = args.map((arg) => {
        return typeof arg !== 'string' ? `${arg}` : arg
      })
      return bindings.appendArgument(...castedArgs)
    }
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
  app.dock = {
    bounce (type = 'informational') {
      return bindings.dockBounce(type)
    },
    cancelBounce: bindings.dockCancelBounce,
    downloadFinished: bindings.dockDownloadFinished,
    setBadge: bindings.dockSetBadgeText,
    getBadge: bindings.dockGetBadgeText,
    hide: bindings.dockHide,
    show: bindings.dockShow,
    isVisible: bindings.dockIsVisible,
    setMenu (menu) {
      dockMenu = menu
      bindings.dockSetMenu(menu)
    },
    getMenu () {
      return dockMenu
    },
    setIcon: bindings.dockSetIcon
  }
}

if (process.platform === 'linux') {
  app.launcher = {
    setBadgeCount: bindings.unityLauncherSetBadgeCount,
    getBadgeCount: bindings.unityLauncherGetBadgeCount,
    isCounterBadgeAvailable: bindings.unityLauncherAvailable,
    isUnityRunning: bindings.unityLauncherAvailable
  }
}

app.allowNTLMCredentialsForAllDomains = function (allow) {
  if (!process.noDeprecations) {
    deprecate.warn('app.allowNTLMCredentialsForAllDomains', 'session.allowNTLMCredentialsForDomains')
  }
  let domains = allow ? '*' : ''
  if (!this.isReady()) {
    this.commandLine.appendSwitch('auth-server-whitelist', domains)
  } else {
    electron.session.defaultSession.allowNTLMCredentialsForDomains(domains)
  }
}

const makeSingleInstance = app.makeSingleInstance
app.makeSingleInstance = function (callback) {
  return makeSingleInstance.call(app, function (argv, ...args) {
    if (process.platform === 'win32') {
      // Delay the checking so the callback is run as quickly as we would expect
      setImmediate(() => {
        argv.forEach((arg) => {
          const parsedURL = url.parse(arg)
          // The URL needs a protocol and the "//" after the protocol
          if (!parsedURL.protocol || !parsedURL.slashes) return
          // The node URL module returns the protocol as "foo-bar:", we need to strip that colon
          const protocol = parsedURL.protocol.substr(0, parsedURL.protocol.length - 1)
          if (app.getDefaultProtocolClient(protocol).split(/ /g)[0].replace(/^"(.+)"$/g, '$1') === process.execPath) {
            app._onOpenURL(arg)
          }
        })
      })
    }
    return callback(argv, ...args)
  })
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (let name of events) {
  app.on(name, (event, webContents, ...args) => {
    webContents.emit(name, event, ...args)
  })
}

// TODO(MarshallOfSound): Remove in 4.0
app.releaseSingleInstance = () => {
  deprecate.warn('app.releaseSingleInstance(cb)', 'app.releaseSingleInstanceLock()')
  app.releaseSingleInstanceLock()
}

// TODO(MarshallOfSound): Remove in 4.0
app.makeSingleInstance = (oldStyleFn) => {
  deprecate.warn('app.makeSingleInstance(cb)', 'app.requestSingleInstanceLock() and app.on(\'second-instance\', cb)')
  if (oldStyleFn && typeof oldStyleFn === 'function') {
    app.on('second-instance', (event, ...args) => oldStyleFn(...args))
  }
  return !app.requestSingleInstanceLock()
}

// Wrappers for native classes.
const {DownloadItem} = process.atomBinding('download_item')
Object.setPrototypeOf(DownloadItem.prototype, EventEmitter.prototype)
