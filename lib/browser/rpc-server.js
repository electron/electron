'use strict'

const electron = require('electron')
const { EventEmitter } = require('events')
const fs = require('fs')

const v8Util = process.electronBinding('v8_util')
const eventBinding = process.electronBinding('event')
const clipboard = process.electronBinding('clipboard')
const features = process.electronBinding('features')

const { isPromise } = electron

const { getContentScripts } = require('@electron/internal/browser/chrome-extension')
const { crashReporterInit } = require('@electron/internal/browser/crash-reporter-init')
const { ipcMainInternal } = require('@electron/internal/browser/ipc-main-internal')
const ipcMainUtils = require('@electron/internal/browser/ipc-main-internal-utils')
const objectsRegistry = require('@electron/internal/browser/objects-registry')
const guestViewManager = require('@electron/internal/browser/guest-view-manager')
const bufferUtils = require('@electron/internal/common/buffer-utils')
const errorUtils = require('@electron/internal/common/error-utils')
const clipboardUtils = require('@electron/internal/common/clipboard-utils')

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
    const descriptor = Object.getOwnPropertyDescriptor(object, name)
    const member = { name, enumerable: descriptor.enumerable, writable: false }
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
  const proto = Object.getPrototypeOf(object)
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
const exceptionToMeta = function (error) {
  return {
    type: 'exception',
    value: errorUtils.serialize(error)
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
const unwrapArgs = function (sender, frameId, contextId, args) {
  const metaToValue = function (meta) {
    switch (meta.type) {
      case 'value':
        return meta.value
      case 'remote-object':
        return objectsRegistry.get(meta.id)
      case 'array':
        return unwrapArgs(sender, frameId, contextId, meta.value)
      case 'buffer':
        return bufferUtils.metaToBuffer(meta.value)
      case 'date':
        return new Date(meta.value)
      case 'promise':
        return Promise.resolve({
          then: metaToValue(meta.then)
        })
      case 'object': {
        const ret = {}
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

        const callIntoRenderer = function (...args) {
          let succeed = false
          if (!sender.isDestroyed()) {
            succeed = sender._sendToFrameInternal(frameId, 'ELECTRON_RENDERER_CALLBACK', contextId, meta.id, valueToMeta(sender, contextId, args))
          }
          if (!succeed) {
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

const isRemoteModuleEnabledCache = new WeakMap()

const isRemoteModuleEnabled = function (contents) {
  if (!isRemoteModuleEnabledCache.has(contents)) {
    isRemoteModuleEnabledCache.set(contents, contents._isRemoteModuleEnabled())
  }

  return isRemoteModuleEnabledCache.get(contents)
}

const handleRemoteCommand = function (channel, handler) {
  ipcMainInternal.on(channel, (event, contextId, ...args) => {
    let returnValue
    if (!isRemoteModuleEnabled(event.sender)) {
      event.returnValue = null
      return
    }

    try {
      returnValue = handler(event, contextId, ...args)
    } catch (error) {
      returnValue = exceptionToMeta(error)
    }

    if (returnValue !== undefined) {
      event.returnValue = returnValue
    }
  })
}

const emitCustomEvent = function (contents, eventName, ...args) {
  const event = eventBinding.createWithSender(contents)

  electron.app.emit(eventName, event, contents, ...args)
  contents.emit(eventName, event, ...args)

  return event
}

handleRemoteCommand('ELECTRON_BROWSER_WRONG_CONTEXT_ERROR', function (event, contextId, passedContextId, id) {
  const objectId = [passedContextId, id]
  if (!rendererFunctions.has(objectId)) {
    // Do nothing if the error has already been reported before.
    return
  }
  removeRemoteListenersAndLogWarning(event.sender, rendererFunctions.get(objectId))
})

handleRemoteCommand('ELECTRON_BROWSER_REQUIRE', function (event, contextId, moduleName) {
  const customEvent = emitCustomEvent(event.sender, 'remote-require', moduleName)

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.require('${moduleName}')`)
    } else {
      customEvent.returnValue = process.mainModule.require(moduleName)
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue)
})

handleRemoteCommand('ELECTRON_BROWSER_GET_BUILTIN', function (event, contextId, moduleName) {
  const customEvent = emitCustomEvent(event.sender, 'remote-get-builtin', moduleName)

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.getBuiltin('${moduleName}')`)
    } else {
      customEvent.returnValue = electron[moduleName]
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue)
})

handleRemoteCommand('ELECTRON_BROWSER_GLOBAL', function (event, contextId, globalName) {
  const customEvent = emitCustomEvent(event.sender, 'remote-get-global', globalName)

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.getGlobal('${globalName}')`)
    } else {
      customEvent.returnValue = global[globalName]
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue)
})

handleRemoteCommand('ELECTRON_BROWSER_CURRENT_WINDOW', function (event, contextId) {
  const customEvent = emitCustomEvent(event.sender, 'remote-get-current-window')

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error('Blocked remote.getCurrentWindow()')
    } else {
      customEvent.returnValue = event.sender.getOwnerBrowserWindow()
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue)
})

handleRemoteCommand('ELECTRON_BROWSER_CURRENT_WEB_CONTENTS', function (event, contextId) {
  const customEvent = emitCustomEvent(event.sender, 'remote-get-current-web-contents')

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error('Blocked remote.getCurrentWebContents()')
    } else {
      customEvent.returnValue = event.sender
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue)
})

handleRemoteCommand('ELECTRON_BROWSER_CONSTRUCTOR', function (event, contextId, id, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args)
  const constructor = objectsRegistry.get(id)

  if (constructor == null) {
    throwRPCError(`Cannot call constructor on missing remote object ${id}`)
  }

  return valueToMeta(event.sender, contextId, new constructor(...args))
})

handleRemoteCommand('ELECTRON_BROWSER_FUNCTION_CALL', function (event, contextId, id, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args)
  const func = objectsRegistry.get(id)

  if (func == null) {
    throwRPCError(`Cannot call function on missing remote object ${id}`)
  }

  try {
    return valueToMeta(event.sender, contextId, func(...args), true)
  } catch (error) {
    const err = new Error(`Could not call remote function '${func.name || 'anonymous'}'. Check that the function signature is correct. Underlying error: ${error.message}\nUnderlying stack: ${error.stack}\n`)
    err.cause = error
    throw err
  }
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_CONSTRUCTOR', function (event, contextId, id, method, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args)
  const object = objectsRegistry.get(id)

  if (object == null) {
    throwRPCError(`Cannot call constructor '${method}' on missing remote object ${id}`)
  }

  return valueToMeta(event.sender, contextId, new object[method](...args))
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_CALL', function (event, contextId, id, method, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args)
  const object = objectsRegistry.get(id)

  if (object == null) {
    throwRPCError(`Cannot call method '${method}' on missing remote object ${id}`)
  }

  try {
    return valueToMeta(event.sender, contextId, object[method](...args), true)
  } catch (error) {
    const err = new Error(`Could not call remote method '${method}'. Check that the method signature is correct. Underlying error: ${error.message}\nUnderlying stack: ${error.stack}\n`)
    err.cause = error
    throw err
  }
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_SET', function (event, contextId, id, name, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args)
  const obj = objectsRegistry.get(id)

  if (obj == null) {
    throwRPCError(`Cannot set property '${name}' on missing remote object ${id}`)
  }

  obj[name] = args[0]
  return null
})

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_GET', function (event, contextId, id, name) {
  const obj = objectsRegistry.get(id)

  if (obj == null) {
    throwRPCError(`Cannot get property '${name}' on missing remote object ${id}`)
  }

  return valueToMeta(event.sender, contextId, obj[name])
})

handleRemoteCommand('ELECTRON_BROWSER_DEREFERENCE', function (event, contextId, id, rendererSideRefCount) {
  objectsRegistry.remove(event.sender, contextId, id, rendererSideRefCount)
})

handleRemoteCommand('ELECTRON_BROWSER_CONTEXT_RELEASE', (event, contextId) => {
  objectsRegistry.clear(event.sender, contextId)
  return null
})

handleRemoteCommand('ELECTRON_BROWSER_GUEST_WEB_CONTENTS', function (event, contextId, guestInstanceId) {
  const guest = guestViewManager.getGuestForWebContents(guestInstanceId, event.sender)

  const customEvent = emitCustomEvent(event.sender, 'remote-get-guest-web-contents', guest)

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.getGuestForWebContents()`)
    } else {
      customEvent.returnValue = guest
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue)
})

// Implements window.close()
ipcMainInternal.on('ELECTRON_BROWSER_WINDOW_CLOSE', function (event) {
  const window = event.sender.getOwnerBrowserWindow()
  if (window) {
    window.close()
  }
  event.returnValue = null
})

ipcMainUtils.handle('ELECTRON_CRASH_REPORTER_INIT', function (event, options) {
  return crashReporterInit(options)
})

ipcMainUtils.handle('ELECTRON_BROWSER_GET_LAST_WEB_PREFERENCES', function (event) {
  return event.sender.getLastWebPreferences()
})

// Methods not listed in this set are called directly in the renderer process.
const allowedClipboardMethods = (() => {
  switch (process.platform) {
    case 'darwin':
      return new Set(['readFindText', 'writeFindText'])
    case 'linux':
      return new Set(Object.keys(clipboard))
    default:
      return new Set()
  }
})()

ipcMainUtils.handle('ELECTRON_BROWSER_CLIPBOARD', function (event, method, ...args) {
  if (!allowedClipboardMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`)
  }

  return clipboardUtils.serialize(electron.clipboard[method](...clipboardUtils.deserialize(args)))
})

if (features.isDesktopCapturerEnabled()) {
  const desktopCapturer = require('@electron/internal/browser/desktop-capturer')

  ipcMainUtils.handle('ELECTRON_BROWSER_DESKTOP_CAPTURER_GET_SOURCES', function (event, ...args) {
    const customEvent = emitCustomEvent(event.sender, 'desktop-capturer-get-sources')

    if (customEvent.defaultPrevented) {
      console.error('Blocked desktopCapturer.getSources()')
      return []
    }

    return desktopCapturer.getSources(event, ...args)
  })
}

const getPreloadScript = async function (preloadPath) {
  let preloadSrc = null
  let preloadError = null
  try {
    preloadSrc = (await fs.promises.readFile(preloadPath)).toString()
  } catch (err) {
    preloadError = errorUtils.serialize(err)
  }
  return { preloadPath, preloadSrc, preloadError }
}

ipcMainUtils.handle('ELECTRON_GET_CONTENT_SCRIPTS', () => getContentScripts())

ipcMainUtils.handle('ELECTRON_BROWSER_SANDBOX_LOAD', async function (event) {
  const preloadPaths = event.sender._getPreloadPaths()

  return {
    contentScripts: getContentScripts(),
    preloadScripts: await Promise.all(preloadPaths.map(path => getPreloadScript(path))),
    process: {
      arch: process.arch,
      platform: process.platform,
      env: process.env,
      version: process.version,
      versions: process.versions,
      execPath: process.helperExecPath
    }
  }
})

ipcMainInternal.on('ELECTRON_BROWSER_PRELOAD_ERROR', function (event, preloadPath, error) {
  event.sender.emit('preload-error', event, preloadPath, errorUtils.deserialize(error))
})
