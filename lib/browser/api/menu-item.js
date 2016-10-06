'use strict'

const roles = require('./menu-item-roles')

let nextCommandId = 0

const MenuItem = function (options) {
  const {Menu} = require('electron')

  // Clone and sanitize the provided options
  options = Object.assign({}, options)
  delete options.overrideProperty
  delete options.overrideReadOnlyProperty

  Object.assign(this, options)

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

  this.overrideProperty('label', roles.getDefaultLabel(this.role))
  this.overrideProperty('sublabel', '')
  this.overrideProperty('enabled', true)
  this.overrideProperty('visible', true)
  this.overrideProperty('checked', false)

  if (!MenuItem.types.includes(this.type)) {
    throw new Error(`Unknown menu item type: ${this.type}`)
  }

  this.overrideReadOnlyProperty('commandId', ++nextCommandId)

  const click = options.click
  this.click = (event, focusedWindow, focusedWebContents) => {
    // Manually flip the checked flags when clicked.
    if (this.type === 'checkbox' || this.type === 'radio') {
      this.checked = !this.checked
    }

    if (!roles.execute(this.role, focusedWindow, focusedWebContents)) {
      if (typeof click === 'function') {
        click(this, focusedWindow, event)
      } else if (typeof this.selector === 'string' && process.platform === 'darwin') {
        Menu.sendActionToFirstResponder(this.selector)
      }
    }
  }
}

MenuItem.types = ['normal', 'separator', 'submenu', 'checkbox', 'radio']

MenuItem.prototype.getDefaultRoleAccelerator = function () {
  return roles.getDefaultAccelerator(this.role)
}

MenuItem.prototype.overrideProperty = function (name, defaultValue = null) {
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
