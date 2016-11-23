'use strict'

const v8Util = process.atomBinding('v8_util')

class CallbacksRegistry {
  constructor () {
    this.nextId = 0
    this.callbacks = {}
  }

  add (callback) {
    // The callback is already added.
    var filenameAndLine, id, location, match, ref, regexp, stackString
    id = v8Util.getHiddenValue(callback, 'callbackId')
    if (id != null) {
      return id
    }
    id = ++this.nextId

    // Capture the location of the function and put it in the ID string,
    // so that release errors can be tracked down easily.
    regexp = /at (.*)/gi
    stackString = (new Error()).stack
    while ((match = regexp.exec(stackString)) !== null) {
      location = match[1]
      if (location.indexOf('(native)') !== -1) {
        continue
      }
      if (location.indexOf('electron.asar') !== -1) {
        continue
      }
      ref = /([^/^)]*)\)?$/gi.exec(location)
      filenameAndLine = ref[1]
      break
    }
    this.callbacks[id] = callback
    v8Util.setHiddenValue(callback, 'callbackId', id)
    v8Util.setHiddenValue(callback, 'location', filenameAndLine)
    return id
  }

  get (id) {
    var ref
    return (ref = this.callbacks[id]) != null ? ref : function () {}
  }

  call (id, ...args) {
    var ref
    return (ref = this.get(id)).call.apply(ref, [global].concat(args))
  }

  apply (id, ...args) {
    var ref
    return (ref = this.get(id)).apply.apply(ref, [global].concat(args))
  }

  remove (id) {
    const callback = this.callbacks[id]
    if (callback) {
      v8Util.deleteHiddenValue(callback, 'callbackId')
      delete this.callbacks[id]
    }
  }
}

module.exports = CallbacksRegistry
