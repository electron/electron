'use strict'

const {BrowserWindow, MenuItem, webContents} = require('electron')
const EventEmitter = require('events').EventEmitter
const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

// Automatically generated radio menu item's group id.
let nextGroupId = 0
let applicationMenu = null

const Menu = bindings.Menu

class Menu extends EventEmitter {
  constructor () {
    this.commandsMap = {}
    this.groupsMap = {}
    this.items = []
    this.delegate = {
      isCommandIdChecked: (commandId) => {
        const command = this.commandsMap[commandId]
        return command != null ? command.checked : undefined
      },
      isCommandIdEnabled: (commandId) => {
        const command = this.commandsMap[commandId]
        return command != null ? command.enabled : undefined
      },
      isCommandIdVisible: (commandId) => {
        const command = this.commandsMap[commandId]
        return command != null ? command.visible : undefined
      },
      getAcceleratorForCommandId: (commandId, useDefaultAccelerator) => {
        const command = this.commandsMap[commandId]
        if (command == null) return
        if (command.accelerator != null) return command.accelerator
        if (useDefaultAccelerator) return command.getDefaultRoleAccelerator()
      },
      getIconForCommandId: (commandId) => {
        const command = this.commandsMap[commandId]
        return command != null ? command.icon : undefined
      },
      executeCommand: (event, commandId) => {
        const command = this.commandsMap[commandId]
        if (command == null) return
        command.click(event, BrowserWindow.getFocusedWindow(), webContents.getFocusedWebContents())
      },
      menuWillShow: () => {
        // Make sure radio groups have at least one menu item selected
        for (let id in this.groupsMap) {
          const group = this.groupsMap[id]
          const checked = false
          for (let idx = 0; idx < group.length; idx++) {
            const radioItem = group[idx]
            if (!radioItem.checked) continue
            checked = true
            break
          }
          if (!checked) v8Util.setHiddenValue(group[0], 'checked', true)
        }
      }
  }
  popup (window, x, y, positioningItem) {
    let [newX, newY, newPositioningItem] = [x, y, positioningItem]
    let asyncPopup

    // menu.popup(x, y, positioningItem)
    if (window != null) {
      if (typeof window !== 'object' || window.constructor !== BrowserWindow) {
        [newPositioningItem, newY, newX] = [y, x, window]
        window = null
      }
    }

    // menu.popup(window, {})
    if (x != null && typeof x === 'object') {
      [newX, newY, newPositioningItem] = [x.x, x.y, x.positioningItem]
      asyncPopup = x.async
    }

    // set up defaults
    if (window == null) window = BrowserWindow.getFocusedWindow()
    if (typeof x !== 'number') newX = -1
    if (typeof y !== 'number') newY = -1
    if (typeof positioningItem !== 'number') newPositioningItem = -1
    if (typeof asyncPopup !== 'boolean') asyncPopup = false

    this.popupAt(window, newX, newY, newPositioningItem, asyncPopup)
  }
  closePopup (window) {
    if (window == null || window.constructor !== BrowserWindow) {
      window = BrowserWindow.getFocusedWindow()
    }

    if (window != null) this.closePopupAt(window.id)
  }
  getMenuItemById (id) {
    const items = this.items

    let found = items.find(item => item.id === id) || null
    for (let i = 0, length = items.length; !found && i < length; i++) {
      if (items[i].submenu) {
        found = items[i].submenu.getMenuItemById(id)
      }
    }
    return found
  }
  append (item) {
    return this.insert(this.getItemCount(), item)
  }
  insert (pos, item) {
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
  _callMenuWillShow () {
    if (this.delegate != null) {
      this.delegate.menuWillShow()
    }
    this.items.forEach(function (item) {
      if (item.submenu != null) {
        item.submenu._callMenuWillShow()
      }
    })
  }
  static setApplicationMenu (menu) {
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
  static getApplicationMenu () {
    return applicationMenu
  }
  static buildFromTemplate (template) {
    if (!Array.isArray(template)) throw new TypeError('Invalid template for Menu')

    const menu = new Menu()
    let positioned = []
    let idx = 0

    template.forEach((item) => {
      idx = (item.position) ? indexToInsertByPosition(positioned, item.position) : idx += 1
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
}

Menu.sendActionToFirstResponder = bindings.sendActionToFirstResponder

/* HELPER METHODS */

// Search between separators to find a radio menu item and return its group id,
function generateGroupId (items, pos) {
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
function indexOfItemById (items, id) {
  for (let idx = 0; idx < items.length; idx += 1) {
    const item = items[idx]
    if (item.id === id) return idx
  }
  return -1
}

// Returns the index of where to insert the item according to |position|.
function indexToInsertByPosition (items, pos) {
  if (!pos) return items.length

  const [query, id] = pos.split('=')
  let idx = indexOfItemById(items, id)

  if (idx === -1 && query !== 'endof') {
    console.warn("Item with id '" + id + "' is not found")
    return items.length
  }

  if (query === 'after') {
    idx += 1
  } else if (query === 'endof') {
    // create new group with id if none exists
    if (idx === -1) {
      items.push({
        id,
        type: 'separator'
      })
      idx = items.length - 1
    }
    idx += 1

    // search for end of group
    while (idx < items.length && items[idx].type !== 'separator') {
      idx += 1
    }
  }
  return idx
}

module.exports = Menu
