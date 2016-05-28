const {ipcRenderer} = require('electron')
const url = require('url')

// TODO(zcbenz): Set it to correct value for content scripts.
const currentExtensionId = window.location.hostname

class Event {
  constructor () {
    this.listeners = []
  }

  addListener (callback) {
    this.listeners.push(callback)
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

  _onDisconnect() {
    ipcRenderer.removeAllListeners(`CHROME_PORT_ONMESSAGE_${this.portId}`)
    this.onDisconnect.emit()
  }
}

const chrome = window.chrome = window.chrome || {}

chrome.extension = {
  getURL: function (path) {
    return url.format({
      protocol: window.location.protocol,
      slashes: true,
      hostname: currentExtensionId,
      pathname: path
    })
  }
}

chrome.runtime = {
  getURL: chrome.extension.getURL,

  onConnect: new OnConnect(),

  connect (...args) {
    // Parse the optional args.
    let extensionId = currentExtensionId
    let connectInfo = {name: ''}
    if (args.length === 1) {
      connectInfo = args[0]
    } else if (args.length === 2) {
      [extensionId, connectInfo] = args
    }

    const {webContentsId, portId} = ipcRenderer.sendSync('CHROME_RUNTIME_CONNECT', extensionId, connectInfo)
    return new Port(webContentsId, portId, connectInfo.name)
  }
}
