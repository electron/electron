'use strict'

const electron = require('electron')
const ipcMain = electron.ipcMain
const objectsRegistry = require('./objects-registry')
const v8Util = process.atomBinding('v8_util')
const IDWeakMap = process.atomBinding('id_weak_map').IDWeakMap

// The internal properties of Function.
const FUNCTION_PROPERTIES = [
  'length', 'name', 'arguments', 'caller', 'prototype'
]

// The remote functions in renderer processes.
// (webContentsId) => {id: Function}
let rendererFunctions = {}

// Return the description of object's members:
let getObjectMembers = function (object) {
  let names = Object.getOwnPropertyNames(object)
  // For Function, we should not override following properties even though they
  // are "own" properties.
  if (typeof object === 'function') {
    names = names.filter((name) => {
      return !FUNCTION_PROPERTIES.includes(name)
    })
  }
  // Map properties to descriptors.
  return names.map((name) => {
    let descriptor = Object.getOwnPropertyDescriptor(object, name)
    let member = {name, enumerable: descriptor.enumerable, writable: false}
    if (descriptor.get === undefined && typeof object[name] === 'function') {
      member.type = 'method'
    } else {
      if (descriptor.set || descriptor.writable) member.writable = true
      member.type = 'get'
    }
    return member
  })
}

// Return the description of object's prototype.
let getObjectPrototype = function (object) {
  let proto = Object.getPrototypeOf(object)
  if (proto === null || proto === Object.prototype) return null
  return {
    members: getObjectMembers(proto),
    proto: getObjectPrototype(proto)
  }
}

// Convert a real value into meta data.
var valueToMeta = function (sender, value, optimizeSimpleObject) {
  var el, i, len, meta
  if (optimizeSimpleObject == null) {
    optimizeSimpleObject = false
  }
  meta = {
    type: typeof value
  }
  if (Buffer.isBuffer(value)) {
    meta.type = 'buffer'
  }
  if (value === null) {
    meta.type = 'value'
  }
  if (Array.isArray(value)) {
    meta.type = 'array'
  }
  if (value instanceof Error) {
    meta.type = 'error'
  }
  if (value instanceof Date) {
    meta.type = 'date'
  }
  if ((value != null ? value.constructor.name : void 0) === 'Promise') {
    meta.type = 'promise'
  }

  // Treat simple objects as value.
  if (optimizeSimpleObject && meta.type === 'object' && v8Util.getHiddenValue(value, 'simple')) {
    meta.type = 'value'
  }

  // Treat the arguments object as array.
  if (meta.type === 'object' && (value.hasOwnProperty('callee')) && (value.length != null)) {
    meta.type = 'array'
  }
  if (meta.type === 'array') {
    meta.members = []
    for (i = 0, len = value.length; i < len; i++) {
      el = value[i]
      meta.members.push(valueToMeta(sender, el))
    }
  } else if (meta.type === 'object' || meta.type === 'function') {
    meta.name = value.constructor.name

    // Reference the original value if it's an object, because when it's
    // passed to renderer we would assume the renderer keeps a reference of
    // it.
    meta.id = objectsRegistry.add(sender, value)
    meta.members = getObjectMembers(value)
    meta.proto = getObjectPrototype(value)
  } else if (meta.type === 'buffer') {
    meta.value = Array.prototype.slice.call(value, 0)
  } else if (meta.type === 'promise') {
    meta.then = valueToMeta(sender, function (v) { value.then(v) })
  } else if (meta.type === 'error') {
    meta.members = plainObjectToMeta(value)

    // Error.name is not part of own properties.
    meta.members.push({
      name: 'name',
      value: value.name
    })
  } else if (meta.type === 'date') {
    meta.value = value.getTime()
  } else {
    meta.type = 'value'
    meta.value = value
  }
  return meta
}

// Convert object to meta by value.
var plainObjectToMeta = function (obj) {
  return Object.getOwnPropertyNames(obj).map(function (name) {
    return {
      name: name,
      value: obj[name]
    }
  })
}

// Convert Error into meta data.
var exceptionToMeta = function (error) {
  return {
    type: 'exception',
    message: error.message,
    stack: error.stack || error
  }
}

// Convert array of meta data from renderer into array of real values.
var unwrapArgs = function (sender, args) {
  var metaToValue
  metaToValue = function (meta) {
    var i, len, member, ref, returnValue
    switch (meta.type) {
      case 'value':
        return meta.value
      case 'remote-object':
        return objectsRegistry.get(meta.id)
      case 'array':
        return unwrapArgs(sender, meta.value)
      case 'buffer':
        return new Buffer(meta.value)
      case 'date':
        return new Date(meta.value)
      case 'promise':
        return Promise.resolve({
          then: metaToValue(meta.then)
        })
      case 'object': {
        let ret = {}
        Object.defineProperty(ret.constructor, 'name', { value: meta.name })

        ref = meta.members
        for (i = 0, len = ref.length; i < len; i++) {
          member = ref[i]
          ret[member.name] = metaToValue(member.value)
        }
        return ret
      }
      case 'function-with-return-value':
        returnValue = metaToValue(meta.value)
        return function () {
          return returnValue
        }
      case 'function': {
        // Cache the callbacks in renderer.
        let webContentsId = sender.getId()
        let callbacks = rendererFunctions[webContentsId]
        if (!callbacks) {
          callbacks = rendererFunctions[webContentsId] = new IDWeakMap()
          sender.once('render-view-deleted', function (event, id) {
            callbacks.clear()
            delete rendererFunctions[id]
          })
        }

        if (callbacks.has(meta.id)) return callbacks.get(meta.id)

        let callIntoRenderer = function (...args) {
          if ((webContentsId in rendererFunctions) && !sender.isDestroyed()) {
            sender.send('ELECTRON_RENDERER_CALLBACK', meta.id, valueToMeta(sender, args))
          } else {
            throw new Error(`Attempting to call a function in a renderer window that has been closed or released. Function provided here: ${meta.location}.`)
          }
        }
        v8Util.setDestructor(callIntoRenderer, function () {
          if ((webContentsId in rendererFunctions) && !sender.isDestroyed()) {
            sender.send('ELECTRON_RENDERER_RELEASE_CALLBACK', meta.id)
          }
        })
        callbacks.set(meta.id, callIntoRenderer)
        return callIntoRenderer
      }
      default:
        throw new TypeError(`Unknown type: ${meta.type}`)
    }
  }
  return args.map(metaToValue)
}

// Call a function and send reply asynchronously if it's a an asynchronous
// style function and the caller didn't pass a callback.
var callFunction = function (event, func, caller, args) {
  var funcMarkedAsync, funcName, funcPassedCallback, ref, ret
  funcMarkedAsync = v8Util.getHiddenValue(func, 'asynchronous')
  funcPassedCallback = typeof args[args.length - 1] === 'function'
  try {
    if (funcMarkedAsync && !funcPassedCallback) {
      args.push(function (ret) {
        event.returnValue = valueToMeta(event.sender, ret, true)
      })
      return func.apply(caller, args)
    } else {
      ret = func.apply(caller, args)
      event.returnValue = valueToMeta(event.sender, ret, true)
    }
  } catch (error) {
    // Catch functions thrown further down in function invocation and wrap
    // them with the function name so it's easier to trace things like
    // `Error processing argument -1.`
    funcName = ((ref = func.name) != null) ? ref : 'anonymous'
    throw new Error(`Could not call remote function '${funcName}'. Check that the function signature is correct. Underlying error: ${error.message}`)
  }
}

ipcMain.on('ELECTRON_BROWSER_REQUIRE', function (event, module) {
  try {
    event.returnValue = valueToMeta(event.sender, process.mainModule.require(module))
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_GET_BUILTIN', function (event, module) {
  try {
    event.returnValue = valueToMeta(event.sender, electron[module])
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_GLOBAL', function (event, name) {
  try {
    event.returnValue = valueToMeta(event.sender, global[name])
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_CURRENT_WINDOW', function (event) {
  try {
    event.returnValue = valueToMeta(event.sender, event.sender.getOwnerBrowserWindow())
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_CURRENT_WEB_CONTENTS', function (event) {
  event.returnValue = valueToMeta(event.sender, event.sender)
})

ipcMain.on('ELECTRON_BROWSER_CONSTRUCTOR', function (event, id, args) {
  try {
    args = unwrapArgs(event.sender, args)
    let constructor = objectsRegistry.get(id)

    // Call new with array of arguments.
    // http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    let obj = new (Function.prototype.bind.apply(constructor, [null].concat(args)))
    event.returnValue = valueToMeta(event.sender, obj)
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_FUNCTION_CALL', function (event, id, args) {
  try {
    args = unwrapArgs(event.sender, args)
    let func = objectsRegistry.get(id)
    return callFunction(event, func, global, args)
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_CONSTRUCTOR', function (event, id, method, args) {
  try {
    args = unwrapArgs(event.sender, args)
    let constructor = objectsRegistry.get(id)[method]

    // Call new with array of arguments.
    let obj = new (Function.prototype.bind.apply(constructor, [null].concat(args)))
    event.returnValue = valueToMeta(event.sender, obj)
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_CALL', function (event, id, method, args) {
  try {
    args = unwrapArgs(event.sender, args)
    let obj = objectsRegistry.get(id)
    return callFunction(event, obj[method], obj, args)
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_SET', function (event, id, name, value) {
  try {
    let obj = objectsRegistry.get(id)
    obj[name] = value
    event.returnValue = null
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_GET', function (event, id, name) {
  try {
    let obj = objectsRegistry.get(id)
    event.returnValue = valueToMeta(event.sender, obj[name])
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_DEREFERENCE', function (event, id) {
  return objectsRegistry.remove(event.sender.getId(), id)
})

ipcMain.on('ELECTRON_BROWSER_GUEST_WEB_CONTENTS', function (event, guestInstanceId) {
  try {
    let guestViewManager = require('./guest-view-manager')
    event.returnValue = valueToMeta(event.sender, guestViewManager.getGuest(guestInstanceId))
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})

ipcMain.on('ELECTRON_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', function (event, requestId, guestInstanceId, method, ...args) {
  try {
    let guestViewManager = require('./guest-view-manager')
    let guest = guestViewManager.getGuest(guestInstanceId)
    if (requestId) {
      const responseCallback = function (result) {
        event.sender.send(`ELECTRON_RENDERER_ASYNC_CALL_TO_GUEST_VIEW_RESPONSE_${requestId}`, result)
      }
      args.push(responseCallback)
    }
    guest[method].apply(guest, args)
  } catch (error) {
    event.returnValue = exceptionToMeta(error)
  }
})
