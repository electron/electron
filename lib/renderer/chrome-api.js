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

    ipcRenderer.on('CHROME_RUNTIME_ONCONNECT', (event, webContentsId, extensionId, connectInfo) => {
      this.emit(new Port(webContentsId, extensionId, connectInfo.name))
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
  constructor (webContentsId, extensionId, name) {
    this.webContentsId = webContentsId
    this.extensionId = extensionId

    this.name = name
    this.onDisconnect = new Event()
    this.onMessage = new Event()
    this.sender = new MessageSender()

    ipcRenderer.on(`CHROME_PORT_ONMESSAGE_${extensionId}`, (event, message) => {
      this.onMessage.emit(message, new MessageSender(), function () {})
    })
  }

  disconnect () {
  }

  postMessage (message) {
    ipcRenderer.send('CHROME_PORT_POSTMESSAGE', this.webContentsId, this.extensionId, message)
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

    const webContentsId = ipcRenderer.sendSync('CHROME_RUNTIME_CONNECT', extensionId, connectInfo)
    return new Port(webContentsId, extensionId, connectInfo.name)
  }
}
