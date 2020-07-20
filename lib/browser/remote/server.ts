import * as electron from 'electron/main';
import { EventEmitter } from 'events';
import objectsRegistry from '@electron/internal/browser/remote/objects-registry';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import { isPromise, isSerializableObject, deserialize, serialize } from '@electron/internal/common/type-utils';
import type { MetaTypeFromRenderer, ObjectMember, MetaType, ObjProtoDescriptor } from '@electron/internal/common/remote/types';

const v8Util = process._linkedBinding('electron_common_v8_util');
const eventBinding = process._linkedBinding('electron_browser_event');
const features = process._linkedBinding('electron_common_features');

if (!features.isRemoteModuleEnabled()) {
  throw new Error('remote module is disabled');
}

// The internal properties of Function.
const FUNCTION_PROPERTIES = [
  'length', 'name', 'arguments', 'caller', 'prototype'
];

type RendererFunctionId = [string, number] // [contextId, funcId]
type FinalizerInfo = { id: RendererFunctionId, webContents: electron.WebContents, frameId: number };
type WeakRef<T> = { deref(): T | undefined }
type CallIntoRenderer = (...args: any[]) => void

// The remote functions in renderer processes.
const rendererFunctionCache = new Map<string, WeakRef<CallIntoRenderer>>();
// eslint-disable-next-line no-undef
const finalizationRegistry = new (globalThis as any).FinalizationRegistry((fi: FinalizerInfo) => {
  const mapKey = fi.id[0] + '~' + fi.id[1];
  const ref = rendererFunctionCache.get(mapKey);
  if (ref !== undefined && ref.deref() === undefined) {
    rendererFunctionCache.delete(mapKey);
    if (!fi.webContents.isDestroyed()) { fi.webContents.sendToFrame(fi.frameId, 'ELECTRON_RENDERER_RELEASE_CALLBACK', fi.id[0], fi.id[1]); }
  }
});

function getCachedRendererFunction (id: RendererFunctionId): CallIntoRenderer | undefined {
  const mapKey = id[0] + '~' + id[1];
  const ref = rendererFunctionCache.get(mapKey);
  if (ref !== undefined) {
    const deref = ref.deref();
    if (deref !== undefined) return deref;
  }
}
function setCachedRendererFunction (id: RendererFunctionId, wc: electron.WebContents, frameId: number, value: CallIntoRenderer) {
  // eslint-disable-next-line no-undef
  const wr = new (globalThis as any).WeakRef(value) as WeakRef<CallIntoRenderer>;
  const mapKey = id[0] + '~' + id[1];
  rendererFunctionCache.set(mapKey, wr);
  finalizationRegistry.register(value, {
    id,
    webContents: wc,
    frameId
  } as FinalizerInfo);
  return value;
}

// Return the description of object's members:
const getObjectMembers = function (object: any): ObjectMember[] {
  let names = Object.getOwnPropertyNames(object);
  // For Function, we should not override following properties even though they
  // are "own" properties.
  if (typeof object === 'function') {
    names = names.filter((name) => {
      return !FUNCTION_PROPERTIES.includes(name);
    });
  }
  // Map properties to descriptors.
  return names.map((name) => {
    const descriptor = Object.getOwnPropertyDescriptor(object, name)!;
    let type: ObjectMember['type'];
    let writable = false;
    if (descriptor.get === undefined && typeof object[name] === 'function') {
      type = 'method';
    } else {
      if (descriptor.set || descriptor.writable) writable = true;
      type = 'get';
    }
    return { name, enumerable: descriptor.enumerable, writable, type };
  });
};

// Return the description of object's prototype.
const getObjectPrototype = function (object: any): ObjProtoDescriptor {
  const proto = Object.getPrototypeOf(object);
  if (proto === null || proto === Object.prototype) return null;
  return {
    members: getObjectMembers(proto),
    proto: getObjectPrototype(proto)
  };
};

// Convert a real value into meta data.
const valueToMeta = function (sender: electron.WebContents, contextId: string, value: any, optimizeSimpleObject = false): MetaType {
  // Determine the type of value.
  let type: MetaType['type'];

  switch (typeof value) {
    case 'object':
      // Recognize certain types of objects.
      if (value instanceof Buffer) {
        type = 'buffer';
      } else if (value && value.constructor && value.constructor.name === 'NativeImage') {
        type = 'nativeimage';
      } else if (Array.isArray(value)) {
        type = 'array';
      } else if (value instanceof Error) {
        type = 'error';
      } else if (isSerializableObject(value)) {
        type = 'value';
      } else if (isPromise(value)) {
        type = 'promise';
      } else if (Object.prototype.hasOwnProperty.call(value, 'callee') && value.length != null) {
        // Treat the arguments object as array.
        type = 'array';
      } else if (optimizeSimpleObject && v8Util.getHiddenValue(value, 'simple')) {
        // Treat simple objects as value.
        type = 'value';
      } else {
        type = 'object';
      }
      break;
    case 'function':
      type = 'function';
      break;
    default:
      type = 'value';
      break;
  }

  // Fill the meta object according to value's type.
  if (type === 'array') {
    return {
      type,
      members: value.map((el: any) => valueToMeta(sender, contextId, el, optimizeSimpleObject))
    };
  } else if (type === 'nativeimage') {
    return { type, value: serialize(value) };
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
    };
  } else if (type === 'buffer') {
    return { type, value };
  } else if (type === 'promise') {
    // Add default handler to prevent unhandled rejections in main process
    // Instead they should appear in the renderer process
    value.then(function () {}, function () {});

    return {
      type,
      then: valueToMeta(sender, contextId, function (onFulfilled: Function, onRejected: Function) {
        value.then(onFulfilled, onRejected);
      })
    };
  } else if (type === 'error') {
    return {
      type,
      value,
      members: Object.keys(value).map(name => ({
        name,
        value: valueToMeta(sender, contextId, value[name])
      }))
    };
  } else {
    return {
      type: 'value',
      value
    };
  }
};

const throwRPCError = function (message: string) {
  const error = new Error(message) as Error & {code: string, errno: number};
  error.code = 'EBADRPC';
  error.errno = -72;
  throw error;
};

const removeRemoteListenersAndLogWarning = (sender: any, callIntoRenderer: (...args: any[]) => void) => {
  const location = v8Util.getHiddenValue(callIntoRenderer, 'location');
  let message = 'Attempting to call a function in a renderer window that has been closed or released.' +
    `\nFunction provided here: ${location}`;

  if (sender instanceof EventEmitter) {
    const remoteEvents = sender.eventNames().filter((eventName) => {
      return sender.listeners(eventName).includes(callIntoRenderer);
    });

    if (remoteEvents.length > 0) {
      message += `\nRemote event names: ${remoteEvents.join(', ')}`;
      remoteEvents.forEach((eventName) => {
        sender.removeListener(eventName as any, callIntoRenderer);
      });
    }
  }

  console.warn(message);
};

const fakeConstructor = (constructor: Function, name: string) =>
  new Proxy(Object, {
    get (target, prop, receiver) {
      if (prop === 'name') {
        return name;
      } else {
        return Reflect.get(target, prop, receiver);
      }
    }
  });

// Convert array of meta data from renderer into array of real values.
const unwrapArgs = function (sender: electron.WebContents, frameId: number, contextId: string, args: any[]) {
  const metaToValue = function (meta: MetaTypeFromRenderer): any {
    switch (meta.type) {
      case 'nativeimage':
        return deserialize(meta.value);
      case 'value':
        return meta.value;
      case 'remote-object':
        return objectsRegistry.get(meta.id);
      case 'array':
        return unwrapArgs(sender, frameId, contextId, meta.value);
      case 'buffer':
        return Buffer.from(meta.value.buffer, meta.value.byteOffset, meta.value.byteLength);
      case 'promise':
        return Promise.resolve({
          then: metaToValue(meta.then)
        });
      case 'object': {
        const ret: any = meta.name !== 'Object' ? Object.create({
          constructor: fakeConstructor(Object, meta.name)
        }) : {};

        for (const { name, value } of meta.members) {
          ret[name] = metaToValue(value);
        }
        return ret;
      }
      case 'function-with-return-value': {
        const returnValue = metaToValue(meta.value);
        return function () {
          return returnValue;
        };
      }
      case 'function': {
        // Merge contextId and meta.id, since meta.id can be the same in
        // different webContents.
        const objectId: [string, number] = [contextId, meta.id];

        // Cache the callbacks in renderer.
        const cachedFunction = getCachedRendererFunction(objectId);
        if (cachedFunction !== undefined) { return cachedFunction; }

        const callIntoRenderer = function (this: any, ...args: any[]) {
          let succeed = false;
          if (!sender.isDestroyed()) {
            succeed = (sender as any)._sendToFrameInternal(frameId, 'ELECTRON_RENDERER_CALLBACK', contextId, meta.id, valueToMeta(sender, contextId, args));
          }
          if (!succeed) {
            removeRemoteListenersAndLogWarning(this, callIntoRenderer);
          }
        };
        v8Util.setHiddenValue(callIntoRenderer, 'location', meta.location);
        Object.defineProperty(callIntoRenderer, 'length', { value: meta.length });

        setCachedRendererFunction(objectId, sender, frameId, callIntoRenderer);
        return callIntoRenderer;
      }
      default:
        throw new TypeError(`Unknown type: ${(meta as any).type}`);
    }
  };
  return args.map(metaToValue);
};

const isRemoteModuleEnabledImpl = function (contents: electron.WebContents) {
  const webPreferences = contents.getLastWebPreferences() || {};
  return webPreferences.enableRemoteModule != null ? !!webPreferences.enableRemoteModule : false;
};

const isRemoteModuleEnabledCache = new WeakMap();

export const isRemoteModuleEnabled = function (contents: electron.WebContents) {
  if (!isRemoteModuleEnabledCache.has(contents)) {
    isRemoteModuleEnabledCache.set(contents, isRemoteModuleEnabledImpl(contents));
  }

  return isRemoteModuleEnabledCache.get(contents);
};

const handleRemoteCommand = function (channel: string, handler: (event: ElectronInternal.IpcMainInternalEvent, contextId: string, ...args: any[]) => void) {
  ipcMainInternal.on(channel, (event, contextId: string, ...args: any[]) => {
    let returnValue;
    if (!isRemoteModuleEnabled(event.sender)) {
      event.returnValue = null;
      return;
    }

    try {
      returnValue = handler(event, contextId, ...args);
    } catch (error) {
      returnValue = {
        type: 'exception',
        value: valueToMeta(event.sender, contextId, error)
      };
    }

    if (returnValue !== undefined) {
      event.returnValue = returnValue;
    }
  });
};

const emitCustomEvent = function (contents: electron.WebContents, eventName: string, ...args: any[]) {
  const event = eventBinding.createWithSender(contents);

  electron.app.emit(eventName, event, contents, ...args);
  contents.emit(eventName, event, ...args);

  return event;
};

const logStack = function (contents: electron.WebContents, code: string, stack: string | undefined) {
  if (stack) {
    console.warn(`WebContents (${contents.id}): ${code}`, stack);
  }
};

handleRemoteCommand('ELECTRON_BROWSER_WRONG_CONTEXT_ERROR', function (event, contextId, passedContextId, id) {
  const objectId: [string, number] = [passedContextId, id];
  const cachedFunction = getCachedRendererFunction(objectId);
  if (cachedFunction === undefined) {
    // Do nothing if the error has already been reported before.
    return;
  }
  removeRemoteListenersAndLogWarning(event.sender, cachedFunction);
});

handleRemoteCommand('ELECTRON_BROWSER_REQUIRE', function (event, contextId, moduleName, stack) {
  logStack(event.sender, `remote.require('${moduleName}')`, stack);
  const customEvent = emitCustomEvent(event.sender, 'remote-require', moduleName);

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.require('${moduleName}')`);
    } else {
      customEvent.returnValue = process.mainModule!.require(moduleName);
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue);
});

handleRemoteCommand('ELECTRON_BROWSER_GET_BUILTIN', function (event, contextId, moduleName, stack) {
  logStack(event.sender, `remote.getBuiltin('${moduleName}')`, stack);
  const customEvent = emitCustomEvent(event.sender, 'remote-get-builtin', moduleName);

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.getBuiltin('${moduleName}')`);
    } else {
      customEvent.returnValue = (electron as any)[moduleName];
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue);
});

handleRemoteCommand('ELECTRON_BROWSER_GLOBAL', function (event, contextId, globalName, stack) {
  logStack(event.sender, `remote.getGlobal('${globalName}')`, stack);
  const customEvent = emitCustomEvent(event.sender, 'remote-get-global', globalName);

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error(`Blocked remote.getGlobal('${globalName}')`);
    } else {
      customEvent.returnValue = (global as any)[globalName];
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue);
});

handleRemoteCommand('ELECTRON_BROWSER_CURRENT_WINDOW', function (event, contextId, stack) {
  logStack(event.sender, 'remote.getCurrentWindow()', stack);
  const customEvent = emitCustomEvent(event.sender, 'remote-get-current-window');

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error('Blocked remote.getCurrentWindow()');
    } else {
      customEvent.returnValue = event.sender.getOwnerBrowserWindow();
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue);
});

handleRemoteCommand('ELECTRON_BROWSER_CURRENT_WEB_CONTENTS', function (event, contextId, stack) {
  logStack(event.sender, 'remote.getCurrentWebContents()', stack);
  const customEvent = emitCustomEvent(event.sender, 'remote-get-current-web-contents');

  if (customEvent.returnValue === undefined) {
    if (customEvent.defaultPrevented) {
      throw new Error('Blocked remote.getCurrentWebContents()');
    } else {
      customEvent.returnValue = event.sender;
    }
  }

  return valueToMeta(event.sender, contextId, customEvent.returnValue);
});

handleRemoteCommand('ELECTRON_BROWSER_CONSTRUCTOR', function (event, contextId, id, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args);
  const constructor = objectsRegistry.get(id);

  if (constructor == null) {
    throwRPCError(`Cannot call constructor on missing remote object ${id}`);
  }

  return valueToMeta(event.sender, contextId, new constructor(...args));
});

handleRemoteCommand('ELECTRON_BROWSER_FUNCTION_CALL', function (event, contextId, id, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args);
  const func = objectsRegistry.get(id);

  if (func == null) {
    throwRPCError(`Cannot call function on missing remote object ${id}`);
  }

  try {
    return valueToMeta(event.sender, contextId, func(...args), true);
  } catch (error) {
    const err = new Error(`Could not call remote function '${func.name || 'anonymous'}'. Check that the function signature is correct. Underlying error: ${error.message}\nUnderlying stack: ${error.stack}\n`);
    (err as any).cause = error;
    throw err;
  }
});

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_CONSTRUCTOR', function (event, contextId, id, method, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args);
  const object = objectsRegistry.get(id);

  if (object == null) {
    throwRPCError(`Cannot call constructor '${method}' on missing remote object ${id}`);
  }

  return valueToMeta(event.sender, contextId, new object[method](...args));
});

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_CALL', function (event, contextId, id, method, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args);
  const object = objectsRegistry.get(id);

  if (object == null) {
    throwRPCError(`Cannot call method '${method}' on missing remote object ${id}`);
  }

  try {
    return valueToMeta(event.sender, contextId, object[method](...args), true);
  } catch (error) {
    const err = new Error(`Could not call remote method '${method}'. Check that the method signature is correct. Underlying error: ${error.message}\nUnderlying stack: ${error.stack}\n`);
    (err as any).cause = error;
    throw err;
  }
});

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_SET', function (event, contextId, id, name, args) {
  args = unwrapArgs(event.sender, event.frameId, contextId, args);
  const obj = objectsRegistry.get(id);

  if (obj == null) {
    throwRPCError(`Cannot set property '${name}' on missing remote object ${id}`);
  }

  obj[name] = args[0];
  return null;
});

handleRemoteCommand('ELECTRON_BROWSER_MEMBER_GET', function (event, contextId, id, name) {
  const obj = objectsRegistry.get(id);

  if (obj == null) {
    throwRPCError(`Cannot get property '${name}' on missing remote object ${id}`);
  }

  return valueToMeta(event.sender, contextId, obj[name]);
});

handleRemoteCommand('ELECTRON_BROWSER_DEREFERENCE', function (event, contextId, id) {
  objectsRegistry.remove(event.sender, contextId, id);
});

handleRemoteCommand('ELECTRON_BROWSER_CONTEXT_RELEASE', (event, contextId) => {
  objectsRegistry.clear(event.sender, contextId);
});
