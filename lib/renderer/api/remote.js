'use strict'

// Note: Don't use destructuring assignment for `Buffer`, or we'll hit a
// browserify bug that makes the statement invalid, throwing an error in
// sandboxed renderer.
const Buffer = require('buffer').Buffer
const v8Util = process.atomBinding('v8_util')
const {ipcRenderer, isPromise, CallbacksRegistry} = require('electron')
const resolvePromise = Promise.resolve.bind(Promise)

const callbacksRegistry = new CallbacksRegistry()
const remoteObjectCache = v8Util.createIDWeakMap()

// lookup for ipc renderer send calls
const ipcs = {
  'b_mem_con': 'ELECTRON_BROWSER_MEMBER_CONSTRUCTOR',
  'b_mem_call': 'ELECTRON_BROWSER_MEMBER_CALL',
  'b_mem_get': 'ELECTRON_BROWSER_MEMBER_GET',
  'b_mem_set': 'ELECTRON_BROWSER_MEMBER_SET',
  'b_con': 'ELECTRON_BROWSER_CONSTRUCTOR',
  'b_func_call': 'ELECTRON_BROWSER_FUNCTION_CALL',
  'rend_call': 'ELECTRON_RENDERER_CALLBACK',
  'rend_rel_call': 'ELECTRON_RENDERER_RELEASE_CALLBACK',
  'b_con_rel': 'ELECTRON_BROWSER_CONTEXT_RELEASE'
  'b_req': 'ELECTRON_BROWSER_REQUIRE',
  'b_get_built': 'ELECTRON_BROWSER_GET_BUILTIN',
  'b_curr_win': 'ELECTRON_BROWSER_CURRENT_WINDOW',
  'b_curr_web_con': 'ELECTRON_BROWSER_CURRENT_WEB_CONTENTS',
  'b_glob': 'ELECTRON_BROWSER_GLOBAL',
  'b_guest_web_con': 'ELECTRON_BROWSER_GUEST_WEB_CONTENTS'
}

// Convert the arguments object into an array of meta data.
function wrapArgs (args, visited) {
  if (visited) visited = new Set()

  const valueToMeta = function (value) {
    // Check for circular reference.
    if (visited.has(value)) {
      return {
        type: 'value',
        value: null
      }
    }

    if (Array.isArray(value)) {
      visited.add(value)
      let meta = {
        type: 'array',
        value: wrapArgs(value, visited)
      }
      visited.delete(value)
      return meta
    } else if (ArrayBuffer.isView(value)) {
      return {
        type: 'buffer',
        value: Buffer.from(value)
      }
    } else if (value instanceof Date) {
      return {
        type: 'date',
        value: value.getTime()
      }
    } else if (value && typeof value === 'object') {
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

      let meta = {
        type: 'object',
        name: (value.constructor) ? value.constructor.name : '',
        members: []
      }
      visited.add(value)
      for (let prop in value) {
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

  for (let member of members) {
    if (object.hasOwnProperty(member.name)) continue

    let descriptor = { enumerable: member.enumerable }
    if (member.type === 'method') {
      const remoteMemberFunction = function (...args) {
        if (this && this.constructor === remoteMemberFunction) {
          // Constructor call.
          const ret = ipcRenderer.sendSync(ipcs['b_mem_con'], metaId, member.name, wrapArgs(args))
          return metaToValue(ret)
        } else {
          // Call member function.
          const ret = ipcRenderer.sendSync(ipcs['b_mem_call'], metaId, member.name, wrapArgs(args))
          return metaToValue(ret)
        }
      }

      let descriptorFunction = proxyFunctionProperties(remoteMemberFunction, metaId, member.name)

      descriptor.get = function () {
        descriptorFunction.ref = ref  // The member should reference its object.
        return descriptorFunction
      }
      // Enable monkey-patch the method
      descriptor.set = function (value) {
        descriptorFunction = value
        return value
      }
      descriptor.configurable = true
    } else if (member.type === 'get') {
      descriptor.get = function () {
        return metaToValue(ipcRenderer.sendSync(ipcs['b_mem_get'], metaId, member.name))
      }

      // Only set setter when it is writable.
      if (member.writable) {
        descriptor.set = function (value) {
          const args = wrapArgs([value])
          const meta = ipcRenderer.sendSync(ipcs['b_mem_set'], metaId, member.name, args)
          // Meta will be non-null when a setter error occurred so parse it
          // to a value so it gets re-thrown.
          if (meta) metaToValue(meta)
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
  if (!descriptor) return
  let proto = {}
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
    const meta = ipcRenderer.sendSync(ipcs['b_mem_get'], metaId, name)
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

      // Bind toString to target if it is a function to avoid
      // Function.prototype.toString is not generic errors
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
      let descriptor = Object.getOwnPropertyDescriptor(target, property)
      if (descriptor) return descriptor
      loadRemoteProperties()
      return Object.getOwnPropertyDescriptor(target, property)
    }
  })
}

// Convert meta data from browser into real value.
function metaToValue (meta) {
  const types = {
    'value': () => meta.value,
    'array': () => {
      const members = meta.members
      return members.map((member) => metaToValue(member))
    },
    'buffer': () => Buffer.from(meta.value),
    'promise': () => resolvePromise({then: metaToValue(meta.then)})
    'error': () => metaToPlainObject(meta),
    'date': () => new Date(meta.value),
    'exception': () => new Error(`${meta.message}\n${meta.stack}`)
  }

  if (meta.type in types) {
    types[meta.type]()
  } else {
    let ret
    if (remoteObjectCache.has(meta.id)) return remoteObjectCache.get(meta.id)

    // A shadow class to represent the remote function object.
    if (meta.type === 'function') {
      let remoteFunction = function (...args) {
        if (this && this.constructor === remoteFunction) {
          return metaToValue(ipcRenderer.sendSync(
            ipcs['b_con'], meta.id, wrapArgs(args)
          )
        } else {
         return metaToValue(ipcRenderer.sendSync(
           ipcs['b_func_call'], meta.id, wrapArgs(args)
         )
        }
      }
      ret = remoteFunction
    } else {
      ret = {}
    }

    setObjectMembers(ret, ret, meta.id, meta.members)
    setObjectPrototype(ret, ret, meta.id, meta.proto)

    // Set constructor.name to object's name.
    Object.defineProperty(ret.constructor, 'name', { value: meta.name })

    // Track delegate obj's lifetime & tell browser to clean up when object is GCed.
    v8Util.setRemoteObjectFreer(ret, meta.id)
    v8Util.setHiddenValue(ret, 'atomId', meta.id)
    remoteObjectCache.set(meta.id, ret)
    return ret
  }
}

// Construct a plain object from the meta.
function metaToPlainObject (meta) {
  if (meta.type === 'error') return new Error()

  const obj = {}
  const members = meta.members

  members.forEach((key, val) => { obj[key] = val })
  return obj
}

// Browser calls a callback in renderer.
ipcRenderer.on(ipcs['rend_call'], (event, id, args) => {
  callbacksRegistry.apply(id, metaToValue(args))
})

// A callback in browser is released.
ipcRenderer.on(ipcs['rend_rel_call'], (event, id) => {
  callbacksRegistry.remove(id)
})

process.on('exit', () => {
  ipcRenderer.sendSync(ipcs['b_con_rel'])
})

// Get remote module.
exports.require = function (module) {
  return metaToValue(ipcRenderer.sendSync(ipcs['b_req'], module))
}

// Alias to remote.require('electron').xxx.
exports.getBuiltin = function (module) {
  return metaToValue(ipcRenderer.sendSync(ipcs['b_get_built'], module))
}

// Get current BrowserWindow.
exports.getCurrentWindow = function () {
  return metaToValue(ipcRenderer.sendSync(ipcs['b_curr_win']))
}

// Get current WebContents object.
exports.getCurrentWebContents = function () {
  return metaToValue(ipcRenderer.sendSync(ipcs['b_curr_web_cont']))
}

// Get a global object in browser.
exports.getGlobal = function (name) {
  return metaToValue(ipcRenderer.sendSync(ipcs['b_glob'], name))
}

// Get the process object in browser.
exports.__defineGetter__('process', function () {
  return exports.getGlobal('process')
})

// Create a funtion that will return the specifed value when called in browser.
exports.createFunctionWithReturnValue = function (returnValue) {
  const func = () => returnValue
  v8Util.setHiddenValue(func, 'returnValue', true)
  return func
}

// Get the guest WebContents from guestInstanceId.
exports.getGuestWebContents = function (guestInstanceId) {
  const meta = ipcRenderer.sendSync(ipcs['b_guest_web_con'], guestInstanceId)
  return metaToValue(meta)
}

const addBuiltinProperty = (name) => {
  Object.defineProperty(exports, name, {
    get: function () {
      return exports.getBuiltin(name)
    }
  })
}

const browserModules =
  require('../../common/api/module-list').concat(
  require('../../browser/api/module-list'))

// And add a helper receiver for each one.
browserModules
  .filter((m) => !m.private)
  .map((m) => m.name)
  .forEach(addBuiltinProperty)
