import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal'
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils'
import * as url from 'url'

// Todo: Import once extensions have been turned into TypeScript
const Event = require('@electron/internal/renderer/extensions/event')

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
      _event: Electron.Event, message: string
    ) => {
      const sendResponse = function () { console.error('sendResponse is not implemented') }
      this.onMessage.emit(message, this.sender, sendResponse)
    })
  }

  disconnect () {
    if (this.disconnected) return

    ipcRendererInternal.sendToAll(this.tabId, `CHROME_PORT_DISCONNECT_${this.portId}`)
    this._onDisconnect()
  }

  postMessage (message: string) {
    ipcRendererInternal.sendToAll(this.tabId, `CHROME_PORT_POSTMESSAGE_${this.portId}`, message)
  }

  _onDisconnect () {
    this.disconnected = true
    ipcRendererInternal.removeAllListeners(`CHROME_PORT_POSTMESSAGE_${this.portId}`)
    this.onDisconnect.emit()
  }
}

// Inject chrome API to the |context|
export function injectTo (extensionId: string, isBackgroundPage: boolean, context: any) {
  const chrome = context.chrome = context.chrome || {}
  let originResultID = 1

  ipcRendererInternal.on(`CHROME_RUNTIME_ONCONNECT_${extensionId}`, (
    _event: Electron.Event, tabId: number, portId: number, connectInfo: { name: string }
  ) => {
    chrome.runtime.onConnect.emit(new Port(tabId, portId, extensionId, connectInfo.name))
  }
  )

  ipcRendererInternal.on(`CHROME_RUNTIME_ONMESSAGE_${extensionId}`, (
    _event: Electron.Event, tabId: number, message: string, resultID: number
  ) => {
    chrome.runtime.onMessage.emit(message, new MessageSender(tabId, extensionId), (messageResult: any) => {
      ipcRendererInternal.send(`CHROME_RUNTIME_ONMESSAGE_RESULT_${resultID}`, messageResult)
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
      if (isBackgroundPage) {
        console.error('chrome.runtime.connect is not supported in background page')
        return
      }

      // Parse the optional args.
      let targetExtensionId = extensionId
      let connectInfo = { name: '' }
      if (args.length === 1) {
        connectInfo = args[0]
      } else if (args.length === 2) {
        [targetExtensionId, connectInfo] = args
      }

      const { tabId, portId } = ipcRendererInternal.sendSync('CHROME_RUNTIME_CONNECT', targetExtensionId, connectInfo)
      return new Port(tabId, portId, extensionId, connectInfo.name)
    },

    // https://developer.chrome.com/extensions/runtime#method-sendMessage
    sendMessage (...args: Array<any>) {
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
          ipcRendererInternal.once(`CHROME_RUNTIME_SENDMESSAGE_RESULT_${originResultID}`,
            (_event: Electron.Event, result: any) => args[1](result)
          )

          message = args[0]
        } else {
          [targetExtensionId, message] = args
        }
      } else {
        console.error('options is not supported')
        ipcRendererInternal.once(`CHROME_RUNTIME_SENDMESSAGE_RESULT_${originResultID}`,
          (event: Electron.Event, result: any) => args[2](result)
        )
      }

      ipcRendererInternal.send('CHROME_RUNTIME_SENDMESSAGE', targetExtensionId, message, originResultID)
      originResultID++
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
      resultCallback: Chrome.Tabs.ExecuteScriptCallback
    ) {
      if (resultCallback) {
        ipcRendererInternal.once(`CHROME_TABS_EXECUTESCRIPT_RESULT_${originResultID}`,
          (_event: Electron.Event, result: any) => resultCallback([result])
        )
      }
      ipcRendererInternal.send('CHROME_TABS_EXECUTESCRIPT', originResultID, tabId, extensionId, details)
      originResultID++
    },

    // https://developer.chrome.com/extensions/tabs#method-sendMessage
    sendMessage (
      tabId: number,
      message: any,
      _options: Chrome.Tabs.SendMessageDetails,
      responseCallback: Chrome.Tabs.SendMessageCallback
    ) {
      if (responseCallback) {
        ipcRendererInternal.once(`CHROME_TABS_SEND_MESSAGE_RESULT_${originResultID}`,
          (_event: Electron.Event, result: any) => responseCallback(result)
        )
      }
      ipcRendererInternal.send('CHROME_TABS_SEND_MESSAGE', tabId, extensionId, isBackgroundPage, message, originResultID)
      originResultID++
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
}
