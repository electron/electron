import { CallbacksRegistry } from '../remote/callbacks-registry';
import { isPromise, isSerializableObject, serialize, deserialize } from '../../common/type-utils';
import { MetaTypeFromRenderer, ObjectMember, ObjProtoDescriptor, MetaType } from '../../common/remote/types';
import { ipcRendererInternal } from '../ipc-renderer-internal';
import type { BrowserWindow, WebContents } from 'electron/main';
import { browserModuleNames } from '@electron/internal/browser/api/module-names';
import { commonModuleList } from '@electron/internal/common/api/module-list';

const v8Util = process._linkedBinding('electron_common_v8_util');
const { hasSwitch } = process._linkedBinding('electron_common_command_line');

const callbacksRegistry = new CallbacksRegistry();
const remoteObjectCache = new Map();
const finalizationRegistry = new (window as any).FinalizationRegistry((id: number) => {
  const ref = remoteObjectCache.get(id);
  if (ref !== undefined && ref.deref() === undefined) {
    remoteObjectCache.delete(id);
    ipcRendererInternal.send('ELECTRON_BROWSER_DEREFERENCE', contextId, id, 0);
  }
});

function getCachedRemoteObject (id: number) {
  const ref = remoteObjectCache.get(id);
  if (ref !== undefined) {
    const deref = ref.deref();
    if (deref !== undefined) return deref;
  }
}
function setCachedRemoteObject (id: number, value: any) {
  const wr = new (window as any).WeakRef(value);
  remoteObjectCache.set(id, wr);
  finalizationRegistry.register(value, id);
  return value;
}

// An unique ID that can represent current context.
const contextId = v8Util.getHiddenValue<string>(global, 'contextId');

// Notify the main process when current context is going to be released.
// Note that when the renderer process is destroyed, the message may not be
// sent, we also listen to the "render-view-deleted" event in the main process
// to guard that situation.
process.on('exit', () => {
  const command = 'ELECTRON_BROWSER_CONTEXT_RELEASE';
  ipcRendererInternal.send(command, contextId);
});

const IS_REMOTE_PROXY = Symbol('is-remote-proxy');

// Convert the arguments object into an array of meta data.
function wrapArgs (args: any[], visited = new Set()): any {
  const valueToMeta = (value: any): any => {
    // Check for circular reference.
    if (visited.has(value)) {
      return {
        type: 'value',
        value: null
      };
    }

    if (value && value.constructor && value.constructor.name === 'NativeImage') {
      return { type: 'nativeimage', value: serialize(value) };
    } else if (Array.isArray(value)) {
      visited.add(value);
      const meta = {
        type: 'array',
        value: wrapArgs(value, visited)
      };
      visited.delete(value);
      return meta;
    } else if (value instanceof Buffer) {
      return {
        type: 'buffer',
        value
      };
    } else if (isSerializableObject(value)) {
      return {
        type: 'value',
        value
      };
    } else if (typeof value === 'object') {
      if (isPromise(value)) {
        return {
          type: 'promise',
          then: valueToMeta(function (onFulfilled: Function, onRejected: Function) {
            value.then(onFulfilled, onRejected);
          })
        };
      } else if (v8Util.getHiddenValue(value, 'electronId')) {
        return {
          type: 'remote-object',
          id: v8Util.getHiddenValue(value, 'electronId')
        };
      }

      const meta: MetaTypeFromRenderer = {
        type: 'object',
        name: value.constructor ? value.constructor.name : '',
        members: []
      };
      visited.add(value);
      for (const prop in value) { // eslint-disable-line guard-for-in
        meta.members.push({
          name: prop,
          value: valueToMeta(value[prop])
        });
      }
      visited.delete(value);
      return meta;
    } else if (typeof value === 'function' && v8Util.getHiddenValue(value, 'returnValue')) {
      return {
        type: 'function-with-return-value',
        value: valueToMeta(value())
      };
    } else if (typeof value === 'function') {
      return {
        type: 'function',
        id: callbacksRegistry.add(value),
        location: v8Util.getHiddenValue(value, 'location'),
        length: value.length
      };
    } else {
      return {
        type: 'value',
        value
      };
    }
  };
  return args.map(valueToMeta);
}

// Populate object's members from descriptors.
// The |ref| will be kept referenced by |members|.
// This matches |getObjectMemebers| in rpc-server.
function setObjectMembers (ref: any, object: any, metaId: number, members: ObjectMember[]) {
  if (!Array.isArray(members)) return;

  for (const member of members) {
    if (Object.prototype.hasOwnProperty.call(object, member.name)) continue;

    const descriptor: PropertyDescriptor = { enumerable: member.enumerable };
    if (member.type === 'method') {
      const remoteMemberFunction = function (this: any, ...args: any[]) {
        let command;
        if (this && this.constructor === remoteMemberFunction) {
          command = 'ELECTRON_BROWSER_MEMBER_CONSTRUCTOR';
        } else {
          command = 'ELECTRON_BROWSER_MEMBER_CALL';
        }
        const ret = ipcRendererInternal.sendSync(command, contextId, metaId, member.name, wrapArgs(args));
        return metaToValue(ret);
      };

      let descriptorFunction = proxyFunctionProperties(remoteMemberFunction, metaId, member.name);

      descriptor.get = () => {
        descriptorFunction.ref = ref; // The member should reference its object.
        return descriptorFunction;
      };
      // Enable monkey-patch the method
      descriptor.set = (value) => {
        descriptorFunction = value;
        return value;
      };
      descriptor.configurable = true;
    } else if (member.type === 'get') {
      descriptor.get = () => {
        const command = 'ELECTRON_BROWSER_MEMBER_GET';
        const meta = ipcRendererInternal.sendSync(command, contextId, metaId, member.name);
        return metaToValue(meta);
      };

      if (member.writable) {
        descriptor.set = (value) => {
          const args = wrapArgs([value]);
          const command = 'ELECTRON_BROWSER_MEMBER_SET';
          const meta = ipcRendererInternal.sendSync(command, contextId, metaId, member.name, args);
          if (meta != null) metaToValue(meta);
          return value;
        };
      }
    }

    Object.defineProperty(object, member.name, descriptor);
  }
}

// Populate object's prototype from descriptor.
// This matches |getObjectPrototype| in rpc-server.
function setObjectPrototype (ref: any, object: any, metaId: number, descriptor: ObjProtoDescriptor) {
  if (descriptor === null) return;
  const proto = {};
  setObjectMembers(ref, proto, metaId, descriptor.members);
  setObjectPrototype(ref, proto, metaId, descriptor.proto);
  Object.setPrototypeOf(object, proto);
}

// Wrap function in Proxy for accessing remote properties
function proxyFunctionProperties (remoteMemberFunction: Function, metaId: number, name: string) {
  let loaded = false;

  // Lazily load function properties
  const loadRemoteProperties = () => {
    if (loaded) return;
    loaded = true;
    const command = 'ELECTRON_BROWSER_MEMBER_GET';
    const meta = ipcRendererInternal.sendSync(command, contextId, metaId, name);
    setObjectMembers(remoteMemberFunction, remoteMemberFunction, meta.id, meta.members);
  };

  return new Proxy(remoteMemberFunction as any, {
    set: (target, property, value) => {
      if (property !== 'ref') loadRemoteProperties();
      target[property] = value;
      return true;
    },
    get: (target, property) => {
      if (property === IS_REMOTE_PROXY) return true;
      if (!Object.prototype.hasOwnProperty.call(target, property)) loadRemoteProperties();
      const value = target[property];
      if (property === 'toString' && typeof value === 'function') {
        return value.bind(target);
      }
      return value;
    },
    ownKeys: (target) => {
      loadRemoteProperties();
      return Object.getOwnPropertyNames(target);
    },
    getOwnPropertyDescriptor: (target, property) => {
      const descriptor = Object.getOwnPropertyDescriptor(target, property);
      if (descriptor) return descriptor;
      loadRemoteProperties();
      return Object.getOwnPropertyDescriptor(target, property);
    }
  });
}

// Convert meta data from browser into real value.
function metaToValue (meta: MetaType): any {
  if (meta.type === 'value') {
    return meta.value;
  } else if (meta.type === 'array') {
    return meta.members.map((member) => metaToValue(member));
  } else if (meta.type === 'nativeimage') {
    return deserialize(meta.value);
  } else if (meta.type === 'buffer') {
    return Buffer.from(meta.value.buffer, meta.value.byteOffset, meta.value.byteLength);
  } else if (meta.type === 'promise') {
    return Promise.resolve({ then: metaToValue(meta.then) });
  } else if (meta.type === 'error') {
    return metaToError(meta);
  } else if (meta.type === 'exception') {
    if (meta.value.type === 'error') { throw metaToError(meta.value); } else { throw new Error(`Unexpected value type in exception: ${meta.value.type}`); }
  } else {
    let ret;
    if ('id' in meta) {
      const cached = getCachedRemoteObject(meta.id);
      if (cached !== undefined) { return cached; }
    }

    // A shadow class to represent the remote function object.
    if (meta.type === 'function') {
      const remoteFunction = function (this: any, ...args: any[]) {
        let command;
        if (this && this.constructor === remoteFunction) {
          command = 'ELECTRON_BROWSER_CONSTRUCTOR';
        } else {
          command = 'ELECTRON_BROWSER_FUNCTION_CALL';
        }
        const obj = ipcRendererInternal.sendSync(command, contextId, meta.id, wrapArgs(args));
        return metaToValue(obj);
      };
      ret = remoteFunction;
    } else {
      ret = {};
    }

    setObjectMembers(ret, ret, meta.id, meta.members);
    setObjectPrototype(ret, ret, meta.id, meta.proto);
    if (ret.constructor && (ret.constructor as any)[IS_REMOTE_PROXY]) {
      Object.defineProperty(ret.constructor, 'name', { value: meta.name });
    }

    // Track delegate obj's lifetime & tell browser to clean up when object is GCed.
    v8Util.setHiddenValue(ret, 'electronId', meta.id);
    setCachedRemoteObject(meta.id, ret);
    return ret;
  }
}

function metaToError (meta: { type: 'error', value: any, members: ObjectMember[] }) {
  const obj = meta.value;
  for (const { name, value } of meta.members) {
    obj[name] = metaToValue(value);
  }
  return obj;
}

function handleMessage (channel: string, handler: Function) {
  ipcRendererInternal.on(channel, (event, passedContextId, id, ...args) => {
    if (passedContextId === contextId) {
      handler(id, ...args);
    } else {
      // Message sent to an un-exist context, notify the error to main process.
      ipcRendererInternal.send('ELECTRON_BROWSER_WRONG_CONTEXT_ERROR', contextId, passedContextId, id);
    }
  });
}

const enableStacks = hasSwitch('enable-api-filtering-logging');

function getCurrentStack (): string | undefined {
  const target = { stack: undefined as string | undefined };
  if (enableStacks) {
    Error.captureStackTrace(target, getCurrentStack);
  }
  return target.stack;
}

// Browser calls a callback in renderer.
handleMessage('ELECTRON_RENDERER_CALLBACK', (id: number, args: any) => {
  callbacksRegistry.apply(id, metaToValue(args));
});

// A callback in browser is released.
handleMessage('ELECTRON_RENDERER_RELEASE_CALLBACK', (id: number) => {
  callbacksRegistry.remove(id);
});

exports.require = (module: string) => {
  const command = 'ELECTRON_BROWSER_REQUIRE';
  const meta = ipcRendererInternal.sendSync(command, contextId, module, getCurrentStack());
  return metaToValue(meta);
};

// Alias to remote.require('electron').xxx.
export function getBuiltin (module: string) {
  const command = 'ELECTRON_BROWSER_GET_BUILTIN';
  const meta = ipcRendererInternal.sendSync(command, contextId, module, getCurrentStack());
  return metaToValue(meta);
}

export function getCurrentWindow (): BrowserWindow {
  const command = 'ELECTRON_BROWSER_CURRENT_WINDOW';
  const meta = ipcRendererInternal.sendSync(command, contextId, getCurrentStack());
  return metaToValue(meta);
}

// Get current WebContents object.
export function getCurrentWebContents (): WebContents {
  const command = 'ELECTRON_BROWSER_CURRENT_WEB_CONTENTS';
  const meta = ipcRendererInternal.sendSync(command, contextId, getCurrentStack());
  return metaToValue(meta);
}

// Get a global object in browser.
export function getGlobal<T = any> (name: string): T {
  const command = 'ELECTRON_BROWSER_GLOBAL';
  const meta = ipcRendererInternal.sendSync(command, contextId, name, getCurrentStack());
  return metaToValue(meta);
}

// Get the process object in browser.
Object.defineProperty(exports, 'process', {
  get: () => exports.getGlobal('process')
});

// Create a function that will return the specified value when called in browser.
export function createFunctionWithReturnValue<T> (returnValue: T): () => T {
  const func = () => returnValue;
  v8Util.setHiddenValue(func, 'returnValue', true);
  return func;
}

const addBuiltinProperty = (name: string) => {
  Object.defineProperty(exports, name, {
    get: () => exports.getBuiltin(name)
  });
};

const browserModules = commonModuleList.concat(browserModuleNames.map(name => ({ name, loader: () => {} })));

// And add a helper receiver for each one.
browserModules
  .filter((m) => !m.private)
  .map((m) => m.name)
  .forEach(addBuiltinProperty);
