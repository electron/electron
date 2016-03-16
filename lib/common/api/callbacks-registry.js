'use strict';

var slice = [].slice;

const v8Util = process.atomBinding('v8_util');

class CallbacksRegistry {
  constructor() {
    this.nextId = 0;
    this.callbacks = {};
  }

  add(callback) {
    // The callback is already added.
    var filenameAndLine, id, location, match, regexp, stackString;
    id = v8Util.getHiddenValue(callback, 'callbackId');
    if (id != null) {
      return id;
    }
    id = ++this.nextId;

    // Capture the location of the function and put it in the ID string,
    // so that release errors can be tracked down easily.
    regexp = /at (.*)/gi;
    stackString = (new Error).stack;
    while ((match = regexp.exec(stackString)) !== null) {
      location = match[1];
      if (location.indexOf('(native)') !== -1) {
        continue;
      }
      if (location.indexOf('atom.asar') !== -1) {
        continue;
      }
      filenameAndLine = /([^\/^\)]*)\)?$/gi.exec(location)[1];
      break;
    }
    this.callbacks[id] = callback;
    v8Util.setHiddenValue(callback, 'callbackId', id);
    v8Util.setHiddenValue(callback, 'location', filenameAndLine);
    return id;
  }

  get(id) {
    const callback = this.callbacks[id];
    return callback != null ? callback : function() {};
  }

  call() {
    const id = arguments[0];
    const args = 2 <= arguments.length ? slice.call(arguments, 1) : [];
    const callback = this.get(id);
    return callback.call.apply(callback, [global].concat(slice.call(args)));
  }

  apply() {
    const id = arguments[0];
    const args = 2 <= arguments.length ? slice.call(arguments, 1) : [];
    const callback = this.get(id);
    return callback.apply.apply(callback, [global].concat(slice.call(args)));
  }

  remove(id) {
    return delete this.callbacks[id];
  }
}

module.exports = CallbacksRegistry;
