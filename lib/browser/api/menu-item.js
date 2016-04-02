'use strict'

var MenuItem, methodInBrowserWindow, nextCommandId, rolesMap

nextCommandId = 0

// Maps role to methods of webContents
rolesMap = {
  undo: 'undo',
  redo: 'redo',
  cut: 'cut',
  copy: 'copy',
  paste: 'paste',
  selectall: 'selectAll',
  minimize: 'minimize',
  close: 'close',
  delete: 'delete'
}

// Maps methods that should be called directly on the BrowserWindow instance
methodInBrowserWindow = {
  minimize: true,
  close: true
}

MenuItem = (function () {
  MenuItem.types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

  function MenuItem (options) {
    var click, ref
    const Menu = require('electron').Menu
    click = options.click
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
    if (this.type === 'submenu' && ((ref = this.submenu) != null ? ref.constructor : void 0) !== Menu) {
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
      var methodName, ref1, ref2
      if ((ref1 = this.type) === 'checkbox' || ref1 === 'radio') {
        this.checked = !this.checked
      }
      if (this.role && rolesMap[this.role] && process.platform !== 'darwin' && (focusedWindow != null)) {
        methodName = rolesMap[this.role]
        if (methodInBrowserWindow[methodName]) {
          return focusedWindow[methodName]()
        } else {
          return (ref2 = focusedWindow.webContents) != null ? ref2[methodName]() : void 0
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
    return this[name] != null ? this[name] : this[name] = defaultValue
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
