const {BrowserWindow, MenuItem} = require('electron')
const {EventEmitter} = require('events')
//const _ = require('lodash')

const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

// Automatically generated radio menu item's group id.
let nextGroupId = 0

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
const indexOfItemById = function(items, id) {
  for (let i = 0; i < items.length; i++) {
    const item = items[i]
    if (item.id === id) return i
  }
  return -1
}

/* Returns the index of where to insert the item according to |position|. */
const indexToInsertByPosition = function(items, position) {
  if (!position) { return items.length }

  const [query, id] = Array.from(position.split('='))
  let insertIndex = indexOfItemById(items, id)
  if ((insertIndex === -1) && (query !== 'endof')) {
    console.warn(`Item with id '${id}' is not found`)
    return items.length
  }

  switch (query) {
    case 'after':
      insertIndex++
      break
    case 'endof':
      /* If the |id| doesn't exist, then create a new group with the |id|. */
      if (insertIndex === -1) {
        items.push({id, type: 'separator'})
        insertIndex = items.length - 1
      }

      /* Find the end of the group. */
      insertIndex++
      while ((insertIndex < items.length) && (items[insertIndex].type !== 'separator')) {
        insertIndex++
      }
      break
  }

  return insertIndex
}

const { Menu } = bindings

Menu.prototype.__proto__ = EventEmitter.prototype

Menu.prototype._init = function() {
  this.commandsMap = {}
  this.groupsMap = {}
  this.items = []
  return this.delegate = {
    isCommandIdChecked: commandId => (this.commandsMap[commandId] != null ? this.commandsMap[commandId].checked : undefined),
    isCommandIdEnabled: commandId => (this.commandsMap[commandId] != null ? this.commandsMap[commandId].enabled : undefined),
    isCommandIdVisible: commandId => (this.commandsMap[commandId] != null ? this.commandsMap[commandId].visible : undefined),
    getAcceleratorForCommandId: commandId => (this.commandsMap[commandId] != null ? this.commandsMap[commandId].accelerator : undefined),
    getIconForCommandId: commandId => (this.commandsMap[commandId] != null ? this.commandsMap[commandId].icon : undefined),
    executeCommand: commandId => {
      return (this.commandsMap[commandId] != null ? this.commandsMap[commandId].click(BrowserWindow.getFocusedWindow()) : undefined)
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

Menu.prototype.popup = function(window, x, y) {
  if ((window != null ? window.constructor : undefined) !== BrowserWindow) {
    /* Shift. */
    y = x
    x = window
    window = BrowserWindow.getFocusedWindow()
  }
  if (x !== null && y !== null) {
    return this._popupAt(window, x, y)
  } else {
    return this._popup(window)
  }
}

Menu.prototype.append = function(item) {
  return this.insert(this.getItemCount(), item)
}

Menu.prototype.insert = function(pos, item) {
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
Menu.prototype._callMenuWillShow = function() {
  if (this.delegate != null) this.delegate.menuWillShow()

  return (() => {
    const result = []
    for (let item of Array.from(this.items)) {
      if (item.submenu != null) {
        result.push(item.submenu._callMenuWillShow())
      }
    }
    return result
  })()
}

let applicationMenu = null
Menu.setApplicationMenu = function(menu) {
  if ((menu !== null) && (menu.constructor !== Menu)) { throw new TypeError('Invalid menu') }
  /* Keep a reference. */
  applicationMenu = menu

  if (process.platform === 'darwin') {
    if (menu === null) { return }
    menu._callMenuWillShow()
    return bindings.setApplicationMenu(menu)
  } else {
    const windows = BrowserWindow.getAllWindows()
    return Array.from(windows).map((w) => w.setMenu(menu))
  }
}

Menu.getApplicationMenu = () => applicationMenu

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

Menu.buildFromTemplate = function(template) {
  if (!Array.isArray(template)) { throw new TypeError('Invalid template for Menu') }

  const positionedTemplate = []
  let insertIndex = 0

  for (var item of Array.from(template)) {
    if (item.position) {
      insertIndex = indexToInsertByPosition(positionedTemplate, item.position)
    } else {
      /* If no |position| is specified, insert after last item. */
      insertIndex++
    }
    positionedTemplate.splice(insertIndex, 0, item)
  }

  const menu = new Menu

  for (item of Array.from(positionedTemplate)) {
    if (typeof item !== 'object') { throw new TypeError('Invalid template for MenuItem') }

    const menuItem = new MenuItem(item)
    for (let key in item) {
      const value = item[key]
      if (menuItem[key] == null) {
        menuItem[key] = value
      }
    }
    menu.append(menuItem)
  }

  return menu
}

module.exports = Menu