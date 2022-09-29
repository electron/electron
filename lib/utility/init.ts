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

Object.defineProperty(process, 'parentPort', {
  get () {
    return parentPort;
  },
  enumerable: true
});

// Finally load entry script.
const entryScript = v8Util.getHiddenValue(process, '_serviceStartupScript');
process._firstFileName = Module._resolveFilename(entryScript, null, false);
Module._load(entryScript, Module, true);
