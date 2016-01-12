var EventEmitter, ObjectsRegistry, v8Util,
  extend = function(child, parent) { for (var key in parent) { if (hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; },
  hasProp = {}.hasOwnProperty;

EventEmitter = require('events').EventEmitter;

v8Util = process.atomBinding('v8_util');

ObjectsRegistry = (function(superClass) {
  extend(ObjectsRegistry, superClass);

  function ObjectsRegistry() {
    this.setMaxListeners(Number.MAX_VALUE);
    this.nextId = 0;

    /*
      Stores all objects by ref-counting.
      (id) => {object, count}
     */
    this.storage = {};

    /*
      Stores the IDs of objects referenced by WebContents.
      (webContentsId) => {(id) => (count)}
     */
    this.owners = {};
  }


  /*
    Register a new object, the object would be kept referenced until you release
    it explicitly.
   */

  ObjectsRegistry.prototype.add = function(webContentsId, obj) {
    var base, base1, id;
    id = this.saveToStorage(obj);

    /* Remember the owner. */
    if ((base = this.owners)[webContentsId] == null) {
      base[webContentsId] = {};
    }
    if ((base1 = this.owners[webContentsId])[id] == null) {
      base1[id] = 0;
    }
    this.owners[webContentsId][id]++;

    /* Returns object's id */
    return id;
  };


  /* Get an object according to its ID. */

  ObjectsRegistry.prototype.get = function(id) {
    var ref;
    return (ref = this.storage[id]) != null ? ref.object : void 0;
  };


  /* Dereference an object according to its ID. */

  ObjectsRegistry.prototype.remove = function(webContentsId, id) {
    var pointer;
    this.dereference(id, 1);

    /* Also reduce the count in owner. */
    pointer = this.owners[webContentsId];
    if (pointer == null) {
      return;
    }
    --pointer[id];
    if (pointer[id] === 0) {
      return delete pointer[id];
    }
  };


  /* Clear all references to objects refrenced by the WebContents. */

  ObjectsRegistry.prototype.clear = function(webContentsId) {
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
  };


  /* Private: Saves the object into storage and assigns an ID for it. */

  ObjectsRegistry.prototype.saveToStorage = function(object) {
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
  };


  /* Private: Dereference the object from store. */

  ObjectsRegistry.prototype.dereference = function(id, count) {
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
  };

  return ObjectsRegistry;

})(EventEmitter);

module.exports = new ObjectsRegistry;
