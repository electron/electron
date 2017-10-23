const {BrowserWindow, MenuItem} = require('electron')
const {EventEmitter} = require('events')
//const _ = require('lodash')

const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

const { Menu } = bindings

Menu.prototype.__proto__ = EventEmitter.prototype

Menu.prototype._init = function () {
  this.commandsMap = {}
  this.groupsMap = {}
  this.items = []
  return this.delegate = {
    isCommandIdChecked: id => (this.commandsMap[id] ? this.commandsMap[id].checked : undefined),
    isCommandIdEnabled: id => (this.commandsMap[id] ? this.commandsMap[id].enabled : undefined),
    isCommandIdVisible: id => (this.commandsMap[id] ? this.commandsMap[id].visible : undefined),
    getAcceleratorForCommandId: id => (this.commandsMap[id] ? this.commandsMap[id].accelerator : undefined),
    getIconForCommandId: id => (this.commandsMap[id] ? this.commandsMap[id].icon : undefined),
    executeCommand: (id) => {
      const command = this.commandsMap[id]
      return command ? this.commandsMap[id].click(BrowserWindow.getFocusedWindow()) : undefined
    },
    menuWillShow: () => {
      for (let id in this.groupsMap) {
        const group = this.groupsMap[id]
        let checked = false
        for (let radioItem in group) {
          if (radioItem.checked) {
            checked = true
            break
          }
        }
        if (!checked) v8Util.setHiddenValue(group[0], 'checked', true)
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

  if (window == null) window = BrowserWindow.getFocusedWindow()
  if (typeof x !== 'number') x = -1
  if (typeof y !== 'number') y = -1
  if (typeof positioningItem !== 'number') positioningItem = -1
  if (typeof asyncPopup !== 'boolean') asyncPopup = false

  this.popupAt(window, x, y, positioningItem, asyncPopup)
}

Menu.prototype.append = function (item) {
  return this.insert(this.getItemCount(), item)
}

Menu.prototype.insert = function (pos, item) {
  if ((item != null ? item.constructor : undefined) !== MenuItem) {
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
      /* Grouping radio menu items. */
      item.overrideReadOnlyProperty('groupId', generateGroupId(this.items, pos))
      if (this.groupsMap[item.groupId] == null) this.groupsMap[item.groupId] = []
      this.groupsMap[item.groupId].push(item)

      /* Setting a radio menu item should flip other items in the group. */
      v8Util.setHiddenValue(item, 'checked', item.checked)
      Object.defineProperty(item, 'checked', {
        enumerable: true,
        get() { return v8Util.getHiddenValue(item, 'checked') },
        set: val => {
          for (let otherItem in this.groupsMap[item.groupId]) {
            if (otherItem !== item) v8Util.setHiddenValue(otherItem, 'checked', false)
          }
          return v8Util.setHiddenValue(item, 'checked', true)
        }
      })

      this.insertRadioItem(pos, item.commandId, item.label, item.groupId)
      break
  }

  if (item.sublabel != null) this.setSublabel(pos, item.sublabel)
  if (item.icon != null) this.setIcon(pos, item.icon)
  if (item.role != null) this.setRole(pos, item.role)

  /* Make menu accessable to items. */
  item.overrideReadOnlyProperty('menu', this)

  /* Remember the items. */
  this.items.splice(pos, 0, item)
  return this.commandsMap[item.commandId] = item
}

/* Force menuWillShow to be called */
Menu.prototype._callMenuWillShow = function () {
  if (this.delegate != null) this.delegate.menuWillShow()

  this.items.forEach(item => {
    if (item.submenu != null) item.submenu._callMenuWillShow()
  })
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

Menu.setApplicationMenu = function (menu) {
  if (menu !== null && menu.constructor !== Menu) {
    throw new TypeError('Invalid menu')
  }

  applicationMenu = menu
  if (process.platform === 'darwin') {
    if (menu === null) return
    menu._callMenuWillShow()
    return bindings.setApplicationMenu(menu)
  } else {
    const windows = BrowserWindow.getAllWindows()
    return windows.map((w) => w.setMenu(menu))
  }
}

Menu.getApplicationMenu = () => applicationMenu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

//  build menu from a template
Menu.buildFromTemplate = function (template) {
  if (!(template instanceof Array)) {
    throw new TypeError('Invalid template for Menu')
  }

  const menu = new Menu
  const positioned = []
  let idx = 0

  template.forEach(item => {
    idx = (item.position) ? indexToInsertByPosition(positioned, item.position) : idx++
    positioned.splice(idx, 0, item)
  })

  positioned.forEach((item) => {
    if (typeof item !== 'object') {
      throw new TypeError('Invalid template for MenuItem')
    }
    menu.append(new MenuItem(item))
  })

  return menu
}

// Automatically generated radio menu item's group id.
let nextGroupId = 0
let applicationMenu = null

/* Search between seperators to find a radio menu item and return its group id */
const generateGroupId = function(items, pos) {
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
  return ++nextGroupId
}

/* Returns the index of item according to |id|. */
const indexOfItemById = function (items, id) {
  for (let i = 0; i < items.length; i++) {
    const item = items[i]
    if (item.id === id) return i
  }
  return -1
}

/* Returns the index of where to insert the item according to |position|. */
function indexToInsertByPosition (items, position) {
  if (!position) return items.length
  const [query, id] = position.split('=')
  let idx = indexOfItemById(items, id)

  if (idx === -1 && query !== 'endof') {
    console.warn(`Item with id '${id}' is not found`)
    return items.length
  }

  const queries = {
    'after': () => idx++,
    'endof': () => {
      if (idx === -1) {
        items.push({id, type: 'separator'})
        idx = items.length - 1
      }

      idx++
      while (idx < items.length && items[idx].type !== 'separator') {
        idx++
      }
    }
  }

  queries[query]()
  return idx
}

// WIP, need to figure out how to pass current menu group to this method
function isOuterSeparator (current, item) {
  current.filter(item => { !item.visible })
  const idx = current.indexof(item)
  if (idx === 0 || idx === current.length) {
    return true
  }
  return false
}

module.exports = Menu