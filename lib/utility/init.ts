import { ParentPort } from '@electron/internal/utility/parent-port';
const Module = require('module');
const v8Util = process._linkedBinding('electron_common_v8_util');

// We modified the original process.argv to let node.js load the init.js,
// we need to restore it here.
process.argv.splice(1, 1, process._serviceStartupScript);

// Clear search paths.
require('../common/reset-search-paths');

// Import common settings.
require('@electron/internal/common/init');

const parentPort: any = new ParentPort();
Object.defineProperty(process, 'parentPort', {
  enumerable: true,
  writable: false,
  value: parentPort
});

// Based on third_party/electron_node/lib/internal/worker/io.js
parentPort.on('newListener', (name: string) => {
  if (name === 'message' && parentPort.listenerCount('message') === 0) {
    parentPort.start();
  }
});

parentPort.on('removeListener', (name: string) => {
  if (name === 'message' && parentPort.listenerCount('message') === 0) {
    parentPort.pause();
  }
});

const origNewListener = parentPort[Symbol('kNewListener')];
parentPort[Symbol('kNewListener')] = function (size: number, type: string, ...args: any[]) {
  if (type === 'message' && (size - 1) === 0) {
    parentPort.start();
  }
  args.unshift(size, type);
  Reflect.apply(origNewListener, this, args);
};

const origRemoveListener = parentPort[Symbol('kRemoveListener')];
parentPort[Symbol('kRemoveListener')] = function (size: number, type: string, ...args: any[]) {
  if (type === 'message' && size === 0) {
    parentPort.pause();
  }
  args.unshift(size, type);
  Reflect.apply(origRemoveListener, this, args);
};

// Finally load entry script.
const entryScript = v8Util.getHiddenValue(process, '_serviceStartupScript');
process._firstFileName = Module._resolveFilename(entryScript, null, false);
Module._load(entryScript, Module, true);
