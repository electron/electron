'use strict'

let nextCommandId = 0

// Maps role to methods of webContents
const rolesMap = {
  undo: 'undo',
  redo: 'redo',
  cut: 'cut',
  copy: 'copy',
  paste: 'paste',
  pasteandmatchstyle: 'pasteAndMatchStyle',
  selectall: 'selectAll',
  minimize: 'minimize',
  close: 'close',
  delete: 'delete',
  quit: 'quit',
  togglefullscreen: 'toggleFullScreen'
}

// Maps methods that should be called directly on the BrowserWindow instance
const methodInBrowserWindow = {
  minimize: true,
  close: true,
  toggleFullScreen: function (window) {
    window.setFullScreen(!window.isFullScreen())
  }
}

const methodInApp = {
  quit: true
}

const MenuItem = function (options) {
  const {app, Menu} = require('electron')

  const click = options.click
  this.selector = options.selector
  this.type = options.type
  this.role = options.role
  this.label = options.label
  this.sublabel = options.sublabel
  this.accelerator = options.accelerator
  this.icon = options.icon
  this.enabled = options.enabled
  this.visible = options.visible
  this.checked = options.checked

  this.submenu = options.submenu
  if (this.submenu != null && this.submenu.constructor !== Menu) {
    this.submenu = Menu.buildFromTemplate(this.submenu)
  }
  if (this.type == null && this.submenu != null) {
    this.type = 'submenu'
  }
  if (this.type === 'submenu' && (this.submenu == null || this.submenu.constructor !== Menu)) {
    throw new Error('Invalid submenu')
  }

  this.overrideReadOnlyProperty('type', 'normal')
  this.overrideReadOnlyProperty('role')
  this.overrideReadOnlyProperty('accelerator')
  this.overrideReadOnlyProperty('icon')
  this.overrideReadOnlyProperty('submenu')

  this.overrideProperty('label', '')
  this.overrideProperty('sublabel', '')
  this.overrideProperty('enabled', true)
  this.overrideProperty('visible', true)
  this.overrideProperty('checked', false)

  if (!MenuItem.types.includes(this.type)) {
    throw new Error(`Unknown menu item type: ${this.type}`)
  }

  this.commandId = ++nextCommandId

  this.click = (focusedWindow) => {
    // Manually flip the checked flags when clicked.
    if (this.type === 'checkbox' || this.type === 'radio') {
      this.checked = !this.checked
    }

    if (this.role && rolesMap[this.role] && process.platform !== 'darwin' && focusedWindow != null) {
      const methodName = rolesMap[this.role]
      if (methodInApp[methodName]) {
        return app[methodName]()
      } else if (typeof methodInBrowserWindow[methodName] === 'function') {
        return methodInBrowserWindow[methodName](focusedWindow)
      } else if (methodInBrowserWindow[methodName]) {
        return focusedWindow[methodName]()
      } else {
        const {webContents} = focusedWindow
        return webContents != null ? webContents[methodName]() : void 0
      }
    } else if (typeof click === 'function') {
      return click(this, focusedWindow)
    } else if (typeof this.selector === 'string' && process.platform === 'darwin') {
      return Menu.sendActionToFirstResponder(this.selector)
    }
  }
}

MenuItem.types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

MenuItem.prototype.overrideProperty = function (name, defaultValue) {
  if (defaultValue == null) {
    defaultValue = null
  }
  if (this[name] == null) {
    this[name] = defaultValue
  }
}

MenuItem.prototype.overrideReadOnlyProperty = function (name, defaultValue) {
  this.overrideProperty(name, defaultValue)
  Object.defineProperty(this, name, {
    enumerable: true,
    writable: false,
    value: this[name]
  })
}

module.exports = MenuItem
