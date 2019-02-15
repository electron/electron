'use strict'

const v8Util = process.atomBinding('v8_util')
const { isPromise } = require('electron')
const resolvePromise = Promise.resolve.bind(Promise)

const CallbacksRegistry = require('@electron/internal/renderer/callbacks-registry')
const bufferUtils = require('@electron/internal/common/buffer-utils')
const errorUtils = require('@electron/internal/common/error-utils')
const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal')

const callbacksRegistry = new CallbacksRegistry()
const remoteObjectCache = v8Util.createIDWeakMap()

// An unique ID that can represent current context.
const contextId = v8Util.getHiddenValue(global, 'contextId')

// Notify the main process when current context is going to be released.
// Note that when the renderer process is destroyed, the message may not be
// sent, we also listen to the "render-view-deleted" event in the main process
// to guard that situation.
process.on('exit', () => {
  const command = 'ELECTRON_BROWSER_CONTEXT_RELEASE'
  ipcRendererInternal.sendSync(command, contextId)
})

// Convert the arguments object into an array of meta data.
function wrapArgs (args, visited = new Set()) {
  const valueToMeta = (value) => {
    // Check for circular reference.
    if (visited.has(value)) {
      return {
        type: 'value',
        value: null
      }
    }

    if (Array.isArray(value)) {
      visited.add(value)
      const meta = {
        type: 'array',
        value: wrapArgs(value, visited)
      }
      visited.delete(value)
      return meta
    } else if (bufferUtils.isBuffer(value)) {
      return {
        type: 'buffer',
        value: bufferUtils.bufferToMeta(value)
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

      const meta = {
        type: 'object',
        name: value.constructor ? value.constructor.name : '',
        members: []
      }
      visited.add(value)
      for (const prop in value) {
        meta.members.push({
          name: prop,
          value: valueToMeta(value[prop])
        })
      }
      visited.delete(value)
      return meta
    } else if (typeof value === 'function' && v8Util.getHiddenValue(value, 'returnValue')) {
      return {
        type: 'function-with-return-value',
        value: valueToMeta(value())
      }
    } else if (typeof value === 'function') {
      return {
        type: 'function',
        id: callbacksRegistry.add(value),
        location: v8Util.getHiddenValue(value, 'location'),
        length: value.length
      }
    } else {
      return {
        type: 'value',
        value: value
      }
    }
  }
  return args.map(valueToMeta)
}

// Populate object's members from descriptors.
// The |ref| will be kept referenced by |members|.
// This matches |getObjectMemebers| in rpc-server.
function setObjectMembers (ref, object, metaId, members) {
  if (!Array.isArray(members)) return

  for (const member of members) {
    if (object.hasOwnProperty(member.name)) continue

    const descriptor = { enumerable: member.enumerable }
    if (member.type === 'method') {
      const remoteMemberFunction = function (...args) {
        let command
        if (this && this.constructor === remoteMemberFunction) {
          command = 'ELECTRON_BROWSER_MEMBER_CONSTRUCTOR'
        } else {
          command = 'ELECTRON_BROWSER_MEMBER_CALL'
        }
        const ret = ipcRendererInternal.sendSync(command, contextId, metaId, member.name, wrapArgs(args))
        return metaToValue(ret)
      }

      let descriptorFunction = proxyFunctionProperties(remoteMemberFunction, metaId, member.name)

      descriptor.get = () => {
        descriptorFunction.ref = ref // The member should reference its object.
        return descriptorFunction
      }
      // Enable monkey-patch the method
      descriptor.set = (value) => {
        descriptorFunction = value
        return value
      }
      descriptor.configurable = true
    } else if (member.type === 'get') {
      descriptor.get = () => {
        const command = 'ELECTRON_BROWSER_MEMBER_GET'
        const meta = ipcRendererInternal.sendSync(command, contextId, metaId, member.name)
        return metaToValue(meta)
      }

      if (member.writable) {
        descriptor.set = (value) => {
          const args = wrapArgs([value])
          const command = 'ELECTRON_BROWSER_MEMBER_SET'
          const meta = ipcRendererInternal.sendSync(command, contextId, metaId, member.name, args)
          if (meta != null) metaToValue(meta)
          return value
        }
      }
    }

    Object.defineProperty(object, member.name, descriptor)
  }
}

// Populate object's prototype from descriptor.
// This matches |getObjectPrototype| in rpc-server.
function setObjectPrototype (ref, object, metaId, descriptor) {
  if (descriptor === null) return
  const proto = {}
  setObjectMembers(ref, proto, metaId, descriptor.members)
  setObjectPrototype(ref, proto, metaId, descriptor.proto)
  Object.setPrototypeOf(object, proto)
}

// Wrap function in Proxy for accessing remote properties
function proxyFunctionProperties (remoteMemberFunction, metaId, name) {
  let loaded = false

  // Lazily load function properties
  const loadRemoteProperties = () => {
    if (loaded) return
    loaded = true
    const command = 'ELECTRON_BROWSER_MEMBER_GET'
    const meta = ipcRendererInternal.sendSync(command, contextId, metaId, name)
    setObjectMembers(remoteMemberFunction, remoteMemberFunction, meta.id, meta.members)
  }

  return new Proxy(remoteMemberFunction, {
    set: (target, property, value, receiver) => {
      if (property !== 'ref') loadRemoteProperties()
      target[property] = value
      return true
    },
    get: (target, property, receiver) => {
      if (!target.hasOwnProperty(property)) loadRemoteProperties()
      const value = target[property]
      if (property === 'toString' && typeof value === 'function') {
        return value.bind(target)
      }
      return value
    },
    ownKeys: (target) => {
      loadRemoteProperties()
      return Object.getOwnPropertyNames(target)
    },
    getOwnPropertyDescriptor: (target, property) => {
      const descriptor = Object.getOwnPropertyDescriptor(target, property)
      if (descriptor) return descriptor
      loadRemoteProperties()
      return Object.getOwnPropertyDescriptor(target, property)
    }
  })
}

// Convert meta data from browser into real value.
function metaToValue (meta) {
  const types = {
    value: () => meta.value,
    array: () => meta.members.map((member) => metaToValue(member)),
    buffer: () => bufferUtils.metaToBuffer(meta.value),
    promise: () => resolvePromise({ then: metaToValue(meta.then) }),
    error: () => metaToPlainObject(meta),
    date: () => new Date(meta.value),
    exception: () => { throw errorUtils.deserialize(meta.value) }
  }

  if (meta.type in types) {
    return types[meta.type]()
  } else {
    let ret
    if (remoteObjectCache.has(meta.id)) {
      return remoteObjectCache.get(meta.id)
    }

    // A shadow class to represent the remote function object.
    if (meta.type === 'function') {
      const remoteFunction = function (...args) {
        let command
        if (this && this.constructor === remoteFunction) {
          command = 'ELECTRON_BROWSER_CONSTRUCTOR'
        } else {
          command = 'ELECTRON_BROWSER_FUNCTION_CALL'
        }
        const obj = ipcRendererInternal.sendSync(command, contextId, meta.id, wrapArgs(args))
        return metaToValue(obj)
      }
      ret = remoteFunction
    } else {
      ret = {}
    }

    setObjectMembers(ret, ret, meta.id, meta.members)
    setObjectPrototype(ret, ret, meta.id, meta.proto)
    Object.defineProperty(ret.constructor, 'name', { value: meta.name })

    // Track delegate obj's lifetime & tell browser to clean up when object is GCed.
    v8Util.setRemoteObjectFreer(ret, contextId, meta.id)
    v8Util.setHiddenValue(ret, 'atomId', meta.id)
    remoteObjectCache.set(meta.id, ret)
    return ret
  }
}

// Construct a plain object from the meta.
function metaToPlainObject (meta) {
  const obj = (() => meta.type === 'error' ? new Error() : {})()
  for (let i = 0; i < meta.members.length; i++) {
    const { name, value } = meta.members[i]
    obj[name] = value
  }
  return obj
}

function handleMessage (channel, handler) {
  ipcRendererInternal.on(channel, (event, passedContextId, id, ...args) => {
    if (passedContextId === contextId) {
      handler(id, ...args)
    } else {
      // Message sent to an un-exist context, notify the error to main process.
      ipcRendererInternal.send('ELECTRON_BROWSER_WRONG_CONTEXT_ERROR', contextId, passedContextId, id)
    }
  })
}

// Browser calls a callback in renderer.
handleMessage('ELECTRON_RENDERER_CALLBACK', (id, args) => {
  callbacksRegistry.apply(id, metaToValue(args))
})

// A callback in browser is released.
handleMessage('ELECTRON_RENDERER_RELEASE_CALLBACK', (id) => {
  callbacksRegistry.remove(id)
})

exports.require = (module) => {
  const command = 'ELECTRON_BROWSER_REQUIRE'
  const meta = ipcRendererInternal.sendSync(command, contextId, module)
  return metaToValue(meta)
}

// Alias to remote.require('electron').xxx.
exports.getBuiltin = (module) => {
  const command = 'ELECTRON_BROWSER_GET_BUILTIN'
  const meta = ipcRendererInternal.sendSync(command, contextId, module)
  return metaToValue(meta)
}

exports.getCurrentWindow = () => {
  const command = 'ELECTRON_BROWSER_CURRENT_WINDOW'
  const meta = ipcRendererInternal.sendSync(command, contextId)
  return metaToValue(meta)
}

// Get current WebContents object.
exports.getCurrentWebContents = () => {
  const command = 'ELECTRON_BROWSER_CURRENT_WEB_CONTENTS'
  const meta = ipcRendererInternal.sendSync(command, contextId)
  return metaToValue(meta)
}

// Get a global object in browser.
exports.getGlobal = (name) => {
  const command = 'ELECTRON_BROWSER_GLOBAL'
  const meta = ipcRendererInternal.sendSync(command, contextId, name)
  return metaToValue(meta)
}

// Get the process object in browser.
exports.__defineGetter__('process', () => exports.getGlobal('process'))

// Create a function that will return the specified value when called in browser.
exports.createFunctionWithReturnValue = (returnValue) => {
  const func = () => returnValue
  v8Util.setHiddenValue(func, 'returnValue', true)
  return func
}

// Get the guest WebContents from guestInstanceId.
exports.getGuestWebContents = (guestInstanceId) => {
  const command = 'ELECTRON_BROWSER_GUEST_WEB_CONTENTS'
  const meta = ipcRendererInternal.sendSync(command, contextId, guestInstanceId)
  return metaToValue(meta)
}

const addBuiltinProperty = (name) => {
  Object.defineProperty(exports, name, {
    get: () => exports.getBuiltin(name)
  })
}

const browserModules =
  require('@electron/internal/common/api/module-list').concat(
    require('@electron/internal/browser/api/module-list'))

// And add a helper receiver for each one.
browserModules
  .filter((m) => !m.private)
  .map((m) => m.name)
  .forEach(addBuiltinProperty)
