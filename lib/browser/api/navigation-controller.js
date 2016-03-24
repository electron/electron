'use strict'

const ipcMain = require('electron').ipcMain

// The history operation in renderer is redirected to browser.
ipcMain.on('ATOM_SHELL_NAVIGATION_CONTROLLER', function (event, method, ...args) {
  var ref
  return (ref = event.sender)[method].apply(ref, args)
})

ipcMain.on('ATOM_SHELL_SYNC_NAVIGATION_CONTROLLER', function (event, method, ...args) {
  var ref
  return event.returnValue = (ref = event.sender)[method].apply(ref, args)
})

// JavaScript implementation of Chromium's NavigationController.
// Instead of relying on Chromium for history control, we compeletely do history
// control on user land, and only rely on WebContents.loadURL for navigation.
// This helps us avoid Chromium's various optimizations so we can ensure renderer
// process is restarted everytime.
var NavigationController = (function () {
  function NavigationController (webContents) {
    this.webContents = webContents
    this.clearHistory()

    // webContents may have already navigated to a page.
    if (this.webContents._getURL()) {
      this.currentIndex++
      this.history.push(this.webContents._getURL())
    }
    this.webContents.on('navigation-entry-commited', (event, url, inPage, replaceEntry) => {
      var currentEntry
      if (this.inPageIndex > -1 && !inPage) {
        // Navigated to a new page, clear in-page mark.
        this.inPageIndex = -1
      } else if (this.inPageIndex === -1 && inPage) {
        // Started in-page navigations.
        this.inPageIndex = this.currentIndex
      }
      if (this.pendingIndex >= 0) {
        // Go to index.
        this.currentIndex = this.pendingIndex
        this.pendingIndex = -1
        return this.history[this.currentIndex] = url
      } else if (replaceEntry) {
        // Non-user initialized navigation.
        return this.history[this.currentIndex] = url
      } else {
        // Normal navigation. Clear history.
        this.history = this.history.slice(0, this.currentIndex + 1)
        currentEntry = this.history[this.currentIndex]
        if ((currentEntry != null ? currentEntry.url : void 0) !== url) {
          this.currentIndex++
          return this.history.push(url)
        }
      }
    })
  }

  NavigationController.prototype.loadURL = function (url, options) {
    if (options == null) {
      options = {}
    }
    this.pendingIndex = -1
    this.webContents._loadURL(url, options)
    return this.webContents.emit('load-url', url, options)
  }

  NavigationController.prototype.getURL = function () {
    if (this.currentIndex === -1) {
      return ''
    } else {
      return this.history[this.currentIndex]
    }
  }

  NavigationController.prototype.stop = function () {
    this.pendingIndex = -1
    return this.webContents._stop()
  }

  NavigationController.prototype.reload = function () {
    this.pendingIndex = this.currentIndex
    return this.webContents._reload(false)
  }

  NavigationController.prototype.reloadIgnoringCache = function () {
    this.pendingIndex = this.currentIndex
    return this.webContents._reload(true)
  }

  NavigationController.prototype.canGoBack = function () {
    return this.getActiveIndex() > 0
  }

  NavigationController.prototype.canGoForward = function () {
    return this.getActiveIndex() < this.history.length - 1
  }

  NavigationController.prototype.canGoToIndex = function (index) {
    return index >= 0 && index < this.history.length
  }

  NavigationController.prototype.canGoToOffset = function (offset) {
    return this.canGoToIndex(this.currentIndex + offset)
  }

  NavigationController.prototype.clearHistory = function () {
    this.history = []
    this.currentIndex = -1
    this.pendingIndex = -1
    return this.inPageIndex = -1
  }

  NavigationController.prototype.goBack = function () {
    if (!this.canGoBack()) {
      return
    }
    this.pendingIndex = this.getActiveIndex() - 1
    if (this.inPageIndex > -1 && this.pendingIndex >= this.inPageIndex) {
      return this.webContents._goBack()
    } else {
      return this.webContents._loadURL(this.history[this.pendingIndex], {})
    }
  }

  NavigationController.prototype.goForward = function () {
    if (!this.canGoForward()) {
      return
    }
    this.pendingIndex = this.getActiveIndex() + 1
    if (this.inPageIndex > -1 && this.pendingIndex >= this.inPageIndex) {
      return this.webContents._goForward()
    } else {
      return this.webContents._loadURL(this.history[this.pendingIndex], {})
    }
  }

  NavigationController.prototype.goToIndex = function (index) {
    if (!this.canGoToIndex(index)) {
      return
    }
    this.pendingIndex = index
    return this.webContents._loadURL(this.history[this.pendingIndex], {})
  }

  NavigationController.prototype.goToOffset = function (offset) {
    var pendingIndex
    if (!this.canGoToOffset(offset)) {
      return
    }
    pendingIndex = this.currentIndex + offset
    if (this.inPageIndex > -1 && pendingIndex >= this.inPageIndex) {
      this.pendingIndex = pendingIndex
      return this.webContents._goToOffset(offset)
    } else {
      return this.goToIndex(pendingIndex)
    }
  }

  NavigationController.prototype.getActiveIndex = function () {
    if (this.pendingIndex === -1) {
      return this.currentIndex
    } else {
      return this.pendingIndex
    }
  }

  NavigationController.prototype.length = function () {
    return this.history.length
  }

  return NavigationController
})()

module.exports = NavigationController
