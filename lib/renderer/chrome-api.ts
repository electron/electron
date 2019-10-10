import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils'
import * as url from 'url'

import { Event } from '@electron/internal/renderer/extensions/event'

class Tab {
  public id: number

  constructor (tabId: number) {
    this.id = tabId
  }
}

class MessageSender {
  public tab: Tab | null
  public id: string
  public url: string

  constructor (tabId: number, extensionId: string) {
    this.tab = tabId ? new Tab(tabId) : null
    this.id = extensionId
    this.url = `chrome-extension://${extensionId}`
  }
}

class Port {
  public disconnected: boolean = false
  public onDisconnect = new Event()
  public onMessage = new Event()
  public sender: MessageSender

  constructor (public tabId: number, public portId: number, extensionId: string, public name: string) {
    this.onDisconnect = new Event()
    this.onMessage = new Event()
    this.sender = new MessageSender(tabId, extensionId)

    ipcRendererInternal.once(`CHROME_PORT_DISCONNECT_${portId}`, () => {
      this._onDisconnect()
    })

    ipcRendererInternal.on(`CHROME_PORT_POSTMESSAGE_${portId}`, (
      _event: Electron.Event, message: any
    ) => {
      const sendResponse = function () { console.error('sendResponse is not implemented') }
      this.onMessage.emit(JSON.parse(message), this.sender, sendResponse)
    })
  }

  disconnect () {
    if (this.disconnected) return

    ipcRendererInternal.sendToAll(this.tabId, `CHROME_PORT_DISCONNECT_${this.portId}`)
    this._onDisconnect()
  }

  postMessage (message: any) {
    ipcRendererInternal.sendToAll(this.tabId, `CHROME_PORT_POSTMESSAGE_${this.portId}`, JSON.stringify(message))
  }

  _onDisconnect () {
    this.disconnected = true
    ipcRendererInternal.removeAllListeners(`CHROME_PORT_POSTMESSAGE_${this.portId}`)
    this.onDisconnect.emit()
  }
}

// Inject chrome API to the |context|
export function injectTo (extensionId: string, context: any) {
  if (process.electronBinding('features').isExtensionsEnabled()) {
    throw new Error('Attempted to load JS chrome-extension polyfill with //extensions support enabled')
  }

  const chrome = context.chrome = context.chrome || {}

  ipcRendererInternal.on(`CHROME_RUNTIME_ONCONNECT_${extensionId}`, (
    _event: Electron.Event, tabId: number, portId: number, connectInfo: { name: string }
  ) => {
    chrome.runtime.onConnect.emit(new Port(tabId, portId, extensionId, connectInfo.name))
  })

  ipcRendererUtils.handle(`CHROME_RUNTIME_ONMESSAGE_${extensionId}`, (
    _event: Electron.Event, tabId: number, message: string
  ) => {
    return new Promise(resolve => {
      chrome.runtime.onMessage.emit(message, new MessageSender(tabId, extensionId), resolve)
    })
  })

  ipcRendererInternal.on('CHROME_TABS_ONCREATED', (_event: Electron.Event, tabId: number) => {
    chrome.tabs.onCreated.emit(new Tab(tabId))
  })

  ipcRendererInternal.on('CHROME_TABS_ONREMOVED', (_event: Electron.Event, tabId: number) => {
    chrome.tabs.onRemoved.emit(tabId)
  })

  chrome.runtime = {
    id: extensionId,

    // https://developer.chrome.com/extensions/runtime#method-getURL
    getURL: function (path: string) {
      return url.format({
        protocol: 'chrome-extension',
        slashes: true,
        hostname: extensionId,
        pathname: path
      })
    },

    // https://developer.chrome.com/extensions/runtime#method-getManifest
    getManifest: function () {
      const manifest = ipcRendererUtils.invokeSync('CHROME_EXTENSION_MANIFEST', extensionId)
      return manifest
    },

    // https://developer.chrome.com/extensions/runtime#method-connect
    connect (...args: Array<any>) {
      // Parse the optional args.
      let targetExtensionId = extensionId
      let connectInfo = { name: '' }
      if (args.length === 1) {
        targetExtensionId = args[0]
      } else if (args.length === 2) {
        [targetExtensionId, connectInfo] = args
      }

      const { tabId, portId } = ipcRendererUtils.invokeSync('CHROME_RUNTIME_CONNECT', targetExtensionId, connectInfo)
      return new Port(tabId, portId, extensionId, connectInfo.name)
    },

    // https://developer.chrome.com/extensions/runtime#method-sendMessage
    sendMessage (...args: Array<any>) {
      // Parse the optional args.
      const targetExtensionId = extensionId
      let message: string
      let options: Object | undefined
      let responseCallback: Chrome.Tabs.SendMessageCallback = () => {}

      if (typeof args[args.length - 1] === 'function') {
        responseCallback = args.pop()
      }

      if (args.length === 1) {
        [message] = args
      } else if (args.length === 2) {
        if (typeof args[0] === 'string') {
          [extensionId, message] = args
        } else {
          [message, options] = args
        }
      } else {
        [extensionId, message, options] = args
      }

      if (options) {
        console.error('options are not supported')
      }

      ipcRendererInternal.invoke('CHROME_RUNTIME_SEND_MESSAGE', targetExtensionId, message).then(responseCallback)
    },

    onConnect: new Event(),
    onMessage: new Event(),
    onInstalled: new Event()
  }

  chrome.tabs = {
    // https://developer.chrome.com/extensions/tabs#method-executeScript
    executeScript (
      tabId: number,
      details: Chrome.Tabs.ExecuteScriptDetails,
      resultCallback: Chrome.Tabs.ExecuteScriptCallback = () => {}
    ) {
      ipcRendererInternal.invoke('CHROME_TABS_EXECUTE_SCRIPT', tabId, extensionId, details)
        .then((result: any) => resultCallback([result]))
    },

    // https://developer.chrome.com/extensions/tabs#method-sendMessage
    sendMessage (
      tabId: number,
      message: any,
      _options: Chrome.Tabs.SendMessageDetails,
      responseCallback: Chrome.Tabs.SendMessageCallback = () => {}
    ) {
      ipcRendererInternal.invoke('CHROME_TABS_SEND_MESSAGE', tabId, extensionId, message).then(responseCallback)
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

  chrome.storage = require('@electron/internal/renderer/extensions/storage').setup(extensionId)

  chrome.pageAction = {
    show () {},
    hide () {},
    setTitle () {},
    getTitle () {},
    setIcon () {},
    setPopup () {},
    getPopup () {}
  }

  chrome.i18n = require('@electron/internal/renderer/extensions/i18n').setup(extensionId)
  chrome.webNavigation = require('@electron/internal/renderer/extensions/web-navigation').setup()

  // Electron has no concept of a browserAction but we should stub these APIs for compatibility
  chrome.browserAction = {
    setIcon () {},
    setPopup () {}
  }
}
