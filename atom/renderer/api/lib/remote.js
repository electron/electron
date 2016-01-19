const ipcRenderer = require('electron').ipcRenderer;
const CallbacksRegistry = require('electron').CallbacksRegistry;
const v8Util = process.atomBinding('v8_util');

const callbacksRegistry = new CallbacksRegistry;

var includes = [].includes;

// Check for circular reference.
var isCircular = function(field, visited) {
  if (typeof field === 'object') {
    if (includes.call(visited, field)) {
      return true;
    }
    visited.push(field);
  }
  return false;
};

// Convert the arguments object into an array of meta data.
var wrapArgs = function(args, visited) {
  var valueToMeta;
  if (visited == null) {
    visited = [];
  }
  valueToMeta = function(value) {
    var field, prop, ret;
    if (Array.isArray(value)) {
      return {
        type: 'array',
        value: wrapArgs(value, visited)
      };
    } else if (Buffer.isBuffer(value)) {
      return {
        type: 'buffer',
        value: Array.prototype.slice.call(value, 0)
      };
    } else if (value instanceof Date) {
      return {
        type: 'date',
        value: value.getTime()
      };
    } else if ((value != null ? value.constructor.name : void 0) === 'Promise') {
      return {
        type: 'promise',
        then: valueToMeta(value.then.bind(value))
      };
    } else if ((value != null) && typeof value === 'object' && v8Util.getHiddenValue(value, 'atomId')) {
      return {
        type: 'remote-object',
        id: v8Util.getHiddenValue(value, 'atomId')
      };
    } else if ((value != null) && typeof value === 'object') {
      ret = {
        type: 'object',
        name: value.constructor.name,
        members: []
      };
      for (prop in value) {
        field = value[prop];
        ret.members.push({
          name: prop,
          value: valueToMeta(isCircular(field, visited) ? null : field)
        });
      }
      return ret;
    } else if (typeof value === 'function' && v8Util.getHiddenValue(value, 'returnValue')) {
      return {
        type: 'function-with-return-value',
        value: valueToMeta(value())
      };
    } else if (typeof value === 'function') {
      return {
        type: 'function',
        id: callbacksRegistry.add(value),
        location: v8Util.getHiddenValue(value, 'location')
      };
    } else {
      return {
        type: 'value',
        value: value
      };
    }
  };
  return Array.prototype.slice.call(args).map(valueToMeta);
};

// Convert meta data from browser into real value.
var metaToValue = function(meta) {
  var el, i, j, len, len1, member, ref1, ref2, results, ret;
  switch (meta.type) {
    case 'value':
      return meta.value;
    case 'array':
      ref1 = meta.members;
      results = [];
      for (i = 0, len = ref1.length; i < len; i++) {
        el = ref1[i];
        results.push(metaToValue(el));
      }
      return results;
    case 'buffer':
      return new Buffer(meta.value);
    case 'promise':
      return Promise.resolve({
        then: metaToValue(meta.then)
      });
    case 'error':
      return metaToPlainObject(meta);
    case 'date':
      return new Date(meta.value);
    case 'exception':
      throw new Error(meta.message + "\n" + meta.stack);
    default:
      if (meta.type === 'function') {
        // A shadow class to represent the remote function object.
        ret = (function() {
          function RemoteFunction() {
            var obj;
            if (this.constructor === RemoteFunction) {

              // Constructor call.
              obj = ipcRenderer.sendSync('ATOM_BROWSER_CONSTRUCTOR', meta.id, wrapArgs(arguments));

              /*
                Returning object in constructor will replace constructed object
                with the returned object.
                http://stackoverflow.com/questions/1978049/what-values-can-a-constructor-return-to-avoid-returning-this
               */
              return metaToValue(obj);
            } else {

              // Function call.
              obj = ipcRenderer.sendSync('ATOM_BROWSER_FUNCTION_CALL', meta.id, wrapArgs(arguments));
              return metaToValue(obj);
            }
          }

          return RemoteFunction;

        })();
      } else {
        ret = v8Util.createObjectWithName(meta.name);
      }

      // Polulate delegate members.
      ref2 = meta.members;
      for (j = 0, len1 = ref2.length; j < len1; j++) {
        member = ref2[j];
        if (member.type === 'function') {
          ret[member.name] = createRemoteMemberFunction(meta.id, member.name);
        } else {
          Object.defineProperty(ret, member.name, createRemoteMemberProperty(meta.id, member.name));
        }
      }

      // Track delegate object's life time, and tell the browser to clean up
      // when the object is GCed.
      v8Util.setDestructor(ret, function() {
        return ipcRenderer.send('ATOM_BROWSER_DEREFERENCE', meta.id);
      });

      // Remember object's id.
      v8Util.setHiddenValue(ret, 'atomId', meta.id);
      return ret;
  }
};

// Construct a plain object from the meta.
var metaToPlainObject = function(meta) {
  var i, len, name, obj, ref1, ref2, value;
  obj = (function() {
    switch (meta.type) {
      case 'error':
        return new Error;
      default:
        return {};
    }
  })();
  ref1 = meta.members;
  for (i = 0, len = ref1.length; i < len; i++) {
    ref2 = ref1[i], name = ref2.name, value = ref2.value;
    obj[name] = value;
  }
  return obj;
};

// Create a RemoteMemberFunction instance.
// This function's content should not be inlined into metaToValue, otherwise V8
// may consider it circular reference.
var createRemoteMemberFunction = function(metaId, name) {
  return (function() {
    function RemoteMemberFunction() {
      var ret;
      if (this.constructor === RemoteMemberFunction) {

        // Constructor call.
        ret = ipcRenderer.sendSync('ATOM_BROWSER_MEMBER_CONSTRUCTOR', metaId, name, wrapArgs(arguments));
        return metaToValue(ret);
      } else {

        // Call member function.
        ret = ipcRenderer.sendSync('ATOM_BROWSER_MEMBER_CALL', metaId, name, wrapArgs(arguments));
        return metaToValue(ret);
      }
    }

    return RemoteMemberFunction;

  })();
};

// Create configuration for defineProperty.
// This function's content should not be inlined into metaToValue, otherwise V8
// may consider it circular reference.
var createRemoteMemberProperty = function(metaId, name) {
  return {
    enumerable: true,
    configurable: false,
    set: function(value) {

      // Set member data.
      ipcRenderer.sendSync('ATOM_BROWSER_MEMBER_SET', metaId, name, value);
      return value;
    },
    get: function() {

      // Get member data.
      return metaToValue(ipcRenderer.sendSync('ATOM_BROWSER_MEMBER_GET', metaId, name));
    }
  };
};

// Browser calls a callback in renderer.
ipcRenderer.on('ATOM_RENDERER_CALLBACK', function(event, id, args) {
  return callbacksRegistry.apply(id, metaToValue(args));
});

// A callback in browser is released.
ipcRenderer.on('ATOM_RENDERER_RELEASE_CALLBACK', function(event, id) {
  return callbacksRegistry.remove(id);
});

// List all built-in modules in browser process.
const browserModules = require('../../../browser/api/lib/exports/electron');

// And add a helper receiver for each one.
var fn = function(name) {
  return Object.defineProperty(exports, name, {
    get: function() {
      return exports.getBuiltin(name);
    }
  });
};
for (var name in browserModules) {
  fn(name);
}

// Get remote module.
// (Just like node's require, the modules are cached permanently, note that this
// is safe leak since the object is not expected to get freed in browser)
var moduleCache = {};

exports.require = function(module) {
  var meta;
  if (moduleCache[module] != null) {
    return moduleCache[module];
  }
  meta = ipcRenderer.sendSync('ATOM_BROWSER_REQUIRE', module);
  return moduleCache[module] = metaToValue(meta);
};

// Optimize require('electron').
moduleCache.electron = exports;

// Alias to remote.require('electron').xxx.
var builtinCache = {};

exports.getBuiltin = function(module) {
  var meta;
  if (builtinCache[module] != null) {
    return builtinCache[module];
  }
  meta = ipcRenderer.sendSync('ATOM_BROWSER_GET_BUILTIN', module);
  return builtinCache[module] = metaToValue(meta);
};

// Get current BrowserWindow object.
var windowCache = null;

exports.getCurrentWindow = function() {
  var meta;
  if (windowCache != null) {
    return windowCache;
  }
  meta = ipcRenderer.sendSync('ATOM_BROWSER_CURRENT_WINDOW');
  return windowCache = metaToValue(meta);
};

// Get current WebContents object.
var webContentsCache = null;

exports.getCurrentWebContents = function() {
  var meta;
  if (webContentsCache != null) {
    return webContentsCache;
  }
  meta = ipcRenderer.sendSync('ATOM_BROWSER_CURRENT_WEB_CONTENTS');
  return webContentsCache = metaToValue(meta);
};

// Get a global object in browser.
exports.getGlobal = function(name) {
  var meta;
  meta = ipcRenderer.sendSync('ATOM_BROWSER_GLOBAL', name);
  return metaToValue(meta);
};

// Get the process object in browser.
var processCache = null;

exports.__defineGetter__('process', function() {
  if (processCache == null) {
    processCache = exports.getGlobal('process');
  }
  return processCache;
});

// Create a funtion that will return the specifed value when called in browser.
exports.createFunctionWithReturnValue = function(returnValue) {
  var func;
  func = function() {
    return returnValue;
  };
  v8Util.setHiddenValue(func, 'returnValue', true);
  return func;
};

// Get the guest WebContents from guestInstanceId.
exports.getGuestWebContents = function(guestInstanceId) {
  var meta;
  meta = ipcRenderer.sendSync('ATOM_BROWSER_GUEST_WEB_CONTENTS', guestInstanceId);
  return metaToValue(meta);
};
