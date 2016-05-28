const {ipcRenderer} = require('electron')
const url = require('url')

let nextId = 0

class Event {
  constructor () {
    this.listeners = []
  }

  addListener (callback) {
    this.listeners.push(callback)
  }

  removeListener (callback) {
    const index = this.listeners.indexOf(callback)
    if (index !== -1) {
      this.listeners.splice(index, 1)
    }
  }

  emit (...args) {
    for (const listener of this.listeners) {
      listener(...args)
    }
  }
}

class Tab {
  constructor (webContentsId) {
    this.id = webContentsId
  }
}

class MessageSender {
  constructor (webContentsId, extensionId) {
    this.tab = new Tab(webContentsId)
    this.id = extensionId
    this.url = `chrome-extension://${extensionId}`
  }
}

class Port {
  constructor (webContentsId, portId, extensionId, name) {
    this.webContentsId = webContentsId
    this.portId = portId
    this.disconnected = false

    this.name = name
    this.onDisconnect = new Event()
    this.onMessage = new Event()
    this.sender = new MessageSender(webContentsId, extensionId)

    ipcRenderer.once(`CHROME_PORT_ONDISCONNECT_${portId}`, () => {
      this._onDisconnect()
    })
    ipcRenderer.on(`CHROME_PORT_ONMESSAGE_${portId}`, (event, message) => {
      const sendResponse = function () { console.error('sendResponse is not implemented') }
      this.onMessage.emit(message, this.sender, sendResponse)
    })
  }

  disconnect () {
    if (this.disconnected) return

    ipcRenderer.send('CHROME_PORT_DISCONNECT', this.webContentsId, this.portId)
    this._onDisconnect()
  }

  postMessage (message) {
    ipcRenderer.send('CHROME_PORT_POSTMESSAGE', this.webContentsId, this.portId, message)
  }

  _onDisconnect () {
    this.disconnected = true
    ipcRenderer.removeAllListeners(`CHROME_PORT_ONMESSAGE_${this.portId}`)
    this.onDisconnect.emit()
  }
}

class OnConnect extends Event {
  constructor () {
    super()

    ipcRenderer.on('CHROME_RUNTIME_ONCONNECT', (event, webContentsId, portId, extensionId, connectInfo) => {
      this.emit(new Port(webContentsId, portId, extensionId, connectInfo.name))
    })
  }
}

// Inject chrome API to the |context|
exports.injectTo = function (extensionId, context) {
  const chrome = context.chrome = context.chrome || {}

  chrome.runtime = {
    getURL: function (path) {
      return url.format({
        protocol: 'chrome-extension',
        slashes: true,
        hostname: extensionId,
        pathname: path
      })
    },

    onConnect: new OnConnect(),

    connect (...args) {
      // Parse the optional args.
      let targetExtensionId = extensionId
      let connectInfo = {name: ''}
      if (args.length === 1) {
        connectInfo = args[0]
      } else if (args.length === 2) {
        [targetExtensionId, connectInfo] = args
      }

      const {webContentsId, portId} = ipcRenderer.sendSync('CHROME_RUNTIME_CONNECT', targetExtensionId, connectInfo)
      return new Port(webContentsId, portId, extensionId, connectInfo.name)
    }
  }

  chrome.tabs = {
    executeScript (tabId, details, callback) {
      const requestId = ++nextId
      ipcRenderer.once(`CHROME_TABS_EXECUTESCRIPT_RESULT_${requestId}`, (event, result) => {
        callback([event.result])
      })
      ipcRenderer.send('CHROME_TABS_EXECUTESCRIPT', requestId, tabId, extensionId, details)
    }
  }

  chrome.extension = {
    getURL: chrome.runtime.getURL
  }
}
