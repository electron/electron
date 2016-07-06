'use strict'

const v8Util = process.atomBinding('v8_util')
const {ipcRenderer, isPromise, CallbacksRegistry} = require('electron')

const callbacksRegistry = new CallbacksRegistry()

var includes = [].includes

var remoteObjectCache = v8Util.createIDWeakMap()

// Check for circular reference.
var isCircular = function (field, visited) {
  if (typeof field === 'object') {
    if (includes.call(visited, field)) {
      return true
    }
    visited.push(field)
  }
  return false
}

// Convert the arguments object into an array of meta data.
var wrapArgs = function (args, visited) {
  var valueToMeta
  if (visited == null) {
    visited = []
  }
  valueToMeta = function (value) {
    var field, prop, ret
    if (Array.isArray(value)) {
      return {
        type: 'array',
        value: wrapArgs(value, visited)
      }
    } else if (Buffer.isBuffer(value)) {
      return {
        type: 'buffer',
        value: Array.prototype.slice.call(value, 0)
      }
    } else if (value instanceof Date) {
      return {
        type: 'date',
        value: value.getTime()
      }
    } else if ((value != null) && typeof value === 'object') {
      if (isPromise(value)) {
        return {
          type: 'promise',
          then: valueToMeta(function (onFulfilled, onRejected) {
            value.then(onFulfilled, onRejected)
          })
        }
      } else if (v8Util.getHiddenValue(value, 'atomId')) {
        return {
          type: 'remote-object',
          id: v8Util.getHiddenValue(value, 'atomId')
        }
      }

      ret = {
        type: 'object',
        name: value.constructor != null ? value.constructor.name : 'Object',
        members: []
      }
      for (prop in value) {
        field = value[prop]
        ret.members.push({
          name: prop,
          value: valueToMeta(isCircular(field, visited) ? null : field)
        })
      }
      return ret
    } else if (typeof value === 'function' && v8Util.getHiddenValue(value, 'returnValue')) {
      return {
        type: 'function-with-return-value',
        value: valueToMeta(value())
      }
    } else if (typeof value === 'function') {
      return {
        type: 'function',
        id: callbacksRegistry.add(value),
        location: v8Util.getHiddenValue(value, 'location')
      }
    } else {
      return {
        type: 'value',
        value: value
      }
    }
  }
  return Array.prototype.slice.call(args).map(valueToMeta)
}

// Populate object's members from descriptors.
// The |ref| will be kept referenced by |members|.
// This matches |getObjectMemebers| in rpc-server.
let setObjectMembers = function (ref, object, metaId, members) {
  for (let member of members) {
    if (object.hasOwnProperty(member.name)) continue

    let descriptor = { enumerable: member.enumerable }
    if (member.type === 'method') {
      let remoteMemberFunction = function () {
        if (this && this.constructor === remoteMemberFunction) {
          // Constructor call.
          let ret = ipcRenderer.sendSync('ELECTRON_BROWSER_MEMBER_CONSTRUCTOR', metaId, member.name, wrapArgs(arguments))
          return metaToValue(ret)
        } else {
          // Call member function.
          let ret = ipcRenderer.sendSync('ELECTRON_BROWSER_MEMBER_CALL', metaId, member.name, wrapArgs(arguments))
          return metaToValue(ret)
        }
      }
      descriptor.get = function () {
        remoteMemberFunction.ref = ref  // The member should reference its object.
        return remoteMemberFunction
      }
      // Enable monkey-patch the method
      descriptor.set = function (value) {
        remoteMemberFunction = value
        return value
      }
      descriptor.configurable = true
    } else if (member.type === 'get') {
      descriptor.get = function () {
        return metaToValue(ipcRenderer.sendSync('ELECTRON_BROWSER_MEMBER_GET', metaId, member.name))
      }

      // Only set setter when it is writable.
      if (member.writable) {
        descriptor.set = function (value) {
          ipcRenderer.sendSync('ELECTRON_BROWSER_MEMBER_SET', metaId, member.name, value)
          return value
        }
      }
    }

    Object.defineProperty(object, member.name, descriptor)
  }
}

// Populate object's prototype from descriptor.
// This matches |getObjectPrototype| in rpc-server.
let setObjectPrototype = function (ref, object, metaId, descriptor) {
  if (descriptor === null) return
  let proto = {}
  setObjectMembers(ref, proto, metaId, descriptor.members)
  setObjectPrototype(ref, proto, metaId, descriptor.proto)
  Object.setPrototypeOf(object, proto)
}

// Convert meta data from browser into real value.
let metaToValue = function (meta) {
  var el, i, len, ref1, results, ret
  switch (meta.type) {
    case 'value':
      return meta.value
    case 'array':
      ref1 = meta.members
      results = []
      for (i = 0, len = ref1.length; i < len; i++) {
        el = ref1[i]
        results.push(metaToValue(el))
      }
      return results
    case 'buffer':
      return new Buffer(meta.value)
    case 'promise':
      return Promise.resolve({
        then: metaToValue(meta.then)
      })
    case 'error':
      return metaToPlainObject(meta)
    case 'date':
      return new Date(meta.value)
    case 'exception':
      throw new Error(meta.message + '\n' + meta.stack)
    default:
      if (remoteObjectCache.has(meta.id)) return remoteObjectCache.get(meta.id)

      if (meta.type === 'function') {
        // A shadow class to represent the remote function object.
        let remoteFunction = function () {
          if (this && this.constructor === remoteFunction) {
            // Constructor call.
            let obj = ipcRenderer.sendSync('ELECTRON_BROWSER_CONSTRUCTOR', meta.id, wrapArgs(arguments))
            // Returning object in constructor will replace constructed object
            // with the returned object.
            // http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
            return metaToValue(obj)
          } else {
            // Function call.
            let obj = ipcRenderer.sendSync('ELECTRON_BROWSER_FUNCTION_CALL', meta.id, wrapArgs(arguments))
            return metaToValue(obj)
          }
        }
        ret = remoteFunction
      } else {
        ret = {}
      }

      // Populate delegate members.
      setObjectMembers(ret, ret, meta.id, meta.members)
      // Populate delegate prototype.
      setObjectPrototype(ret, ret, meta.id, meta.proto)

      // Set constructor.name to object's name.
      Object.defineProperty(ret.constructor, 'name', { value: meta.name })

      // Track delegate object's life time, and tell the browser to clean up
      // when the object is GCed.
      v8Util.setRemoteObjectFreer(ret, meta.id)

      // Remember object's id.
      v8Util.setHiddenValue(ret, 'atomId', meta.id)
      remoteObjectCache.set(meta.id, ret)
      return ret
  }
}

// Construct a plain object from the meta.
var metaToPlainObject = function (meta) {
  var i, len, obj, ref1
  obj = (function () {
    switch (meta.type) {
      case 'error':
        return new Error()
      default:
        return {}
    }
  })()
  ref1 = meta.members
  for (i = 0, len = ref1.length; i < len; i++) {
    let {name, value} = ref1[i]
    obj[name] = value
  }
  return obj
}

// Browser calls a callback in renderer.
ipcRenderer.on('ELECTRON_RENDERER_CALLBACK', function (event, id, args) {
  callbacksRegistry.apply(id, metaToValue(args))
})

// A callback in browser is released.
ipcRenderer.on('ELECTRON_RENDERER_RELEASE_CALLBACK', function (event, id) {
  callbacksRegistry.remove(id)
})

// List all built-in modules in browser process.
const browserModules = require('../../browser/api/exports/electron')

// And add a helper receiver for each one.
for (let name of Object.getOwnPropertyNames(browserModules)) {
  Object.defineProperty(exports, name, {
    get: function () {
      return exports.getBuiltin(name)
    }
  })
}

// Get remote module.
exports.require = function (module) {
  return metaToValue(ipcRenderer.sendSync('ELECTRON_BROWSER_REQUIRE', module))
}

// Alias to remote.require('electron').xxx.
exports.getBuiltin = function (module) {
  return metaToValue(ipcRenderer.sendSync('ELECTRON_BROWSER_GET_BUILTIN', module))
}

// Get current BrowserWindow.
exports.getCurrentWindow = function () {
  return metaToValue(ipcRenderer.sendSync('ELECTRON_BROWSER_CURRENT_WINDOW'))
}

// Get current WebContents object.
exports.getCurrentWebContents = function () {
  return metaToValue(ipcRenderer.sendSync('ELECTRON_BROWSER_CURRENT_WEB_CONTENTS'))
}

// Get a global object in browser.
exports.getGlobal = function (name) {
  return metaToValue(ipcRenderer.sendSync('ELECTRON_BROWSER_GLOBAL', name))
}

// Get the process object in browser.
exports.__defineGetter__('process', function () {
  return exports.getGlobal('process')
})

// Create a funtion that will return the specifed value when called in browser.
exports.createFunctionWithReturnValue = function (returnValue) {
  var func
  func = function () {
    return returnValue
  }
  v8Util.setHiddenValue(func, 'returnValue', true)
  return func
}

// Get the guest WebContents from guestInstanceId.
exports.getGuestWebContents = function (guestInstanceId) {
  var meta
  meta = ipcRenderer.sendSync('ELECTRON_BROWSER_GUEST_WEB_CONTENTS', guestInstanceId)
  return metaToValue(meta)
}
