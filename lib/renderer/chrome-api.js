const {ipcRenderer} = require('electron')
const Event = require('./extensions/event')
const url = require('url')

let nextId = 0

class Tab {
  constructor (tabId) {
    this.id = tabId
  }
}

class MessageSender {
  constructor (tabId, extensionId) {
    this.tab = tabId ? new Tab(tabId) : null
    this.id = extensionId
    this.url = `chrome-extension://${extensionId}`
  }
}

class Port {
  constructor (tabId, portId, extensionId, name) {
    this.tabId = tabId
    this.portId = portId
    this.disconnected = false

    this.name = name
    this.onDisconnect = new Event()
    this.onMessage = new Event()
    this.sender = new MessageSender(tabId, extensionId)

    ipcRenderer.once(`CHROME_PORT_DISCONNECT_${portId}`, () => {
      this._onDisconnect()
    })
    ipcRenderer.on(`CHROME_PORT_POSTMESSAGE_${portId}`, (event, message) => {
      const sendResponse = function () { console.error('sendResponse is not implemented') }
      this.onMessage.emit(message, this.sender, sendResponse)
    })
  }

  disconnect () {
    if (this.disconnected) return

    ipcRenderer.sendToAll(this.tabId, `CHROME_PORT_DISCONNECT_${this.portId}`)
    this._onDisconnect()
  }

  postMessage (message) {
    ipcRenderer.sendToAll(this.tabId, `CHROME_PORT_POSTMESSAGE_${this.portId}`, message)
  }

  _onDisconnect () {
    this.disconnected = true
    ipcRenderer.removeAllListeners(`CHROME_PORT_POSTMESSAGE_${this.portId}`)
    this.onDisconnect.emit()
  }
}

// Inject chrome API to the |context|
exports.injectTo = function (extensionId, isBackgroundPage, context) {
  const chrome = context.chrome = context.chrome || {}
  let originResultID = 1

  ipcRenderer.on(`CHROME_RUNTIME_ONCONNECT_${extensionId}`, (event, tabId, portId, connectInfo) => {
    chrome.runtime.onConnect.emit(new Port(tabId, portId, extensionId, connectInfo.name))
  })

  ipcRenderer.on(`CHROME_RUNTIME_ONMESSAGE_${extensionId}`, (event, tabId, message, resultID) => {
    chrome.runtime.onMessage.emit(message, new MessageSender(tabId, extensionId), (messageResult) => {
      ipcRenderer.send(`CHROME_RUNTIME_ONMESSAGE_RESULT_${resultID}`, messageResult)
    })
  })

  ipcRenderer.on('CHROME_TABS_ONCREATED', (event, tabId) => {
    chrome.tabs.onCreated.emit(new Tab(tabId))
  })

  ipcRenderer.on('CHROME_TABS_ONREMOVED', (event, tabId) => {
    chrome.tabs.onRemoved.emit(tabId)
  })

  chrome.runtime = {
    id: extensionId,

    getURL: function (path) {
      return url.format({
        protocol: 'chrome-extension',
        slashes: true,
        hostname: extensionId,
        pathname: path
      })
    },

    connect (...args) {
      if (isBackgroundPage) {
        console.error('chrome.runtime.connect is not supported in background page')
        return
      }

      // Parse the optional args.
      let targetExtensionId = extensionId
      let connectInfo = {name: ''}
      if (args.length === 1) {
        connectInfo = args[0]
      } else if (args.length === 2) {
        [targetExtensionId, connectInfo] = args
      }

      const {tabId, portId} = ipcRenderer.sendSync('CHROME_RUNTIME_CONNECT', targetExtensionId, connectInfo)
      return new Port(tabId, portId, extensionId, connectInfo.name)
    },

    sendMessage (...args) {
      if (isBackgroundPage) {
        console.error('chrome.runtime.sendMessage is not supported in background page')
        return
      }

      // Parse the optional args.
      let targetExtensionId = extensionId
      let message
      if (args.length === 1) {
        message = args[0]
      } else if (args.length === 2) {
        // A case of not provide extension-id: (message, responseCallback)
        if (typeof args[1] === 'function') {
          ipcRenderer.on(`CHROME_RUNTIME_SENDMESSAGE_RESULT_${originResultID}`, (event, result) => args[1](result))
          message = args[0]
        } else {
          [targetExtensionId, message] = args
        }
      } else {
        console.error('options is not supported')
        ipcRenderer.on(`CHROME_RUNTIME_SENDMESSAGE_RESULT_${originResultID}`, (event, result) => args[2](result))
      }

      ipcRenderer.send('CHROME_RUNTIME_SENDMESSAGE', targetExtensionId, message, originResultID)
      originResultID++
    },

    onConnect: new Event(),
    onMessage: new Event(),
    onInstalled: new Event()
  }

  chrome.tabs = {
    executeScript (tabId, details, callback) {
      const requestId = ++nextId
      ipcRenderer.once(`CHROME_TABS_EXECUTESCRIPT_RESULT_${requestId}`, (event, result) => {
        callback([event.result])
      })
      ipcRenderer.send('CHROME_TABS_EXECUTESCRIPT', requestId, tabId, extensionId, details)
    },

    sendMessage (tabId, message, options, responseCallback) {
      if (responseCallback) {
        ipcRenderer.on(`CHROME_TABS_SEND_MESSAGE_RESULT_${originResultID}`, (event, result) => responseCallback(result))
      }
      ipcRenderer.send('CHROME_TABS_SEND_MESSAGE', tabId, extensionId, isBackgroundPage, message, originResultID)
      originResultID++;
    },

    onUpdated: new Event(),
    onCreated: new Event(),
    onRemoved: new Event()
  }

  chrome.extension = {
    getURL: chrome.runtime.getURL,
    connect: chrome.runtime.connect,
    onConnect: chrome.runtime.onConnect,
    sendMessage: chrome.runtime.sendMessage,
    onMessage: chrome.runtime.onMessage
  }

  chrome.storage = require('./extensions/storage')

  chrome.pageAction = {
    show () {},
    hide () {},
    setTitle () {},
    getTitle () {},
    setIcon () {},
    setPopup () {},
    getPopup () {}
  }

  chrome.i18n = require('./extensions/i18n').setup(extensionId)
  chrome.webNavigation = require('./extensions/web-navigation').setup()
}
