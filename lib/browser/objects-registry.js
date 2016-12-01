'use strict'

const v8Util = process.atomBinding('v8_util')

class ObjectsRegistry {
  constructor () {
    this.nextId = 0

    // Stores all objects by ref-counting.
    // (id) => {object, count}
    this.storage = {}

    // Stores the IDs of objects referenced by WebContents.
    // (webContentsId) => [id]
    this.owners = {}
  }

  // Register a new object and return its assigned ID. If the object is already
  // registered then the already assigned ID would be returned.
  add (webContents, obj) {
    // Get or assign an ID to the object.
    const id = this.saveToStorage(obj)

    // Add object to the set of referenced objects.
    const webContentsId = webContents.getId()
    let owner = this.owners[webContentsId]
    if (!owner) {
      owner = this.owners[webContentsId] = new Set()
      this.registerDeleteListener(webContents, webContentsId)
    }
    if (!owner.has(id)) {
      owner.add(id)
      // Increase reference count if not referenced before.
      this.storage[id].count++
    }
    return id
  }

  // Clear the storage when webContents is reloaded/navigated.
  registerDeleteListener (webContents, webContentsId) {
    const processId = webContents.getProcessId()
    const listener = (event, deletedProcessId) => {
      if (deletedProcessId === processId) {
        webContents.removeListener('render-view-deleted', listener)
        this.clear(webContentsId)
      }
    }
    webContents.on('render-view-deleted', listener)
  }

  // Get an object according to its ID.
  get (id) {
    return this.storage[id].object
  }

  // Dereference an object according to its ID.
  remove (webContentsId, id) {
    // Dereference from the storage.
    this.dereference(id)

    // Also remove the reference in owner.
    let owner = this.owners[webContentsId]
    if (owner) {
      owner.delete(id)
    }
  }

  // Clear all references to objects refrenced by the WebContents.
  clear (webContentsId) {
    let owner = this.owners[webContentsId]
    if (!owner) return

    for (let id of owner) this.dereference(id)

    delete this.owners[webContentsId]
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
}

module.exports = new ObjectsRegistry()
