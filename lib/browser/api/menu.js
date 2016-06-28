'use strict'

const BrowserWindow = require('electron').BrowserWindow
const MenuItem = require('electron').MenuItem
const EventEmitter = require('events').EventEmitter
const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

// Automatically generated radio menu item's group id.
var nextGroupId = 0

// Search between separators to find a radio menu item and return its group id,
// otherwise generate a group id.
var generateGroupId = function (items, pos) {
  var i, item, j, k, ref1, ref2, ref3
  if (pos > 0) {
    for (i = j = ref1 = pos - 1; ref1 <= 0 ? j <= 0 : j >= 0; i = ref1 <= 0 ? ++j : --j) {
      item = items[i]
      if (item.type === 'radio') {
        return item.groupId
      }
      if (item.type === 'separator') {
        break
      }
    }
  } else if (pos < items.length) {
    for (i = k = ref2 = pos, ref3 = items.length - 1; ref2 <= ref3 ? k <= ref3 : k >= ref3; i = ref2 <= ref3 ? ++k : --k) {
      item = items[i]
      if (item.type === 'radio') {
        return item.groupId
      }
      if (item.type === 'separator') {
        break
      }
    }
  }
  return ++nextGroupId
}

// Returns the index of item according to |id|.
var indexOfItemById = function (items, id) {
  var i, item, j, len
  for (i = j = 0, len = items.length; j < len; i = ++j) {
    item = items[i]
    if (item.id === id) {
      return i
    }
  }
  return -1
}

// Returns the index of where to insert the item according to |position|.
var indexToInsertByPosition = function (items, position) {
  var insertIndex
  if (!position) {
    return items.length
  }
  const [query, id] = position.split('=')
  insertIndex = indexOfItemById(items, id)
  if (insertIndex === -1 && query !== 'endof') {
    console.warn("Item with id '" + id + "' is not found")
    return items.length
  }
  switch (query) {
    case 'after':
      insertIndex++
      break
    case 'endof':

      // If the |id| doesn't exist, then create a new group with the |id|.
      if (insertIndex === -1) {
        items.push({
          id: id,
          type: 'separator'
        })
        insertIndex = items.length - 1
      }

      // Find the end of the group.
      insertIndex++
      while (insertIndex < items.length && items[insertIndex].type !== 'separator') {
        insertIndex++
      }
  }
  return insertIndex
}

const Menu = bindings.Menu

Object.setPrototypeOf(Menu.prototype, EventEmitter.prototype)

Menu.prototype._init = function () {
  this.commandsMap = {}
  this.groupsMap = {}
  this.items = []
  this.delegate = {
    isCommandIdChecked: (commandId) => {
      var command = this.commandsMap[commandId]
      return command != null ? command.checked : undefined
    },
    isCommandIdEnabled: (commandId) => {
      var command = this.commandsMap[commandId]
      return command != null ? command.enabled : undefined
    },
    isCommandIdVisible: (commandId) => {
      var command = this.commandsMap[commandId]
      return command != null ? command.visible : undefined
    },
    getAcceleratorForCommandId: (commandId, context) => {
      const command = this.commandsMap[commandId]
      if (command == null) return
      if (command.accelerator != null) return command.accelerator
      if (context !== 'tray') return command.getDefaultRoleAccelerator()
    },
    getIconForCommandId: (commandId) => {
      var command = this.commandsMap[commandId]
      return command != null ? command.icon : undefined
    },
    executeCommand: (event, commandId, context) => {
      const command = this.commandsMap[commandId]
      if (command != null) {
        event.menuType = context
        command.click(event, BrowserWindow.getFocusedWindow(), context)
      }
    },
    menuWillShow: () => {
      // Make sure radio groups have at least one menu item seleted.
      var checked, group, id, j, len, radioItem, ref1
      ref1 = this.groupsMap
      for (id in ref1) {
        group = ref1[id]
        checked = false
        for (j = 0, len = group.length; j < len; j++) {
          radioItem = group[j]
          if (!radioItem.checked) {
            continue
          }
          checked = true
          break
        }
        if (!checked) {
          v8Util.setHiddenValue(group[0], 'checked', true)
        }
      }
    }
  }
}

Menu.prototype.popup = function (window, x, y, positioningItem) {
  if (typeof window !== 'object' || window.constructor !== BrowserWindow) {
    // Shift.
    positioningItem = y
    y = x
    x = window
    window = BrowserWindow.getFocusedWindow()
  }

  // Default to showing under mouse location.
  if (typeof x !== 'number') x = -1
  if (typeof y !== 'number') y = -1

  // Default to not highlighting any item.
  if (typeof positioningItem !== 'number') positioningItem = -1

  this.popupAt(window, x, y, positioningItem)
}

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
  if (item.sublabel != null) {
    this.setSublabel(pos, item.sublabel)
  }
  if (item.icon != null) {
    this.setIcon(pos, item.icon)
  }
  if (item.role != null) {
    this.setRole(pos, item.role)
  }

  // Make menu accessable to items.
  item.overrideReadOnlyProperty('menu', this)

  // Remember the items.
  this.items.splice(pos, 0, item)
  this.commandsMap[item.commandId] = item
}

// Force menuWillShow to be called
Menu.prototype._callMenuWillShow = function () {
  if (this.delegate != null) {
    this.delegate.menuWillShow()
  }
  this.items.forEach(function (item) {
    if (item.submenu != null) {
      item.submenu._callMenuWillShow()
    }
  })
}

var applicationMenu = null

Menu.setApplicationMenu = function (menu) {
  if (!(menu === null || menu.constructor === Menu)) {
    throw new TypeError('Invalid menu')
  }

  // Keep a reference.
  applicationMenu = menu
  if (process.platform === 'darwin') {
    if (menu === null) {
      return
    }
    menu._callMenuWillShow()
    bindings.setApplicationMenu(menu)
  } else {
    BrowserWindow.getAllWindows().forEach(function (window) {
      window.setMenu(menu)
    })
  }
}

Menu.getApplicationMenu = function () {
  return applicationMenu
}

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

Menu.buildFromTemplate = function (template) {
  var insertIndex, item, j, k, key, len, len1, menu, menuItem, positionedTemplate
  if (!Array.isArray(template)) {
    throw new TypeError('Invalid template for Menu')
  }
  positionedTemplate = []
  insertIndex = 0
  for (j = 0, len = template.length; j < len; j++) {
    item = template[j]
    if (item.position) {
      insertIndex = indexToInsertByPosition(positionedTemplate, item.position)
    } else {
      // If no |position| is specified, insert after last item.
      insertIndex++
    }
    positionedTemplate.splice(insertIndex, 0, item)
  }
  menu = new Menu()
  for (k = 0, len1 = positionedTemplate.length; k < len1; k++) {
    item = positionedTemplate[k]
    if (typeof item !== 'object') {
      throw new TypeError('Invalid template for MenuItem')
    }
    menuItem = new MenuItem(item)
    for (key in item) {
      // Preserve extra fields specified by user
      if (!menuItem.hasOwnProperty(key)) {
        menuItem[key] = item[key]
      }
    }
    menu.append(menuItem)
  }
  return menu
}

module.exports = Menu
