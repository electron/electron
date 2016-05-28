const {ipcRenderer} = require('electron')
const url = require('url')

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

class OnConnect extends Event {
  constructor () {
    super()

    ipcRenderer.on('CHROME_RUNTIME_ONCONNECT', (event, webContentsId, portId, connectInfo) => {
      this.emit(new Port(webContentsId, portId, connectInfo.name))
    })
  }
}

class MessageSender {
  constructor () {
    this.tab = null
    this.frameId = null
    this.id = null
    this.url = null
    this.tlsChannelId = null
  }
}

class Port {
  constructor (webContentsId, portId, name) {
    this.webContentsId = webContentsId
    this.portId = portId

    this.name = name
    this.onDisconnect = new Event()
    this.onMessage = new Event()
    this.sender = new MessageSender()

    ipcRenderer.once(`CHROME_PORT_ONDISCONNECT_${portId}`, () => {
      this._onDisconnect()
    })
    ipcRenderer.on(`CHROME_PORT_ONMESSAGE_${portId}`, (event, message) => {
      this.onMessage.emit(message, new MessageSender(), function () {})
    })
  }

  disconnect () {
    ipcRenderer.send('CHROME_PORT_DISCONNECT', this.webContentsId, this.portId)
    this._onDisconnect()
  }

  postMessage (message) {
    ipcRenderer.send('CHROME_PORT_POSTMESSAGE', this.webContentsId, this.portId, message)
  }

  _onDisconnect () {
    ipcRenderer.removeAllListeners(`CHROME_PORT_ONMESSAGE_${this.portId}`)
    this.onDisconnect.emit()
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
      return new Port(webContentsId, portId, connectInfo.name)
    }
  }

  chrome.extension = {
    getURL: chrome.runtime.getURL
  }
}
