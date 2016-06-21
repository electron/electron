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
  quit: 'quit'
}

// Maps methods that should be called directly on the BrowserWindow instance
const methodInBrowserWindow = {
  minimize: true,
  close: true
}

const methodInApp = {
  quit: true
}

const MenuItem = (function () {
  MenuItem.types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  function MenuItem (options) {
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
    if ((this.submenu != null) && this.submenu.constructor !== Menu) {
      this.submenu = Menu.buildFromTemplate(this.submenu)
    }
    if ((this.type == null) && (this.submenu != null)) {
      this.type = 'submenu'
    }
    if (this.type === 'submenu' && (this.submenu != null ? this.submenu.constructor : void 0) !== Menu) {
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
    if (MenuItem.types.indexOf(this.type) === -1) {
      throw new Error('Unknown menu type ' + this.type)
    }
    this.commandId = ++nextCommandId
    this.click = (focusedWindow) => {
      // Manually flip the checked flags when clicked.
      if (this.type === 'checkbox' || this.type === 'radio') {
        this.checked = !this.checked
      }

      if (this.role && rolesMap[this.role] && process.platform !== 'darwin' && (focusedWindow != null)) {
        const methodName = rolesMap[this.role]
        if(methodInApp[methodName]) {
          return app[methodName]()
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

  MenuItem.prototype.overrideProperty = function (name, defaultValue) {
    if (defaultValue == null) {
      defaultValue = null
    }
    this[name] != null ? this[name] : this[name] = defaultValue

    return this[name]
  }

  MenuItem.prototype.overrideReadOnlyProperty = function (name, defaultValue) {
    if (defaultValue == null) {
      defaultValue = null
    }
    if (this[name] == null) {
      this[name] = defaultValue
    }
    return Object.defineProperty(this, name, {
      enumerable: true,
      writable: false,
      value: this[name]
    })
  }

  return MenuItem
})()

module.exports = MenuItem
