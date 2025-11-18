import timers = require('timers');
import * as util from 'util';

import type * as stream from 'stream';

type AnyFn = (...args: any[]) => any

// Activate UV loop for timer callbacks to run immediately
const wrapWithActivateUvLoop = function <T extends AnyFn> (func: T): T {
  return wrap(func, function (func) {
    return function (this: any, ...args: any[]) {
      process.activateUvLoop();
      return func.apply(this, args);
    };
  }) as T;
};

/**
 * Casts to any below for func are due to Typescript not supporting symbols
 * in index signatures
 *
 * Refs: https://github.com/Microsoft/TypeScript/issues/1863
 */
function wrap <T extends AnyFn> (func: T, wrapper: (fn: AnyFn) => T) {
  const wrapped = wrapper(func);
  if ((func as any)[util.promisify.custom]) {
    (wrapped as any)[util.promisify.custom] = wrapper((func as any)[util.promisify.custom]);
  }
  return wrapped;
}

process.nextTick = wrapWithActivateUvLoop(process.nextTick);
global.setImmediate = timers.setImmediate = wrapWithActivateUvLoop(timers.setImmediate);
global.clearImmediate = timers.clearImmediate;

timers.setTimeout = wrapWithActivateUvLoop(timers.setTimeout);
timers.setInterval = wrapWithActivateUvLoop(timers.setInterval);

if (process.type === 'browser' ||
    process.type === 'utility') {
  global.setTimeout = timers.setTimeout;
  global.setInterval = timers.setInterval;
}

if (process.platform === 'win32') {
  const { Readable } = require('stream') as typeof stream;
  const stdin = new Readable();
  stdin.push(null);
  Object.defineProperty(process, 'stdin', {
    configurable: false,
    enumerable: true,
    get () {
      return stdin;
    }
  });
}

const Module = require('module') as NodeJS.ModuleInternal;

const makeElectronModule = (name: string) => {
  const electronModule = new Module('electron', null);
  electronModule.id = 'electron';
  electronModule.loaded = true;
  electronModule.filename = name;
  Object.defineProperty(electronModule, 'exports', {
    get: () => require('electron')
  });
  Module._cache[name] = electronModule;
};

makeElectronModule('electron');
makeElectronModule('electron/common');
if (process.type === 'browser') {
  makeElectronModule('electron/main');
} else if (process.type === 'renderer') {
  makeElectronModule('electron/renderer');
} else if (process.type === 'utility') {
  makeElectronModule('electron/utility');
}

const originalResolveFilename = Module._resolveFilename;

// Module aliases for TypeScript: 'electron/{common,main,renderer,utility}'
const electronModuleNames = new Set([
  'electron', 'electron/main', 'electron/renderer', 'electron/common', 'electron/utility'
]);
Module._resolveFilename = function (request, parent, isMain, options) {
  if (electronModuleNames.has(request)) {
    return 'electron';
  } else {
    return originalResolveFilename(request, parent, isMain, options);
  }
};
