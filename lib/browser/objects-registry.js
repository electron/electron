'use strict'

const v8Util = process.atomBinding('v8_util')

const getOwnerKey = (webContents, contextId) => {
  return `${webContents.id}-${contextId}`
}

class ObjectsRegistry {
  constructor () {
    this.nextId = 0

    // Stores all objects by ref-counting.
    // (id) => {object, count}
    this.storage = {}

    // Stores the IDs of objects referenced by WebContents.
    // (ownerKey) => [id]
    this.owners = {}
  }

  // Register a new object and return its assigned ID. If the object is already
  // registered then the already assigned ID would be returned.
  add (webContents, contextId, obj) {
    // Get or assign an ID to the object.
    const id = this.saveToStorage(obj)

    // Add object to the set of referenced objects.
    const ownerKey = getOwnerKey(webContents, contextId)
    let owner = this.owners[ownerKey]
    if (!owner) {
      owner = this.owners[ownerKey] = new Set()
      this.registerDeleteListener(webContents, contextId)
    }
    if (!owner.has(id)) {
      owner.add(id)
      // Increase reference count if not referenced before.
      this.storage[id].count++
    }
    return id
  }

  // Get an object according to its ID.
  get (id) {
    const pointer = this.storage[id]
    if (pointer != null) return pointer.object
  }

  // Dereference an object according to its ID.
  // Note that an object may be double-freed (cleared when page is reloaded, and
  // then garbage collected in old page).
  remove (webContents, contextId, id) {
    const ownerKey = getOwnerKey(webContents, contextId)
    let owner = this.owners[ownerKey]
    if (owner) {
      // Remove the reference in owner.
      owner.delete(id)
      // Dereference from the storage.
      this.dereference(id)
    }
  }

  // Clear all references to objects refrenced by the WebContents.
  clear (webContents, contextId) {
    const ownerKey = getOwnerKey(webContents, contextId)
    let owner = this.owners[ownerKey]
    if (!owner) return

    for (let id of owner) this.dereference(id)

    delete this.owners[ownerKey]
  }

  // Private: Saves the object into storage and assigns an ID for it.
  saveToStorage (object) {
    let id = v8Util.getHiddenValue(object, 'atomId')
    if (!id) {
      id = ++this.nextId
      this.storage[id] = {
        count: 0,
        object: object
      }
      v8Util.setHiddenValue(object, 'atomId', id)
    }
    return id
  }

  // Private: Dereference the object from store.
  dereference (id) {
    let pointer = this.storage[id]
    if (pointer == null) {
      return
    }
    pointer.count -= 1
    if (pointer.count === 0) {
      v8Util.deleteHiddenValue(pointer.object, 'atomId')
      delete this.storage[id]
    }
  }

  // Private: Clear the storage when renderer process is destroyed.
  registerDeleteListener (webContents, contextId) {
    // contextId => ${processHostId}-${contextCount}
    const processHostId = contextId.split('-')[0]
    const listener = (event, deletedProcessHostId) => {
      if (deletedProcessHostId &&
          deletedProcessHostId.toString() === processHostId) {
        webContents.removeListener('render-view-deleted', listener)
        this.clear(webContents, contextId)
      }
    }
    webContents.on('render-view-deleted', listener)
  }
}

module.exports = new ObjectsRegistry()
