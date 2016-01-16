'use strict';

const EventEmitter = require('events').EventEmitter;
const v8Util = process.atomBinding('v8_util');

class ObjectsRegistry extends EventEmitter {
  constructor() {
    super();

    this.setMaxListeners(Number.MAX_VALUE);
    this.nextId = 0;

    // Stores all objects by ref-counting.
    // (id) => {object, count}
    this.storage = {};

    // Stores the IDs of objects referenced by WebContents.
    // (webContentsId) => {(id) => (count)}
    this.owners = {};
  }

  // Register a new object, the object would be kept referenced until you release
  // it explicitly.
  add(webContentsId, obj) {
    var base, base1, id;
    id = this.saveToStorage(obj);

    // Remember the owner.
    if ((base = this.owners)[webContentsId] == null) {
      base[webContentsId] = {};
    }
    if ((base1 = this.owners[webContentsId])[id] == null) {
      base1[id] = 0;
    }
    this.owners[webContentsId][id]++;

    // Returns object's id
    return id;
  }

  // Get an object according to its ID.
  get(id) {
    var ref;
    return (ref = this.storage[id]) != null ? ref.object : void 0;
  }

  // Dereference an object according to its ID.
  remove(webContentsId, id) {
    var pointer;
    this.dereference(id, 1);

    // Also reduce the count in owner.
    pointer = this.owners[webContentsId];
    if (pointer == null) {
      return;
    }
    --pointer[id];
    if (pointer[id] === 0) {
      return delete pointer[id];
    }
  }

  // Clear all references to objects refrenced by the WebContents.
  clear(webContentsId) {
    var count, id, ref;
    this.emit("clear-" + webContentsId);
    if (this.owners[webContentsId] == null) {
      return;
    }
    ref = this.owners[webContentsId];
    for (id in ref) {
      count = ref[id];
      this.dereference(id, count);
    }
    return delete this.owners[webContentsId];
  }

  // Private: Saves the object into storage and assigns an ID for it.
  saveToStorage(object) {
    var id;
    id = v8Util.getHiddenValue(object, 'atomId');
    if (!id) {
      id = ++this.nextId;
      this.storage[id] = {
        count: 0,
        object: object
      };
      v8Util.setHiddenValue(object, 'atomId', id);
    }
    ++this.storage[id].count;
    return id;
  }

  // Private: Dereference the object from store.
  dereference(id, count) {
    var pointer;
    pointer = this.storage[id];
    if (pointer == null) {
      return;
    }
    pointer.count -= count;
    if (pointer.count === 0) {
      v8Util.deleteHiddenValue(pointer.object, 'atomId');
      return delete this.storage[id];
    }
  }
}

module.exports = new ObjectsRegistry;
