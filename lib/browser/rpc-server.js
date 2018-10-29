'use strict'

const electron = require('electron')
const {EventEmitter} = require('events')
const fs = require('fs')
const v8Util = process.atomBinding('v8_util')

const {ipcMain, isPromise, webContents} = electron

const objectsRegistry = require('./objects-registry')
const bufferUtils = require('../common/buffer-utils')

const hasProp = {}.hasOwnProperty

// The internal properties of Function.
const FUNCTION_PROPERTIES = [
  'length', 'name', 'arguments', 'caller', 'prototype'
]

// The remote functions in renderer processes.
// id => Function
let rendererFunctions = v8Util.createDoubleIDWeakMap()

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
let valueToMeta = function (sender, contextId, value, optimizeSimpleObject = false) {
  // Determine the type of value.
  const meta = { type: typeof value }
  if (meta.type === 'object') {
    // Recognize certain types of objects.
    if (value === null) {
      meta.type = 'value'
    } else if (bufferUtils.isBuffer(value)) {
      meta.type = 'buffer'
    } else if (Array.isArray(value)) {
      meta.type = 'array'
    } else if (value instanceof Error) {
      meta.type = 'error'
    } else if (value instanceof Date) {
      meta.type = 'date'
    } else if (isPromise(value)) {
      meta.type = 'promise'
    } else if (hasProp.call(value, 'callee') && value.length != null) {
      // Treat the arguments object as array.
      meta.type = 'array'
    } else if (optimizeSimpleObject && v8Util.getHiddenValue(value, 'simple')) {
      // Treat simple objects as value.
      meta.type = 'value'
    }
  }

  // Fill the meta object according to value's type.
  if (meta.type === 'array') {
    meta.members = value.map((el) => valueToMeta(sender, contextId, el, optimizeSimpleObject))
  } else if (meta.type === 'object' || meta.type === 'function') {
    meta.name = value.constructor ? value.constructor.name : ''

    // Reference the original value if it's an object, because when it's
    // passed to renderer we would assume the renderer keeps a reference of
    // it.
    meta.id = objectsRegistry.add(sender, contextId, value)
    meta.members = getObjectMembers(value)
    meta.proto = getObjectPrototype(value)
  } else if (meta.type === 'buffer') {
    meta.value = bufferUtils.bufferToMeta(value)
  } else if (meta.type === 'promise') {
    // Add default handler to prevent unhandled rejections in main process
    // Instead they should appear in the renderer process
    value.then(function () {}, function () {})

    meta.then = valueToMeta(sender, contextId, function (onFulfilled, onRejected) {
      value.then(onFulfilled, onRejected)
    })
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
const plainObjectToMeta = function (obj) {
  return Object.getOwnPropertyNames(obj).map(function (name) {
    return {
      name: name,
      value: obj[name]
    }
  })
}

// Convert Error into meta data.
const exceptionToMeta = function (sender, contextId, error) {
  return {
    type: 'exception',
    message: error.message,
    stack: error.stack || error,
    cause: valueToMeta(sender, contextId, error.cause)
  }
}

const throwRPCError = function (message) {
  const error = new Error(message)
  error.code = 'EBADRPC'
  error.errno = -72
  throw error
}

const removeRemoteListenersAndLogWarning = (sender, callIntoRenderer) => {
  const location = v8Util.getHiddenValue(callIntoRenderer, 'location')
  let message = `Attempting to call a function in a renderer window that has been closed or released.` +
    `\nFunction provided here: ${location}`

  if (sender instanceof EventEmitter) {
    const remoteEvents = sender.eventNames().filter((eventName) => {
      return sender.listeners(eventName).includes(callIntoRenderer)
    })

    if (remoteEvents.length > 0) {
      message += `\nRemote event names: ${remoteEvents.join(', ')}`
      remoteEvents.forEach((eventName) => {
        sender.removeListener(eventName, callIntoRenderer)
      })
    }
  }

  console.warn(message)
}

// Convert array of meta data from renderer into array of real values.
const unwrapArgs = function (sender, contextId, args) {
  const metaToValue = function (meta) {
    let i, len, member, ref, returnValue
    switch (meta.type) {
      case 'value':
        return meta.value
      case 'remote-object':
        return objectsRegistry.get(meta.id)
      case 'array':
        return unwrapArgs(sender, contextId, meta.value)
      case 'buffer':
        return bufferUtils.metaToBuffer(meta.value)
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
        // Merge contextId and meta.id, since meta.id can be the same in
        // different webContents.
        const objectId = [contextId, meta.id]

        // Cache the callbacks in renderer.
        if (rendererFunctions.has(objectId)) {
          return rendererFunctions.get(objectId)
        }

        const callIntoRenderer = function (...args) {
          if (!sender.isDestroyed()) {
            sender.send('ELECTRON_RENDERER_CALLBACK', contextId, meta.id, valueToMeta(sender, contextId, args))
          } else {
            removeRemoteListenersAndLogWarning(this, callIntoRenderer)
          }
        }
        v8Util.setHiddenValue(callIntoRenderer, 'location', meta.location)
        Object.defineProperty(callIntoRenderer, 'length', { value: meta.length })

        v8Util.setRemoteCallbackFreer(callIntoRenderer, contextId, meta.id, sender)
        rendererFunctions.set(objectId, callIntoRenderer)
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
const callFunction = function (event, contextId, func, caller, args) {
  let err, funcMarkedAsync, funcName, funcPassedCallback, ref, ret
  funcMarkedAsync = v8Util.getHiddenValue(func, 'asynchronous')
  funcPassedCallback = typeof args[args.length - 1] === 'function'
  try {
    if (funcMarkedAsync && !funcPassedCallback) {
      args.push(function (ret) {
        event.returnValue = valueToMeta(event.sender, contextId, ret, true)
      })
      func.apply(caller, args)
    } else {
      ret = func.apply(caller, args)
      event.returnValue = valueToMeta(event.sender, contextId, ret, true)
    }
  } catch (error) {
    // Catch functions thrown further down in function invocation and wrap
    // them with the function name so it's easier to trace things like
    // `Error processing argument -1.`
    funcName = ((ref = func.name) != null) ? ref : 'anonymous'
    err = new Error(`Could not call remote function '${funcName}'. Check that the function signature is correct. Underlying error: ${error.message}`)
    err.cause = error
    throw err
  }
}

ipcMain.on('ELECTRON_BROWSER_WRONG_CONTEXT_ERROR', function (event, contextId, passedContextId, id) {
  const objectId = [passedContextId, id]
  if (!rendererFunctions.has(objectId)) {
    // Do nothing if the error has already been reported before.
    return
  }
  removeRemoteListenersAndLogWarning(event.sender, rendererFunctions.get(objectId))
})

ipcMain.on('ELECTRON_BROWSER_REQUIRE', function (event, contextId, module) {
  try {
    event.returnValue = valueToMeta(event.sender, contextId, process.mainModule.require(module))
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_GET_BUILTIN', function (event, contextId, module) {
  try {
    event.returnValue = valueToMeta(event.sender, contextId, electron[module])
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_GLOBAL', function (event, contextId, name) {
  try {
    event.returnValue = valueToMeta(event.sender, contextId, global[name])
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_CURRENT_WINDOW', function (event, contextId) {
  try {
    event.returnValue = valueToMeta(event.sender, contextId, event.sender.getOwnerBrowserWindow())
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_CURRENT_WEB_CONTENTS', function (event, contextId) {
  event.returnValue = valueToMeta(event.sender, contextId, event.sender)
})

ipcMain.on('ELECTRON_BROWSER_CONSTRUCTOR', function (event, contextId, id, args) {
  try {
    args = unwrapArgs(event.sender, contextId, args)
    let constructor = objectsRegistry.get(id)

    if (constructor == null) {
      throwRPCError(`Cannot call constructor on missing remote object ${id}`)
    }

    // Call new with array of arguments.
    // http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    let obj = new (Function.prototype.bind.apply(constructor, [null].concat(args)))()
    event.returnValue = valueToMeta(event.sender, contextId, obj)
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_FUNCTION_CALL', function (event, contextId, id, args) {
  try {
    args = unwrapArgs(event.sender, contextId, args)
    let func = objectsRegistry.get(id)

    if (func == null) {
      throwRPCError(`Cannot call function on missing remote object ${id}`)
    }

    callFunction(event, contextId, func, global, args)
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_CONSTRUCTOR', function (event, contextId, id, method, args) {
  try {
    args = unwrapArgs(event.sender, contextId, args)
    let object = objectsRegistry.get(id)

    if (object == null) {
      throwRPCError(`Cannot call constructor '${method}' on missing remote object ${id}`)
    }

    // Call new with array of arguments.
    let constructor = object[method]
    let obj = new (Function.prototype.bind.apply(constructor, [null].concat(args)))()
    event.returnValue = valueToMeta(event.sender, contextId, obj)
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_CALL', function (event, contextId, id, method, args) {
  try {
    args = unwrapArgs(event.sender, contextId, args)
    let obj = objectsRegistry.get(id)

    if (obj == null) {
      throwRPCError(`Cannot call function '${method}' on missing remote object ${id}`)
    }

    callFunction(event, contextId, obj[method], obj, args)
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_SET', function (event, contextId, id, name, args) {
  try {
    args = unwrapArgs(event.sender, contextId, args)
    let obj = objectsRegistry.get(id)

    if (obj == null) {
      throwRPCError(`Cannot set property '${name}' on missing remote object ${id}`)
    }

    obj[name] = args[0]
    event.returnValue = null
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_MEMBER_GET', function (event, contextId, id, name) {
  try {
    let obj = objectsRegistry.get(id)

    if (obj == null) {
      throwRPCError(`Cannot get property '${name}' on missing remote object ${id}`)
    }

    event.returnValue = valueToMeta(event.sender, contextId, obj[name])
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_DEREFERENCE', function (event, contextId, id) {
  objectsRegistry.remove(event.sender, contextId, id)
})

ipcMain.on('ELECTRON_BROWSER_CONTEXT_RELEASE', (event, contextId) => {
  objectsRegistry.clear(event.sender, contextId)
  event.returnValue = null
})

ipcMain.on('ELECTRON_BROWSER_GUEST_WEB_CONTENTS', function (event, contextId, guestInstanceId) {
  try {
    let guestViewManager = require('./guest-view-manager')
    event.returnValue = valueToMeta(event.sender, contextId, guestViewManager.getGuest(guestInstanceId))
  } catch (error) {
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', function (event, contextId, requestId, guestInstanceId, method, ...args) {
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
    event.returnValue = exceptionToMeta(event.sender, contextId, error)
  }
})

ipcMain.on('ELECTRON_BROWSER_SEND_TO', function (event, sendToAll, webContentsId, channel, ...args) {
  let contents = webContents.fromId(webContentsId)
  if (!contents) {
    console.error(`Sending message to WebContents with unknown ID ${webContentsId}`)
    return
  }

  if (sendToAll) {
    contents.sendToAll(channel, ...args)
  } else {
    contents.send(channel, ...args)
  }
})

// Implements window.close()
ipcMain.on('ELECTRON_BROWSER_WINDOW_CLOSE', function (event) {
  const window = event.sender.getOwnerBrowserWindow()
  if (window) {
    window.close()
  }
  event.returnValue = null
})

ipcMain.on('ELECTRON_BROWSER_SANDBOX_LOAD', function (event) {
  const preloadPath = event.sender._getPreloadPath()
  let preloadSrc = null
  let preloadError = null
  if (preloadPath) {
    try {
      preloadSrc = fs.readFileSync(preloadPath).toString()
    } catch (err) {
      preloadError = {stack: err ? err.stack : (new Error(`Failed to load "${preloadPath}"`)).stack}
    }
  }
  event.returnValue = {
    preloadSrc: preloadSrc,
    preloadError: preloadError,
    webContentsId: event.sender.getId(),
    platform: process.platform,
    env: process.env
  }
})
