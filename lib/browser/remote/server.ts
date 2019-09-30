'use strict'

import * as electron from 'electron'
import { EventEmitter } from 'events'
import { isBuffer, bufferToMeta, BufferMeta, metaToBuffer } from '@electron/internal/common/remote/buffer-utils'
import objectsRegistry from './objects-registry'
import { ipcMainInternal } from '../ipc-main-internal'
import * as guestViewManager from '@electron/internal/browser/guest-view-manager'
import * as errorUtils from '@electron/internal/common/error-utils'
import { isPromise } from '@electron/internal/common/remote/is-promise'

const v8Util = process.electronBinding('v8_util')
const eventBinding = process.electronBinding('event')
const features = process.electronBinding('features')

if (!features.isRemoteModuleEnabled()) {
  throw new Error('remote module is disabled')
}

const hasProp = {}.hasOwnProperty

// The internal properties of Function.
const FUNCTION_PROPERTIES = [
  'length', 'name', 'arguments', 'caller', 'prototype'
]

// The remote functions in renderer processes.
// id => Function
const rendererFunctions = v8Util.createDoubleIDWeakMap()

type ObjectMember = {
  name: string,
  value?: any,
  enumerable?: boolean,
  writable?: boolean,
  type?: 'method' | 'get'
}

// Return the description of object's members:
const getObjectMembers = function (object: any): ObjectMember[] {
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
    const descriptor = Object.getOwnPropertyDescriptor(object, name)!
    let type: ObjectMember['type']
    let writable = false
    if (descriptor.get === undefined && typeof object[name] === 'function') {
      type = 'method'
    } else {
      if (descriptor.set || descriptor.writable) writable = true
      type = 'get'
    }
    return { name, enumerable: descriptor.enumerable, writable, type }
  })
}

type ObjProtoDescriptor = {
  members: ObjectMember[],
  proto: ObjProtoDescriptor
} | null

// Return the description of object's prototype.
const getObjectPrototype = function (object: any): ObjProtoDescriptor {
  const proto = Object.getPrototypeOf(object)
  if (proto === null || proto === Object.prototype) return null
  return {
    members: getObjectMembers(proto),
    proto: getObjectPrototype(proto)
  }
}

type MetaType = {
  type: 'number',
  value: number
} | {
  type: 'boolean',
  value: boolean
} | {
  type: 'string',
  value: string
} | {
  type: 'bigint',
  value: bigint
} | {
  type: 'symbol',
  value: symbol
} | {
  type: 'undefined',
  value: undefined
} | {
  type: 'object' | 'function',
  name: string,
  members: ObjectMember[],
  proto: ObjProtoDescriptor,
  id: number,
} | {
  type: 'value',
  value: any,
} | {
  type: 'buffer',
  value: BufferMeta,
} | {
  type: 'array',
  members: MetaType[]
} | {
  type: 'error',
  members: ObjectMember[]
} | {
  type: 'date',
  value: number
} | {
  type: 'promise',
  then: MetaType
}

// Convert a real value into meta data.
const valueToMeta = function (sender: electron.WebContents, contextId: string, value: any, optimizeSimpleObject = false): MetaType {
  // Determine the type of value.
  let type: MetaType['type'] = typeof value
  if (type === 'object') {
    // Recognize certain types of objects.
    if (value === null) {
      type = 'value'
    } else if (isBuffer(value)) {
      type = 'buffer'
    } else if (Array.isArray(value)) {
      type = 'array'
    } else if (value instanceof Error) {
      type = 'error'
    } else if (value instanceof Date) {
      type = 'date'
    } else if (isPromise(value)) {
      type = 'promise'
    } else if (hasProp.call(value, 'callee') && value.length != null) {
      // Treat the arguments object as array.
      type = 'array'
    } else if (optimizeSimpleObject && v8Util.getHiddenValue(value, 'simple')) {
      // Treat simple objects as value.
      type = 'value'
    }
  }

  // Fill the meta object according to value's type.
  if (type === 'array') {
    return {
      type,
      members: value.map((el: any) => valueToMeta(sender, contextId, el, optimizeSimpleObject))
    }
  } else if (type === 'object' || type === 'function') {
    return {
      type,
      name: value.constructor ? value.constructor.name : '',
      // Reference the original value if it's an object, because when it's
      // passed to renderer we would assume the renderer keeps a reference of
      // it.
      id: objectsRegistry.add(sender, contextId, value),
      members: getObjectMembers(value),
      proto: getObjectPrototype(value)
    }
  } else if (type === 'buffer') {
    return { type, value: bufferToMeta(value) }
  } else if (type === 'promise') {
    // Add default handler to prevent unhandled rejections in main process
    // Instead they should appear in the renderer process
    value.then(function () {}, function () {})

    return {
      type,
      then: valueToMeta(sender, contextId, function (onFulfilled: Function, onRejected: Function) {
        value.then(onFulfilled, onRejected)
      })
    }
  } else if (type === 'error') {
    const members = plainObjectToMeta(value)

    // Error.name is not part of own properties.
    members.push({
      name: 'name',
      value: value.name
    })
    return { type, members }
  } else if (type === 'date') {
    return { type, value: value.getTime() }
  } else {
    return {
      type: 'value',
      value
    }
  }
}

// Convert object to meta by value.
const plainObjectToMeta = function (obj: any): ObjectMember[] {
  return Object.getOwnPropertyNames(obj).map(function (name) {
    return {
      name: name,
      value: obj[name]
    }
  })
}

// Convert Error into meta data.
const exceptionToMeta = function (error: Error) {
  return {
    type: 'exception',
    value: errorUtils.serialize(error)
  }
}

const throwRPCError = function (message: string) {
  const error = new Error(message) as Error & {code: string, errno: number}
  error.code = 'EBADRPC'
  error.errno = -72
  throw error
}

const removeRemoteListenersAndLogWarning = (sender: any, callIntoRenderer: (...args: any[]) => void) => {
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
        sender.removeListener(eventName as any, callIntoRenderer)
      })
    }
  }

  console.warn(message)
}

type MetaTypeFromRenderer = {
  type: 'value',
  value: any
} | {
  type: 'remote-object',
  id: number
} | {
  type: 'array',
  value: MetaTypeFromRenderer[]
} | {
  type: 'buffer',
  value: BufferMeta
} | {
  type: 'date',
  value: number
} | {
  type: 'promise',
  then: MetaTypeFromRenderer
} | {
  type: 'object',
  name: string,
  members: { name: string, value: MetaTypeFromRenderer }[]
} | {
  type: 'function-with-return-value',
  value: MetaTypeFromRenderer
} | {
  type: 'function',
  id: number,
  location: string,
  length: number
}

// Convert array of meta data from renderer into array of real values.
const unwrapArgs = function (sender: electron.WebContents, frameId: number, contextId: string, args: any[]) {
  const metaToValue = function (meta: MetaTypeFromRenderer): any {
    switch (meta.type) {
      case 'value':
        return meta.value
      case 'remote-object':
        return objectsRegistry.get(meta.id)
      case 'array':
        return unwrapArgs(sender, frameId, contextId, meta.value)
      case 'buffer':
        return metaToBuffer(meta.value)
      case 'date':
        return new Date(meta.value)
      case 'promise':
        return Promise.resolve({
          then: metaToValue(meta.then)
        })
      case 'object': {
        const ret: any = {}
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

        const callIntoRenderer = function (this: any, ...args: any[]) {
          let succeed = false
          if (!sender.isDestroyed()) {
            succeed = (sender as any)._sendToFrameInternal(frameId, 'ELECTRON_RENDERER_CALLBACK', contextId, meta.id, valueToMeta(sender, contextId, args))
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
        throw new TypeError(`Unknown type: ${(meta as any).type}`)
    }
  }
  return args.map(metaToValue)
}

const isRemoteModuleEnabledImpl = function (contents: electron.WebContents) {
  const webPreferences = (contents as any).getLastWebPreferences() || {}
  return !!webPreferences.enableRemoteModule
}

const isRemoteModuleEnabledCache = new WeakMap()

const isRemoteModuleEnabled = function (contents: electron.WebContents) {
  if (!isRemoteModuleEnabledCache.has(contents)) {
    isRemoteModuleEnabledCache.set(contents, isRemoteModuleEnabledImpl(contents))
  }

  return isRemoteModuleEnabledCache.get(contents)
}

const handleRemoteCommand = function (channel: string, handler: (event: ElectronInternal.IpcMainInternalEvent, contextId: string, ...args: any[]) => void) {
  ipcMainInternal.on(channel, (event, contextId: string, ...args: any[]) => {
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

const emitCustomEvent = function (contents: electron.WebContents, eventName: string, ...args: any[]) {
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
      customEvent.returnValue = process.mainModule!.require(moduleName)
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
      customEvent.returnValue = (electron as any)[moduleName]
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
      customEvent.returnValue = (global as any)[globalName]
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
    const err = new Error(`Could not call remote function '${func.name || 'anonymous'}'. Check that the function signature is correct. Underlying error: ${error.message}\nUnderlying stack: ${error.stack}\n`);
    (err as any).cause = error
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
    const err = new Error(`Could not call remote method '${method}'. Check that the method signature is correct. Underlying error: ${error.message}\nUnderlying stack: ${error.stack}\n`);
    (err as any).cause = error
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

module.exports = {
  isRemoteModuleEnabled
}
