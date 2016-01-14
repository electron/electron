var IDWeakMap, callFunction, electron, exceptionToMeta, ipcMain, objectsRegistry, path, plainObjectToMeta, unwrapArgs, v8Util, valueToMeta,
  slice = [].slice;

path = require('path');

electron = require('electron');

ipcMain = electron.ipcMain;

objectsRegistry = require('./objects-registry');

v8Util = process.atomBinding('v8_util');

IDWeakMap = process.atomBinding('id_weak_map').IDWeakMap;

// Convert a real value into meta data.
valueToMeta = function(sender, value, optimizeSimpleObject) {
  var el, field, i, len, meta, name;
  if (optimizeSimpleObject == null) {
    optimizeSimpleObject = false;
  }
  meta = {
    type: typeof value
  };
  if (Buffer.isBuffer(value)) {
    meta.type = 'buffer';
  }
  if (value === null) {
    meta.type = 'value';
  }
  if (Array.isArray(value)) {
    meta.type = 'array';
  }
  if (value instanceof Error) {
    meta.type = 'error';
  }
  if (value instanceof Date) {
    meta.type = 'date';
  }
  if ((value != null ? value.constructor.name : void 0) === 'Promise') {
    meta.type = 'promise';
  }

  // Treat simple objects as value.
  if (optimizeSimpleObject && meta.type === 'object' && v8Util.getHiddenValue(value, 'simple')) {
    meta.type = 'value';
  }

  // Treat the arguments object as array.
  if (meta.type === 'object' && (value.callee != null) && (value.length != null)) {
    meta.type = 'array';
  }
  if (meta.type === 'array') {
    meta.members = [];
    for (i = 0, len = value.length; i < len; i++) {
      el = value[i];
      meta.members.push(valueToMeta(sender, el));
    }
  } else if (meta.type === 'object' || meta.type === 'function') {
    meta.name = value.constructor.name;

    // Reference the original value if it's an object, because when it's
    // passed to renderer we would assume the renderer keeps a reference of
    // it.
    meta.id = objectsRegistry.add(sender.getId(), value);
    meta.members = (function() {
      var results;
      results = [];
      for (name in value) {
        field = value[name];
        results.push({
          name: name,
          type: typeof field
        });
      }
      return results;
    })();
  } else if (meta.type === 'buffer') {
    meta.value = Array.prototype.slice.call(value, 0);
  } else if (meta.type === 'promise') {
    meta.then = valueToMeta(sender, value.then.bind(value));
  } else if (meta.type === 'error') {
    meta.members = plainObjectToMeta(value);

    // Error.name is not part of own properties.
    meta.members.push({
      name: 'name',
      value: value.name
    });
  } else if (meta.type === 'date') {
    meta.value = value.getTime();
  } else {
    meta.type = 'value';
    meta.value = value;
  }
  return meta;
};

// Convert object to meta by value.
plainObjectToMeta = function(obj) {
  return Object.getOwnPropertyNames(obj).map(function(name) {
    return {
      name: name,
      value: obj[name]
    };
  });
};

// Convert Error into meta data.
exceptionToMeta = function(error) {
  return {
    type: 'exception',
    message: error.message,
    stack: error.stack || error
  };
};

// Convert array of meta data from renderer into array of real values.
unwrapArgs = function(sender, args) {
  var metaToValue;
  metaToValue = function(meta) {
    var i, len, member, ref, rendererReleased, ret, returnValue;
    switch (meta.type) {
      case 'value':
        return meta.value;
      case 'remote-object':
        return objectsRegistry.get(meta.id);
      case 'array':
        return unwrapArgs(sender, meta.value);
      case 'buffer':
        return new Buffer(meta.value);
      case 'date':
        return new Date(meta.value);
      case 'promise':
        return Promise.resolve({
          then: metaToValue(meta.then)
        });
      case 'object':
        ret = v8Util.createObjectWithName(meta.name);
        ref = meta.members;
        for (i = 0, len = ref.length; i < len; i++) {
          member = ref[i];
          ret[member.name] = metaToValue(member.value);
        }
        return ret;
      case 'function-with-return-value':
        returnValue = metaToValue(meta.value);
        return function() {
          return returnValue;
        };
      case 'function':
        // Cache the callbacks in renderer.
        if (!sender.callbacks) {
          sender.callbacks = new IDWeakMap;
          sender.on('render-view-deleted', function() {
            return sender.callbacks.clear();
          });
        }
        if (sender.callbacks.has(meta.id)) {
          return sender.callbacks.get(meta.id);
        }
        rendererReleased = false;
        objectsRegistry.once("clear-" + (sender.getId()), function() {
          return rendererReleased = true;
        });
        ret = function() {
          if (rendererReleased) {
            throw new Error("Attempting to call a function in a renderer window that has been closed or released. Function provided here: " + meta.location + ".");
          }
          return sender.send('ATOM_RENDERER_CALLBACK', meta.id, valueToMeta(sender, arguments));
        };
        v8Util.setDestructor(ret, function() {
          if (rendererReleased) {
            return;
          }
          sender.callbacks.remove(meta.id);
          return sender.send('ATOM_RENDERER_RELEASE_CALLBACK', meta.id);
        });
        sender.callbacks.set(meta.id, ret);
        return ret;
      default:
        throw new TypeError("Unknown type: " + meta.type);
    }
  };
  return args.map(metaToValue);
};

// Call a function and send reply asynchronously if it's a an asynchronous
// style function and the caller didn't pass a callback.
callFunction = function(event, func, caller, args) {
  var e, error1, funcMarkedAsync, funcName, funcPassedCallback, ref, ret;
  funcMarkedAsync = v8Util.getHiddenValue(func, 'asynchronous');
  funcPassedCallback = typeof args[args.length - 1] === 'function';
  try {
    if (funcMarkedAsync && !funcPassedCallback) {
      args.push(function(ret) {
        return event.returnValue = valueToMeta(event.sender, ret, true);
      });
      return func.apply(caller, args);
    } else {
      ret = func.apply(caller, args);
      return event.returnValue = valueToMeta(event.sender, ret, true);
    }
  } catch (error1) {
    e = error1;

    // Catch functions thrown further down in function invocation and wrap
    // them with the function name so it's easier to trace things like
    // `Error processing argument -1.`
    funcName = (ref = func.name) != null ? ref : "anonymous";
    throw new Error("Could not call remote function `" + funcName + "`. Check that the function signature is correct. Underlying error: " + e.message);
  }
};

// Send by BrowserWindow when its render view is deleted.
process.on('ATOM_BROWSER_RELEASE_RENDER_VIEW', function(id) {
  return objectsRegistry.clear(id);
});

ipcMain.on('ATOM_BROWSER_REQUIRE', function(event, module) {
  var e, error1;
  try {
    return event.returnValue = valueToMeta(event.sender, process.mainModule.require(module));
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_GET_BUILTIN', function(event, module) {
  var e, error1;
  try {
    return event.returnValue = valueToMeta(event.sender, electron[module]);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_GLOBAL', function(event, name) {
  var e, error1;
  try {
    return event.returnValue = valueToMeta(event.sender, global[name]);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_CURRENT_WINDOW', function(event) {
  var e, error1;
  try {
    return event.returnValue = valueToMeta(event.sender, event.sender.getOwnerBrowserWindow());
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_CURRENT_WEB_CONTENTS', function(event) {
  return event.returnValue = valueToMeta(event.sender, event.sender);
});

ipcMain.on('ATOM_BROWSER_CONSTRUCTOR', function(event, id, args) {
  var constructor, e, error1, obj;
  try {
    args = unwrapArgs(event.sender, args);
    constructor = objectsRegistry.get(id);

    // Call new with array of arguments.
    // http://stackoverflow.com/questions/1606797/use-of-apply-with-new-operator-is-this-possible
    obj = new (Function.prototype.bind.apply(constructor, [null].concat(args)));
    return event.returnValue = valueToMeta(event.sender, obj);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_FUNCTION_CALL', function(event, id, args) {
  var e, error1, func;
  try {
    args = unwrapArgs(event.sender, args);
    func = objectsRegistry.get(id);
    return callFunction(event, func, global, args);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_MEMBER_CONSTRUCTOR', function(event, id, method, args) {
  var constructor, e, error1, obj;
  try {
    args = unwrapArgs(event.sender, args);
    constructor = objectsRegistry.get(id)[method];

    // Call new with array of arguments.
    obj = new (Function.prototype.bind.apply(constructor, [null].concat(args)));
    return event.returnValue = valueToMeta(event.sender, obj);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_MEMBER_CALL', function(event, id, method, args) {
  var e, error1, obj;
  try {
    args = unwrapArgs(event.sender, args);
    obj = objectsRegistry.get(id);
    return callFunction(event, obj[method], obj, args);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_MEMBER_SET', function(event, id, name, value) {
  var e, error1, obj;
  try {
    obj = objectsRegistry.get(id);
    obj[name] = value;
    return event.returnValue = null;
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_MEMBER_GET', function(event, id, name) {
  var e, error1, obj;
  try {
    obj = objectsRegistry.get(id);
    return event.returnValue = valueToMeta(event.sender, obj[name]);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_DEREFERENCE', function(event, id) {
  return objectsRegistry.remove(event.sender.getId(), id);
});

ipcMain.on('ATOM_BROWSER_GUEST_WEB_CONTENTS', function(event, guestInstanceId) {
  var e, error1, guestViewManager;
  try {
    guestViewManager = require('./guest-view-manager');
    return event.returnValue = valueToMeta(event.sender, guestViewManager.getGuest(guestInstanceId));
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});

ipcMain.on('ATOM_BROWSER_ASYNC_CALL_TO_GUEST_VIEW', function() {
  var args, e, error1, event, guest, guestInstanceId, guestViewManager, method;
  event = arguments[0], guestInstanceId = arguments[1], method = arguments[2], args = 4 <= arguments.length ? slice.call(arguments, 3) : [];
  try {
    guestViewManager = require('./guest-view-manager');
    guest = guestViewManager.getGuest(guestInstanceId);
    return guest[method].apply(guest, args);
  } catch (error1) {
    e = error1;
    return event.returnValue = exceptionToMeta(e);
  }
});
