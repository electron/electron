'use strict'

const electron = require('electron')
const { EventEmitter } = require('events')
const fs = require('fs')
const v8Util = process.atomBinding('v8_util')

const { ipcMain, isPromise } = electron

const objectsRegistry = require('./objects-registry')
const bufferUtils = require('../common/buffer-utils')

const hasProp = {}.hasOwnProperty

// The internal properties of Function.
const FUNCTION_PROPERTIES = [
  'length', 'name', 'arguments', 'caller', 'prototype'
]

// The remote functions in renderer processes.
// id => Function
const rendererFunctions = v8Util.createDoubleIDWeakMap()

// Return the description of object's members:
const getObjectMembers = function (object) {
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
    let member = { name, enumerable: descriptor.enumerable, writable: false }
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
const getObjectPrototype = function (object) {
  let proto = Object.getPrototypeOf(object)
  if (proto === null || proto === Object.prototype) return null
  return {
    members: getObjectMembers(proto),
    proto: getObjectPrototype(proto)
  }
}

// Convert a real value into meta data.
const valueToMeta = function (sender, contextId, value, optimizeSimpleObject = false) {
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

const removeRemoteListenersAndLogWarning = (sender, meta, callIntoRenderer) => {
  let message = `Attempting to call a function in a renderer window that has been closed or released.` +
    `\nFunction provided here: ${meta.location}`

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

        for (const { name, value } of meta.members) {
          ret[name] = metaToValue(value)
        }
        return ret
      }
      case 'function-with-return-value':
        const returnValue = metaToValue(meta.value)
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

        const processId = sender.getProcessId()
        let callIntoRenderer = function (...args) {
          if (!sender.isDestroyed() && processId === sender.getProcessId()) {
            sender.send('ELECTRON_RENDERER_CALLBACK', contextId, meta.id, valueToMeta(sender, contextId, args))
          } else {
            removeRemoteListenersAndLogWarning(this, meta, callIntoRenderer)
          }
        }
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
  const funcMarkedAsync = v8Util.getHiddenValue(func, 'asynchronous')
  const funcPassedCallback = typeof args[args.length - 1] === 'function'
  try {
    if (funcMarkedAsync && !funcPassedCallback) {
      args.push(function (ret) {
        event.returnValue = valueToMeta(event.sender, contextId, ret, true)
      })
      func.apply(caller, args)
    } else {
      const ret = func.apply(caller, args)
      return valueToMeta(event.sender, contextId, ret, true)
    }
  } catch (error) {
    // Catch functions thrown further down in function invocation and wrap
    // them with the function name so it's easier to trace things like
    // `Error processing argument -1.`
    const funcName = func.name || 'anonymous'
    const err = new Error(`Could not call remote function '${funcName}'. Check that the function signature is correct. Underlying error: ${error.message}`)
    err.cause = error
    throw err
  }
}

const handleRemoteCommand = function (channel, handler) {
  ipcMain.on(channel, (event, contextId, ...args) => {
    let returnValue
    try {
      returnValue = handler(event, contextId, ...args)
    } catch (error) {
      returnValue = exceptionToMeta(event.sender, contextId, error)
    }
    if (returnValue !== undefined) {
      event.returnValue = returnValue
    }
  })
}

handleRemoteCommand('ELECTRON_BROWSER_REQUIRE', function (event, contextId, module) {
  return valueToMeta(event.sender, contextId, process.mainModule.require(module))
})

handleRemoteCommand('ELECTRON_BROWSER_GET_BUILTIN', function (event, contextId, module) {
  return valueToMeta(event.sender, contextId, electron[module])
})

handleRemoteCommand('ELECTRON_BROWSER_GLOBAL', function (event, contextId, name) {
  return valueToMeta(event.sender, contextId, global[name])
})

handleRemoteCommand('ELECTRON_BROWSER_CURRENT_WINDOW', function (event, contextId) {
  return valueToMeta(event.sender, contextId, event.sender.getOwnerBrowserWindow())
})

handleRemoteCommand('ELECTRON_BROWSER_CURRENT_WEB_CONTENTS', function (event, contextId) {
  return valueToMeta(event.sender, contextId, event.sender)
})

handleRemoteCommand('ELECTRON_BROWSER_CONSTRUCTOR', function (event, contextId, id, args) {
  args = unwrapArgs(event.sender, contextId, args)
  let constructor = objectsRegistry.get(id)

  if (constructor == null) {
    throwRPCError(`Cannot call constructor on missing remote object ${id}`)
  }

  return valueToMeta(event.sender, contextId, new constructor(...args))
})

handleRemoteCommand('ELECTRON_BROWSER_FUNCTION_CALL', function (event, contextId, id, args) {
  args = unwrapArgs(event.sender, contextId, args)
  let func = objectsRegistry.get(id)

  if (func == null) {
    throwRPCError(`Cannot call function on missing remote object ${id}`)
  }

  return callFunction(event, contextId, func, global, args)
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_CONSTRUCTOR', function (event, contextId, id, method, args) {
  args = unwrapArgs(event.sender, contextId, args)
  let object = objectsRegistry.get(id)

  if (object == null) {
    throwRPCError(`Cannot call constructor '${method}' on missing remote object ${id}`)
  }

  return valueToMeta(event.sender, contextId, new object[method](...args))
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_CALL', function (event, contextId, id, method, args) {
  args = unwrapArgs(event.sender, contextId, args)
  let obj = objectsRegistry.get(id)

  if (obj == null) {
    throwRPCError(`Cannot call function '${method}' on missing remote object ${id}`)
  }

  return callFunction(event, contextId, obj[method], obj, args)
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_SET', function (event, contextId, id, name, args) {
  args = unwrapArgs(event.sender, contextId, args)
  let obj = objectsRegistry.get(id)

  if (obj == null) {
    throwRPCError(`Cannot set property '${name}' on missing remote object ${id}`)
  }

  obj[name] = args[0]
  return null
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_GET', function (event, contextId, id, name) {
  let obj = objectsRegistry.get(id)

  if (obj == null) {
    throwRPCError(`Cannot get property '${name}' on missing remote object ${id}`)
  }

  return valueToMeta(event.sender, contextId, obj[name])
})

handleRemoteCommand('ELECTRON_BROWSER_DEREFERENCE', function (event, contextId, id) {
  objectsRegistry.remove(event.sender, contextId, id)
})

handleRemoteCommand('ELECTRON_BROWSER_CONTEXT_RELEASE', (event, contextId) => {
  objectsRegistry.clear(event.sender, contextId)
  return null
})

handleRemoteCommand('ELECTRON_BROWSER_GUEST_WEB_CONTENTS', function (event, contextId, guestInstanceId) {
  let guestViewManager = require('./guest-view-manager')
  return valueToMeta(event.sender, contextId, guestViewManager.getGuest(guestInstanceId))
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
      preloadError = { stack: err ? err.stack : (new Error(`Failed to load "${preloadPath}"`)).stack }
    }
  }
  event.returnValue = {
    preloadSrc,
    preloadError,
    process: {
      arch: process.arch,
      platform: process.platform,
      env: process.env,
      version: process.version,
      versions: process.versions
    }
  }
})
