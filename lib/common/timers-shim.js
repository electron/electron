// Drop-in replacement for timers-browserify@1.4.2.
// Provides the Node.js 'timers' API surface for renderer/web webpack bundles
// without relying on window.postMessage (which the newer timers-browserify 2.x
// polyfill uses and can interfere with Electron IPC).

'use strict';

var apply = Function.prototype.apply;
var slice = Array.prototype.slice;
var immediateIds = {};
var nextImmediateId = 0;

// --- DOM timer wrappers ------------------------------------------------
// Wrap the global setTimeout/setInterval so we return Timeout objects that
// expose ref/unref/close matching the Node.js API shape.

function Timeout (id, clearFn) {
  this._id = id;
  this._clearFn = clearFn;
}

Timeout.prototype.unref = Timeout.prototype.ref = function () {};
Timeout.prototype.close = function () {
  this._clearFn.call(globalThis, this._id);
};

exports.setTimeout = function () {
  return new Timeout(apply.call(setTimeout, globalThis, arguments), clearTimeout);
};

exports.setInterval = function () {
  return new Timeout(apply.call(setInterval, globalThis, arguments), clearInterval);
};

exports.clearTimeout = exports.clearInterval = function (timeout) {
  if (timeout) timeout.close();
};

// --- Legacy enroll/unenroll (Node < 11 API, preserved for compatibility) ------

exports.enroll = function (item, msecs) {
  clearTimeout(item._idleTimeoutId);
  item._idleTimeout = msecs;
};

exports.unenroll = function (item) {
  clearTimeout(item._idleTimeoutId);
  item._idleTimeout = -1;
};

exports._unrefActive = exports.active = function (item) {
  clearTimeout(item._idleTimeoutId);

  var msecs = item._idleTimeout;
  if (msecs >= 0) {
    item._idleTimeoutId = setTimeout(function onTimeout () {
      if (item._onTimeout) item._onTimeout();
    }, msecs);
  }
};

// --- setImmediate / clearImmediate -------------------------------------
// Prefer the native implementations when available. Fall back to a
// nextTick-based shim that avoids window.postMessage.

exports.setImmediate = typeof setImmediate === 'function'
  ? setImmediate
  : function (fn) {
    var id = nextImmediateId++;
    var args = arguments.length < 2 ? false : slice.call(arguments, 1);

    immediateIds[id] = true;

    Promise.resolve().then(function onMicrotask () {
      if (immediateIds[id]) {
        if (args) {
          fn.apply(null, args);
        } else {
          fn.call(null);
        }
        exports.clearImmediate(id);
      }
    });

    return id;
  };

exports.clearImmediate = typeof clearImmediate === 'function'
  ? clearImmediate
  : function (id) {
    delete immediateIds[id];
  };
