const {app} = require('electron')
const {EventEmitter} = require('events')
const {Notification: NativeNotification, isSupported} = process.atomBinding('notification')
const querystring = require('querystring')
const path = require('path')
const url = require('url')

Object.setPrototypeOf(NativeNotification.prototype, EventEmitter.prototype)

let providedProtocol
let notificationId = 1
const notificationMap = {}

class Notification extends NativeNotification {
  constructor (options) {
    super(options)
    this.id = notificationId++
    notificationMap[this.id] = this
    // On windows we require certain things to be true before allowing buttons
    // * isSingleInstance() === true
    // * protocol has been provided
    if (process.platform === 'win32' && options.actions && options.actions.find(action => action.type === 'button')) {
      if (!providedProtocol) {
        throw new Error('You must call Notification.setWindowsProtocol before using button actions on Windows')
      }
      this._setWindowsProtocolHandler(`${providedProtocol}/${this.id}`)
    }
    delete this._setWindowsProtocolHandler
  }
}

Notification.isSupported = isSupported
Notification.fromID = (id) => notificationMap[id]

// Windows magic stuff to support buttons
Notification.setWindowsProtocol = (protocol) => {
  if (process.platform !== 'win32' || !protocol) return
  if (!app.isSingleInstance()) {
    throw new Error('You must call app.makeSingleInstance before using button actions on Windows')
  }
  providedProtocol = `${protocol}://action`
  app.setAsDefaultProtocolClient(protocol, process.execPath, process.defaultAppRan ? [path.resolve(__dirname, '../../../default_app.asar/index.html')] : [])
}

app.on('single-instance', (event, argv) => {
  if (!providedProtocol) return
  let notificationAction = argv.find(arg => arg.startsWith(providedProtocol))
  if (!notificationAction) return
  event.preventDefault()
  // Let's play this safe to stop people crashing apps by sending random fakish arg strings
  try {
    const parsed = url.parse(notificationAction)
    const parts = parsed.pathname.substr(1).split('/')
    // This is unexpected
    if (parts.length !== 2) return
    // It's not a button so who knows what's going on
    if (parts[1] !== 'button') return
    const id = parseInt(parts[0], 10)
    if (isNaN(id)) return
    const notification = Notification.fromID(id)
    if (!notification) return
    const parsedQuerystring = querystring.parse(parsed.query)
    if (typeof parsedQuerystring.id === 'undefined') return
    const actionId = parseInt(parsedQuerystring.id, 10)
    if (isNaN(actionId)) return
    notification._emitAction(actionId)
  } catch (err) {
    // Ignore
  }
})

module.exports = Notification
