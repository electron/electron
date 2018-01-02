'use strict'

const {BrowserWindow, MenuItem, webContents} = require('electron')
const EventEmitter = require('events').EventEmitter
const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

const {Menu} = bindings
let applicationMenu = null
let groupIdIndex = 0

Object.setPrototypeOf(Menu.prototype, EventEmitter.prototype)

/* Instance Methods */

Menu.prototype._init = function () {
  this.commandsMap = {}
  this.groupsMap = {}
  this.items = []
  this.delegate = {
    isCommandIdChecked: id => this.commandsMap[id] ? this.commandsMap[id].checked : undefined,
    isCommandIdEnabled: id => this.commandsMap[id] ? this.commandsMap[id].enabled : undefined,
    isCommandIdVisible: id => this.commandsMap[id] ? this.commandsMap[id].visible : undefined,
    getAcceleratorForCommandId: (id, useDefaultAccelerator) => {
      const command = this.commandsMap[id]
      if (!command) return
      if (command.accelerator) return command.accelerator
      if (useDefaultAccelerator) return command.getDefaultRoleAccelerator()
    },
    getIconForCommandId: id => this.commandsMap[id] ? this.commandsMap[id].icon : undefined,
    executeCommand: (event, id) => {
      const command = this.commandsMap[id]
      if (!command) return
      command.click(event, BrowserWindow.getFocusedWindow(), webContents.getFocusedWebContents())
    },
    menuWillShow: () => {
      // Ensure radio groups have at least one menu item seleted
      for (const id in this.groupsMap) {
        const found = this.groupsMap[id].find(item => item.checked) || null
        if (!found) v8Util.setHiddenValue(this.groupsMap[id][0], 'checked', true)
      }
    }
  }
}

Menu.prototype.popup = function (window, x, y, positioningItem) {
  let [newX, newY, newPosition, newWindow] = [x, y, positioningItem, window]
  let opts
  let callback

  // menu.popup(x, y, positioningItem)
  if (window != null && !(window instanceof BrowserWindow)) {
    [newPosition, newY, newX, newWindow] = [y, x, window, null]
  }

  // menu.popup([w], x, y, callback)
  if (typeof newPosition === 'function') {
    callback = newPosition
  }

  // menu.popup({})
  if (window != null && window.constructor === Object) {
    opts = window
    callback = arguments[1]
  // menu.popup(window, {})
  } else if (x && typeof x === 'object') {
    opts = x
    callback = arguments[2]
  }

  if (opts) {
    newX = opts.x
    newY = opts.y
    newPosition = opts.positioningItem
  }
  if (typeof callback !== 'function') {
    callback = () => {}
  }

  // set defaults
  if (typeof newX !== 'number') newX = -1
  if (typeof newY !== 'number') newY = -1
  if (typeof newPosition !== 'number') newPosition = -1
  if (!newWindow || (newWindow && newWindow.constructor !== BrowserWindow)) {
    newWindow = BrowserWindow.getFocusedWindow()

    // No window focused?
    if (!newWindow) {
      const browserWindows = BrowserWindow.getAllWindows()

      if (browserWindows && browserWindows.length > 0) {
        newWindow = browserWindows[0]
      } else {
        throw new Error(`Cannot open Menu without a BrowserWindow present`)
      }
    }
  }

  this.popupAt(newWindow, newX, newY, newPosition, callback)

  return { browserWindow: newWindow, x: newX, y: newY, position: newPosition }
}

Menu.prototype.closePopup = function (window) {
  if (window && window.constructor !== BrowserWindow) {
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
  if (this.delegate) this.delegate.menuWillShow()
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
  if (process.platform === 'darwin') {
    if (!menu) return
    menu._callMenuWillShow()
    bindings.setApplicationMenu(menu)
  } else {
    const windows = BrowserWindow.getAllWindows()
    return windows.map(w => w.setMenu(menu))
  }
}

Menu.buildFromTemplate = function (template) {
  if (!(template instanceof Array)) {
    throw new TypeError('Invalid template for Menu')
  }

  const menu = new Menu()
  const positioned = []
  let idx = 0

  // sort template by position
  template.forEach(item => {
    idx = (item.position) ? indexToInsertByPosition(positioned, item.position) : idx += 1
    positioned.splice(idx, 0, item)
  })

  // add each item from positioned menu to application menu
  positioned.forEach((item) => {
    if (typeof item !== 'object') {
      throw new TypeError('Invalid template for MenuItem')
    }
    menu.append(new MenuItem(item))
  })

  return menu
}

/* Helper Functions */

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

function indexOfItemById (items, id) {
  const foundItem = items.find(item => item.id === id) || -1
  return items.indexOf(foundItem)
}

function indexToInsertByPosition (items, position) {
  if (!position) return items.length

  const [query, id] = position.split('=') // parse query and id from position
  const idx = indexOfItemById(items, id) // calculate initial index of item

  // warn if query doesn't exist
  if (idx === -1 && query !== 'endof') {
    console.warn(`Item with id ${id} is not found`)
    return items.length
  }

  // compute new index based on query
  const queries = {
    after: (index) => {
      index += 1
      return index
    },
    endof: (index) => {
      if (index === -1) {
        items.push({id, type: 'separator'})
        index = items.length - 1
      }

      index += 1
      while (index < items.length && items[index].type !== 'separator') index += 1
      return index
    }
  }

  // return new index if needed, or original indexOfItemById
  return (query in queries) ? queries[query](idx) : idx
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
