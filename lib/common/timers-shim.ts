// Drop-in replacement for timers-browserify@1.4.2.
// Provides the Node.js 'timers' API surface for renderer/web webpack bundles
// without relying on window.postMessage (which the newer timers-browserify 2.x
// polyfill uses and can interfere with Electron IPC).

const immediateIds: Record<number, boolean> = {};
let nextImmediateId = 0;

// --- DOM timer wrappers ------------------------------------------------
// Wrap the global setTimeout/setInterval so we return Timeout objects that
// expose ref/unref/close matching the Node.js API shape.

class Timeout {
  _id: ReturnType<typeof globalThis.setTimeout>;
  _clearFn: (id: ReturnType<typeof globalThis.setTimeout>) => void;

  constructor(
    id: ReturnType<typeof globalThis.setTimeout>,
    clearFn: (id: ReturnType<typeof globalThis.setTimeout>) => void
  ) {
    this._id = id;
    this._clearFn = clearFn;
  }

  unref() {}
  ref() {}

  close() {
    this._clearFn.call(globalThis, this._id);
  }
}

export const setTimeout = function (...args: Parameters<typeof globalThis.setTimeout>): Timeout {
  return new Timeout(globalThis.setTimeout(...args), globalThis.clearTimeout);
};

export const setInterval = function (...args: Parameters<typeof globalThis.setInterval>): Timeout {
  return new Timeout(globalThis.setInterval(...args), globalThis.clearInterval);
};

export const clearTimeout = function (timeout: Timeout | undefined) {
  if (timeout) timeout.close();
};

export const clearInterval = clearTimeout;

// --- Legacy enroll/unenroll (Node < 11 API, preserved for compatibility) ------

interface EnrollableItem {
  _idleTimeoutId?: ReturnType<typeof setTimeout>;
  _idleTimeout?: number;
  _onTimeout?: () => void;
}

export const enroll = function (item: EnrollableItem, msecs: number) {
  clearTimeout(item._idleTimeoutId);
  item._idleTimeout = msecs;
};

export const unenroll = function (item: EnrollableItem) {
  clearTimeout(item._idleTimeoutId);
  item._idleTimeout = -1;
};

export const active = function (item: EnrollableItem) {
  clearTimeout(item._idleTimeoutId);

  const msecs = item._idleTimeout;
  if (msecs !== undefined && msecs >= 0) {
    item._idleTimeoutId = setTimeout(function onTimeout() {
      if (item._onTimeout) item._onTimeout();
    }, msecs);
  }
};

export const _unrefActive = active;

// --- setImmediate / clearImmediate -------------------------------------
// Prefer the native implementations when available. Fall back to a
// nextTick-based shim that avoids window.postMessage.

const clearImmediateFallback = function (id: number) {
  delete immediateIds[id];
};

export const setImmediate =
  typeof globalThis.setImmediate === 'function'
    ? globalThis.setImmediate
    : function (fn: (...args: unknown[]) => void, ...rest: unknown[]) {
        const id = nextImmediateId++;

        immediateIds[id] = true;

        Promise.resolve().then(function onMicrotask() {
          if (immediateIds[id]) {
            fn(...rest);
            clearImmediateFallback(id);
          }
        });

        return id;
      };

export const clearImmediate =
  typeof globalThis.clearImmediate === 'function' ? globalThis.clearImmediate : clearImmediateFallback;
