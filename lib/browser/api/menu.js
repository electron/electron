'use strict'

const {TopLevelWindow, MenuItem, webContents} = require('electron')
const EventEmitter = require('events').EventEmitter
const v8Util = process.atomBinding('v8_util')
const bindings = process.atomBinding('menu')

const {Menu} = bindings
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

Menu.prototype.popup = function (options) {
  if (options == null || typeof options !== 'object') {
    throw new TypeError('Options must be an object')
  }
  let {window, x, y, positioningItem, callback} = options

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
    throw new TypeError('Invalid template for Menu')
  }

  const menu = new Menu()
  const filtered = removeExtraSeparators(template)
  const sorted = sortTemplate(filtered)

  sorted.forEach((item) => {
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

function sortTemplate (template) {
  const sorted = sortMenuItems(template)
  for (let id in template) {
    const item = sorted[id]
    if (Array.isArray(item.submenu)) {
      item.submenu = sortTemplate(item.submenu)
    }
  }
  return sorted
}

function splitArray (arr, predicate) {
  let lastArr = []
  const multiArr = [lastArr]
  arr.forEach(item => {
    if (predicate(item)) {
      if (lastArr.length > 0) {
        lastArr = []
        multiArr.push(lastArr)
      }
    } else {
      lastArr.push(item)
    }
  })
  return multiArr
}

function joinArrays (arrays, joiner) {
  const joinedArr = []
  arrays.forEach((arr, i) => {
    if (i > 0 && arr.length > 0) {
      joinedArr.push(joiner)
    }
    joinedArr.push(...arr)
  })
  return joinedArr
}

const pushOntoMultiMap = (map, key, value) => {
  if (!map.has(key)) {
    map.set(key, [])
  }
  map.get(key).push(value)
}

function indexOfGroupContainingCommand (groups, command, ignoreGroup) {
  return groups.findIndex(
    candiateGroup =>
      candiateGroup !== ignoreGroup &&
      candiateGroup.some(
        candidateItem => candidateItem.command === command
      )
  )
}

// Sort nodes topologically using a depth-first approach. Encountered cycles
// are broken.
function sortTopologically (originalOrder, edgesById) {
  const sorted = []
  const marked = new Set()

  function visit (id) {
    if (marked.has(id)) return
    marked.add(id)
    const edges = edgesById.get(id)
    if (edges != null) {
      edges.forEach(visit)
    }
    sorted.push(id)
  }

  originalOrder.forEach(visit)
  return sorted
}

function attemptToMergeAGroup (groups) {
  for (let i = 0; i < groups.length; i++) {
    const group = groups[i]
    for (const item of group) {
      const toCommands = [...(item.before || []), ...(item.after || [])]
      for (const command of toCommands) {
        const index = indexOfGroupContainingCommand(groups, command, group)
        if (index === -1) continue
        const mergeTarget = groups[index]
        // Merge with group containing `command`
        mergeTarget.push(...group)
        groups.splice(i, 1)
        return true
      }
    }
  }
  return false
}

// Merge groups based on before/after positions
// Mutates both the array of groups, and the individual group arrays.
function mergeGroups (groups) {
  let mergedAGroup = true
  while (mergedAGroup) {
    mergedAGroup = attemptToMergeAGroup(groups)
  }
  return groups
}

function sortItemsInGroup (group) {
  const originalOrder = group.map((node, i) => i)
  const edges = new Map()
  const commandToIndex = new Map(group.map((item, i) => [item.command, i]))

  group.forEach((item, i) => {
    if (item.before) {
      item.before.forEach(toCommand => {
        const to = commandToIndex.get(toCommand)
        if (to != null) {
          pushOntoMultiMap(edges, to, i)
        }
      })
    }
    if (item.after) {
      item.after.forEach(toCommand => {
        const to = commandToIndex.get(toCommand)
        if (to != null) {
          pushOntoMultiMap(edges, i, to)
        }
      })
    }
  })

  const sortedNodes = sortTopologically(originalOrder, edges)

  return sortedNodes.map(i => group[i])
}

function findEdgesInGroup (groups, i, edges) {
  const group = groups[i]
  for (const item of group) {
    if (item.beforeGroupContaining) {
      for (const command of item.beforeGroupContaining) {
        const to = indexOfGroupContainingCommand(groups, command, group)
        if (to !== -1) {
          pushOntoMultiMap(edges, to, i)
          return
        }
      }
    }
    if (item.afterGroupContaining) {
      for (const command of item.afterGroupContaining) {
        const to = indexOfGroupContainingCommand(groups, command, group)
        if (to !== -1) {
          pushOntoMultiMap(edges, i, to)
          return
        }
      }
    }
  }
}

function sortGroups (groups) {
  const originalOrder = groups.map((item, i) => i)
  const edges = new Map()

  for (let i = 0; i < groups.length; i++) {
    findEdgesInGroup(groups, i, edges)
  }

  const sortedGroupIndexes = sortTopologically(originalOrder, edges)
  return sortedGroupIndexes.map(i => groups[i])
}

function isSeparator (item) {
  return item.type === 'separator'
}

function sortMenuItems (menuItems) {
  // Split the items into their implicit groups based upon separators.
  const groups = splitArray(menuItems, isSeparator)
  // Merge groups that contain before/after references to eachother.
  const mergedGroups = mergeGroups(groups)
  // Sort each individual group internally.
  const mergedGroupsWithSortedItems = mergedGroups.map(sortItemsInGroup)
  // Sort the groups based upon their beforeGroupContaining/afterGroupContaining
  // references.
  const sortedGroups = sortGroups(mergedGroupsWithSortedItems)
  // Join the groups back
  return joinArrays(sortedGroups, { type: 'separator' })
}

module.exports = Menu
