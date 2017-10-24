'use strict'

const {BrowserWindow, MenuItem, webContents} = require('electron')
const EventEmitter = require('events').EventEmitter
const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

const {Menu} = bindings
let applicationMenu = null
let groupIdIndex = 0

Object.setPrototypeOf(Menu.prototype, EventEmitter.prototype)

Menu.prototype._init = function () {
  this.commandsMap = {}
  this.groupsMap = {}
  this.items = []
  this.delegate = {
    isCommandIdChecked: id => this.commandsMap[id].checked,
    isCommandIdEnabled: id => this.commandsMap[id].enabled,
    isCommandIdVisible: id => this.commandsMap[id].visible,
    getAcceleratorForCommandId: (id, useDefaultAccelerator) => {
      const command = this.commandsMap[id]
      if (command === null) return
      if (command.accelerator !== null) return command.accelerator
      if (useDefaultAccelerator) return command.getDefaultRoleAccelerator()
    },
    getIconForCommandId: id => this.commandsMap[id].icon,
    executeCommand: (event, id) => {
      const command = this.commandsMap[id]
      if (command === null) return
      command.click(event, BrowserWindow.getFocusedWindow(), webContents.getFocusedWebContents())
    },
    menuWillShow: () => {
      // Ensure radio groups have at least one menu item seleted
      for (let id in this.groupsMap) {
        let found = this.groupsMap[id].find(item => item.checked) || null
        if (!found) v8Util.setHiddenValue(this.groupsMap[id][0], 'checked', true)
      }
    }
  }
}

Menu.prototype.popup = function (window, x, y, positioningItem) {
  let asyncPopup

  // menu.popup(x, y, positioningItem)
  if (window != null && (typeof window !== 'object' || window.constructor !== BrowserWindow)) {
    // Shift.
    positioningItem = y
    y = x
    x = window
    window = null
  }

  // menu.popup(window, {})
  if (x != null && typeof x === 'object') {
    const options = x
    x = options.x
    y = options.y
    positioningItem = options.positioningItem
    asyncPopup = options.async
  }

  // Default to showing in focused window.
  if (window == null) window = BrowserWindow.getFocusedWindow()

  // Default to showing under mouse location.
  if (typeof x !== 'number') x = -1
  if (typeof y !== 'number') y = -1

  // Default to not highlighting any item.
  if (typeof positioningItem !== 'number') positioningItem = -1

  // Default to synchronous for backwards compatibility.
  if (typeof asyncPopup !== 'boolean') asyncPopup = false

  this.popupAt(window, x, y, positioningItem, asyncPopup)
}

Menu.prototype.closePopup = function (window) {
  if (window == null || window.constructor !== BrowserWindow) {
    window = BrowserWindow.getFocusedWindow()
  }
  if (window != null) {
    this.closePopupAt(window.id)
  }
}

Menu.prototype.getMenuItemById = function (id) {
  const items = this.items

  let found = items.find(item => item.id === id) || null
  for (let i = 0, length = items.length; !found && i < length; i++) {
    if (items[i].submenu) {
      found = items[i].submenu.getMenuItemById(id)
    }
  }
  return found
}

//cleaned up
Menu.prototype.append = function (item) {
  return this.insert(this.getItemCount(), item)
}

Menu.prototype.insert = function (pos, item) {
  var base, name
  if ((item != null ? item.constructor : void 0) !== MenuItem) {
    throw new TypeError('Invalid item')
  }
  switch (item.type) {
    case 'normal':
      this.insertItem(pos, item.commandId, item.label)
      break
    case 'checkbox':
      this.insertCheckItem(pos, item.commandId, item.label)
      break
    case 'separator':
      this.insertSeparator(pos)
      break
    case 'submenu':
      this.insertSubMenu(pos, item.commandId, item.label, item.submenu)
      break
    case 'radio':
      // Grouping radio menu items.
      item.overrideReadOnlyProperty('groupId', generateGroupId(this.items, pos))
      if ((base = this.groupsMap)[name = item.groupId] == null) {
        base[name] = []
      }
      this.groupsMap[item.groupId].push(item)

      // Setting a radio menu item should flip other items in the group.
      v8Util.setHiddenValue(item, 'checked', item.checked)
      Object.defineProperty(item, 'checked', {
        enumerable: true,
        get: function () {
          return v8Util.getHiddenValue(item, 'checked')
        },
        set: () => {
          var j, len, otherItem, ref1
          ref1 = this.groupsMap[item.groupId]
          for (j = 0, len = ref1.length; j < len; j++) {
            otherItem = ref1[j]
            if (otherItem !== item) {
              v8Util.setHiddenValue(otherItem, 'checked', false)
            }
          }
          return v8Util.setHiddenValue(item, 'checked', true)
        }
      })
      this.insertRadioItem(pos, item.commandId, item.label, item.groupId)
  }

  if (item.sublabel != null) this.setSublabel(pos, item.sublabel)
  if (item.icon != null) this.setIcon(pos, item.icon)
  if (item.role != null) this.setRole(pos, item.role)

  // Make menu accessable to items.
  item.overrideReadOnlyProperty('menu', this)

  // Remember the items.
  this.items.splice(pos, 0, item)
  this.commandsMap[item.commandId] = item
}

// Force menuWillShow to be called
// cleaned up
Menu.prototype._callMenuWillShow = function () {
  if (this.delegate != null) this.delegate.menuWillShow()
  this.items.forEach(item => {
    if (item.submenu != null) item.submenu._callMenuWillShow()
  })
}

// cleaned up
Menu.setApplicationMenu = function (menu) {
  if (!(menu === null || menu.constructor === Menu)) {
    throw new TypeError('Invalid menu')
  }

  applicationMenu = menu
  if (process.platform === 'darwin') {
    if (menu === null) return
    menu._callMenuWillShow()
    bindings.setApplicationMenu(menu)
  } else {
    const windows = BrowserWindow.getAllWindows()
    return windows.map(w => w.setMenu(menu))
  }
}

// cleaned up
Menu.getApplicationMenu = () => applicationMenu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

// cleaned up
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

/* Helper Methods */

// Search between separators to find a radio menu item and return its group id,
function generateGroupId (items, pos) {
  let i, item
  if (pos > 0) {
    let asc, start
    for (start = pos - 1, i = start, asc = start <= 0; asc ? i <= 0 : i >= 0; asc ? i++ : i--) {
      item = items[i]
      if (item.type === 'radio') { return item.groupId }
      if (item.type === 'separator') { break }
    }
  } else if (pos < items.length) {
    let asc1, end
    for (i = pos, end = items.length - 1, asc1 = pos <= end; asc1 ? i <= end : i >= end; asc1 ? i++ : i--) {
      item = items[i]
      if (item.type === 'radio') { return item.groupId }
      if (item.type === 'separator') { break }
    }
  }
  return ++groupIdIndex
}

// Returns the index of item according to |id|.
// cleaned up
function indexOfItemById (items, id) {
  const foundItem = items.find(item => item.id === id) || null
  return items.indexOf(foundItem)
}

// Returns the index of where to insert the item according to |position|
// cleaned up
function indexToInsertByPosition (items, position) {
  if (!position) return items.length

  const [query, id] = position.split('=') // parse query and id from position
  const idx = indexOfItemById(items, id) // calculate initial index of item

  // warn if query doesn't exist
  if (idx === -1 && query !== 'endof') {
    console.warn("Item with id '" + id + "' is not found")
    return items.length
  }

  // compute new index based on query
  const queries = {
    'after': (index) => index += 1,
    'endof': (index) => {
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

module.exports = Menu