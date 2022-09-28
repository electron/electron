import { MessagePortMain } from '@electron/internal/browser/message-port-main';
import * as events from 'events';
const Module = require('module');
const { EventEmitter } = events;
const v8Util = process._linkedBinding('electron_common_v8_util');

// We modified the original process.argv to let node.js load the init.js,
// we need to restore it here.
process.argv.splice(1, 1, process._serviceStartupScript);

// Clear search paths.
require('../common/reset-search-paths');

// Import common settings.
require('@electron/internal/common/init');

const parentPort: Electron.ParentPort = new EventEmitter() as any;
parentPort.postMessage = v8Util.getHiddenValue(process, '_postMessage');

v8Util.setHiddenValue(global, 'messagechannel', {
  didReceiveMessage (event: any, ports: any[]) {
    event.ports = ports.map(p => new MessagePortMain(p));
    (parentPort as events.EventEmitter).emit('message', event);
  }
});

let _parentPort = parentPort;
Object.defineProperty(process, 'parentPort', {
  get () {
    return _parentPort;
  },
  set (value) {
    _parentPort = value;
  },
  enumerable: false,
  configurable: true
});

// Finally load entry script.
process._firstFileName = Module._resolveFilename(process._serviceStartupScript, null, false);
Module._load(process._serviceStartupScript, Module, true);
