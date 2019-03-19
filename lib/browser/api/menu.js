'use strict'

const { TopLevelWindow, MenuItem, webContents } = require('electron')
const { sortMenuItems } = require('@electron/internal/browser/api/menu-utils')
const EventEmitter = require('events').EventEmitter
const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

const { Menu } = bindings
let applicationMenu = null
let groupIdIndex = 0

Object.setPrototypeOf(Menu.prototype, EventEmitter.prototype)

// Menu Delegate.
// This object should hold no reference to |Menu| to avoid cyclic reference.
const delegate = {
  isCommandIdChecked: (menu, id) => menu.commandsMap[id] ? menu.commandsMap[id].checked : undefined,
  isCommandIdEnabled: (menu, id) => menu.commandsMap[id] ? menu.commandsMap[id].enabled : undefined,
  isCommandIdVisible: (menu, id) => menu.commandsMap[id] ? menu.commandsMap[id].visible : undefined,
  getAcceleratorForCommandId: (menu, id, useDefaultAccelerator) => {
    const command = menu.commandsMap[id]
    if (!command) return
    if (command.accelerator != null) return command.accelerator
    if (useDefaultAccelerator) return command.getDefaultRoleAccelerator()
  },
  shouldRegisterAcceleratorForCommandId: (menu, id) => menu.commandsMap[id] ? menu.commandsMap[id].registerAccelerator : undefined,
  executeCommand: (menu, event, id) => {
    const command = menu.commandsMap[id]
    if (!command) return
    command.click(event, TopLevelWindow.getFocusedWindow(), webContents.getFocusedWebContents())
  },
  menuWillShow: (menu) => {
    // Ensure radio groups have at least one menu item seleted
    for (const id in menu.groupsMap) {
      const found = menu.groupsMap[id].find(item => item.checked) || null
      if (!found) v8Util.setHiddenValue(menu.groupsMap[id][0], 'checked', true)
    }
  }
}

/* Instance Methods */

Menu.prototype._init = function () {
  this.commandsMap = {}
  this.groupsMap = {}
  this.items = []
  this.delegate = delegate
}

Menu.prototype.popup = function (options = {}) {
  if (options == null || typeof options !== 'object') {
    throw new TypeError('Options must be an object')
  }
  let { window, x, y, positioningItem, callback } = options

  // no callback passed
  if (!callback || typeof callback !== 'function') callback = () => {}

  // set defaults
  if (typeof x !== 'number') x = -1
  if (typeof y !== 'number') y = -1
  if (typeof positioningItem !== 'number') positioningItem = -1

  // find which window to use
  const wins = TopLevelWindow.getAllWindows()
  if (!wins || wins.indexOf(window) === -1) {
    window = TopLevelWindow.getFocusedWindow()
    if (!window && wins && wins.length > 0) {
      window = wins[0]
    }
    if (!window) {
      throw new Error(`Cannot open Menu without a TopLevelWindow present`)
    }
  }

  this.popupAt(window, x, y, positioningItem, callback)
  return { browserWindow: window, x, y, position: positioningItem }
}

Menu.prototype.closePopup = function (window) {
  if (window instanceof TopLevelWindow) {
    this.closePopupAt(window.id)
  } else {
    // Passing -1 (invalid) would make closePopupAt close the all menu runners
    // belong to this menu.
    this.closePopupAt(-1)
  }
}

Menu.prototype.getMenuItemById = function (id) {
  const items = this.items

  let found = items.find(item => item.id === id) || null
  for (let i = 0; !found && i < items.length; i++) {
    if (items[i].submenu) {
      found = items[i].submenu.getMenuItemById(id)
    }
  }
  return found
}

Menu.prototype.append = function (item) {
  return this.insert(this.getItemCount(), item)
}

Menu.prototype.insert = function (pos, item) {
  if ((item ? item.constructor : void 0) !== MenuItem) {
    throw new TypeError('Invalid item')
  }

  if (pos < 0) {
    throw new RangeError(`Position ${pos} cannot be less than 0`)
  } else if (pos > this.getItemCount()) {
    throw new RangeError(`Position ${pos} cannot be greater than the total MenuItem count`)
  }

  // insert item depending on its type
  insertItemByType.call(this, item, pos)

  // set item properties
  if (item.sublabel) this.setSublabel(pos, item.sublabel)
  if (item.icon) this.setIcon(pos, item.icon)
  if (item.role) this.setRole(pos, item.role)

  // Make menu accessable to items.
  item.overrideReadOnlyProperty('menu', this)

  // Remember the items.
  this.items.splice(pos, 0, item)
  this.commandsMap[item.commandId] = item
}

Menu.prototype._callMenuWillShow = function () {
  if (this.delegate) this.delegate.menuWillShow(this)
  this.items.forEach(item => {
    if (item.submenu) item.submenu._callMenuWillShow()
  })
}

/* Static Methods */

Menu.getApplicationMenu = () => applicationMenu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

// set application menu with a preexisting menu
Menu.setApplicationMenu = function (menu) {
  if (menu && menu.constructor !== Menu) {
    throw new TypeError('Invalid menu')
  }

  applicationMenu = menu
  v8Util.setHiddenValue(global, 'applicationMenuSet', true)

  if (process.platform === 'darwin') {
    if (!menu) return
    menu._callMenuWillShow()
    bindings.setApplicationMenu(menu)
  } else {
    const windows = TopLevelWindow.getAllWindows()
    return windows.map(w => w.setMenu(menu))
  }
}

Menu.buildFromTemplate = function (template) {
  if (!Array.isArray(template)) {
    throw new TypeError('Invalid template for Menu: Menu template must be an array')
  }
  if (!areValidTemplateItems(template)) {
    throw new TypeError('Invalid template for MenuItem: must have at least one of label, role or type')
  }
  const filtered = removeExtraSeparators(template)
  const sorted = sortTemplate(filtered)

  const menu = new Menu()
  sorted.forEach(item => {
    if (item instanceof MenuItem) {
      menu.append(item)
    } else {
      menu.append(new MenuItem(item))
    }
  })

  return menu
}

/* Helper Functions */

// validate the template against having the wrong attribute
function areValidTemplateItems (template) {
  return template.every(item =>
    item != null &&
    typeof item === 'object' &&
    (item.hasOwnProperty('label') ||
     item.hasOwnProperty('role') ||
     item.type === 'separator'))
}

function sortTemplate (template) {
  const sorted = sortMenuItems(template)
  for (const id in sorted) {
    const item = sorted[id]
    if (Array.isArray(item.submenu)) {
      item.submenu = sortTemplate(item.submenu)
    }
  }
  return sorted
}

// Search between separators to find a radio menu item and return its group id
function generateGroupId (items, pos) {
  if (pos > 0) {
    for (let idx = pos - 1; idx >= 0; idx--) {
      if (items[idx].type === 'radio') return items[idx].groupId
      if (items[idx].type === 'separator') break
    }
  } else if (pos < items.length) {
    for (let idx = pos; idx <= items.length - 1; idx++) {
      if (items[idx].type === 'radio') return items[idx].groupId
      if (items[idx].type === 'separator') break
    }
  }
  groupIdIndex += 1
  return groupIdIndex
}

function removeExtraSeparators (items) {
  // fold adjacent separators together
  let ret = items.filter((e, idx, arr) => {
    if (e.visible === false) return true
    return e.type !== 'separator' || idx === 0 || arr[idx - 1].type !== 'separator'
  })

  // remove edge separators
  ret = ret.filter((e, idx, arr) => {
    if (e.visible === false) return true
    return e.type !== 'separator' || (idx !== 0 && idx !== arr.length - 1)
  })

  return ret
}

function insertItemByType (item, pos) {
  const types = {
    normal: () => this.insertItem(pos, item.commandId, item.label),
    checkbox: () => this.insertCheckItem(pos, item.commandId, item.label),
    separator: () => this.insertSeparator(pos),
    submenu: () => this.insertSubMenu(pos, item.commandId, item.label, item.submenu),
    radio: () => {
      // Grouping radio menu items
      item.overrideReadOnlyProperty('groupId', generateGroupId(this.items, pos))
      if (this.groupsMap[item.groupId] == null) {
        this.groupsMap[item.groupId] = []
      }
      this.groupsMap[item.groupId].push(item)

      // Setting a radio menu item should flip other items in the group.
      v8Util.setHiddenValue(item, 'checked', item.checked)
      Object.defineProperty(item, 'checked', {
        enumerable: true,
        get: () => v8Util.getHiddenValue(item, 'checked'),
        set: () => {
          this.groupsMap[item.groupId].forEach(other => {
            if (other !== item) v8Util.setHiddenValue(other, 'checked', false)
          })
          v8Util.setHiddenValue(item, 'checked', true)
        }
      })
      this.insertRadioItem(pos, item.commandId, item.label, item.groupId)
    }
  }
  types[item.type]()
}

module.exports = Menu
