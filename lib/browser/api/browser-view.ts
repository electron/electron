import { EventEmitter } from 'events'
const { BrowserView } = process.electronBinding('browser_view')

// BrowserView is an EventEmitter.
Object.setPrototypeOf(BrowserView.prototype, EventEmitter.prototype)

BrowserView.fromWebContents = (webContents) => {
  for (const view of BrowserView.getAllViews()) {
    if (view.webContents.equal(webContents)) return view
  }

  return null
}

module.exports = BrowserView
